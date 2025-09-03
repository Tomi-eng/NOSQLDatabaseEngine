#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <chrono>
#include <sstream>

// System includes for networking and epoll
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

class Connection {
public:
    int fd;
    std::string read_buffer;
    std::string write_buffer;
    size_t write_offset;
    bool should_close;
    std::chrono::steady_clock::time_point last_activity;
    
    Connection(int socket_fd) 
        : fd(socket_fd), write_offset(0), should_close(false),
          last_activity(std::chrono::steady_clock::now()) {
        read_buffer.reserve(4096);
        write_buffer.reserve(4096);
    }
    
    bool has_pending_write() const {
        return write_offset < write_buffer.size();
    }
    
    void queue_write(const std::string& data) {
        write_buffer.append(data);
        last_activity = std::chrono::steady_clock::now();
    }
    
    void update_activity() {
        last_activity = std::chrono::steady_clock::now();
    }
    
    bool is_timeout(std::chrono::seconds timeout) const {
        auto now = std::chrono::steady_clock::now();
        return (now - last_activity) > timeout;
    }
};

class EventLoopServer {
private:
    int server_fd;
    int epoll_fd;
    std::unordered_map<int, std::unique_ptr<Connection>> connections;
    std::queue<std::function<void()>> task_queue;
    bool running;
    
    static constexpr int MAX_EVENTS = 1024;
    static constexpr int BUFFER_SIZE = 4096;
    static constexpr std::chrono::seconds CONNECTION_TIMEOUT{30};
    
public:
    EventLoopServer(int port) : server_fd(-1), epoll_fd(-1), running(false) {
        setup_server(port);
        setup_epoll();
    }
    
    ~EventLoopServer() {
        cleanup();
    }
    
    void run() {
        std::cout << "Event loop server starting..." << std::endl;
        running = true;
        
        struct epoll_event events[MAX_EVENTS];
        auto last_cleanup = std::chrono::steady_clock::now();
        
        while (running) {
            // Wait for events with timeout for periodic tasks
            int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
            
            if (event_count == -1) {
                if (errno == EINTR) continue;
                perror("epoll_wait failed");
                break;
            }
            
            // Process all ready events
            for (int i = 0; i < event_count; i++) {
                handle_event(events[i]);
            }
            
            // Process queued tasks (non-blocking operations)
            process_task_queue();
            
            // Periodic cleanup (every 5 seconds)
            auto now = std::chrono::steady_clock::now();
            if (now - last_cleanup > std::chrono::seconds(5)) {
                cleanup_connections();
                last_cleanup = now;
            }
        }
        
        std::cout << "Event loop server stopped." << std::endl;
    }
    
    void stop() {
        running = false;
    }
    
    // Queue a task to be executed in the next event loop iteration
    void queue_task(std::function<void()> task) {
        task_queue.push(std::move(task));
    }
    
private:
    void setup_server(int port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            throw std::runtime_error("Failed to create server socket");
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set socket options");
        }
        
        // Make server socket non-blocking
        make_non_blocking(server_fd);
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            throw std::runtime_error("Failed to bind server socket");
        }
        
        if (listen(server_fd, SOMAXCONN) < 0) {
            throw std::runtime_error("Failed to listen on server socket");
        }
        
        std::cout << "Server listening on port " << port << std::endl;
    }
    
    void setup_epoll() {
        epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd == -1) {
            throw std::runtime_error("Failed to create epoll instance");
        }
        
        // Add server socket to epoll
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLET; // Edge-triggered
        event.data.fd = server_fd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
            throw std::runtime_error("Failed to add server socket to epoll");
        }
    }
    
    void make_non_blocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            throw std::runtime_error("Failed to get socket flags");
        }
        
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw std::runtime_error("Failed to set socket non-blocking");
        }
    }
    
    void handle_event(const struct epoll_event& event) {
        int fd = event.data.fd;
        
        if (fd == server_fd) {
            // New connection
            handle_new_connection();
        } else {
            // Existing connection
            auto it = connections.find(fd);
            if (it != connections.end()) {
                if (event.events & (EPOLLERR | EPOLLHUP)) {
                    close_connection(fd);
                } else if (event.events & EPOLLIN) {
                    handle_read(it->second.get());
                } else if (event.events & EPOLLOUT) {
                    handle_write(it->second.get());
                }
            }
        }
    }
    
    void handle_new_connection() {
        while (true) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No more connections to accept
                    break;
                }
                perror("Accept failed");
                continue;
            }
            
            // Make client socket non-blocking
            make_non_blocking(client_fd);
            
            // Create connection object
            auto connection = std::make_unique