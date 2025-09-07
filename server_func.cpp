#include "server_func.h"
#include "databaseutils.h"

void make_non_blocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags");
    }
    flags |= O_NONBLOCK; // Add the O_NONBLOCK flag
    if (fcntl(fd, F_SETFL, flags) == -1) {
       throw std::runtime_error("Failed to set socket as non blocking");
    }
}


void handle_new_connection(int epoll_fd, int server_fd, std::vector<struct epoll_event> &poll_fds, std::unordered_map<int, std::unique_ptr<Connection>> &connections){
    struct epoll_event ev;
    int client_fd;
    socklen_t socklen;
    struct sockaddr_storage client_addr;

    for(;;){
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &socklen);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more connections to accept
                break;
            }
            throw std::runtime_error("Error accepting new connections");
        }
        // Make client socket non-blocking
        make_non_blocking(client_fd);

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd; 
        poll_fds.push_back(ev);
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0){
            close(client_fd);
            continue;
        }

    }
    return;
}

void disconnect_client(int epoll_fd,int client_fd){
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0){
        throw std::system_error(errno, std::generic_category(), "Failed to open an epoll file descriptor.");
    }
}

void process_poll_events(int server_fd, int epoll_fd, std::vector<struct epoll_event> &poll_fds, std::unordered_map<int, std::unique_ptr<Connection>> &connections){
    for(int i = poll_fds.size() - 1; i  ; i = i - 1) {
        // Check if someone's ready to read
        struct epoll_event event = poll_fds[i];
        if (event.data.fd == server_fd) {
            // New connection(s) available
            handle_new_connection(epoll_fd, server_fd, poll_fds, connections);
        }else{
            // Client event
            if (event.events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // Client disconnected or error
                disconnect_client(epoll_fd, event.data.fd);
            } else if (event.events & EPOLLIN) {
                // Data available to read
                HandleClientRequest(fd);
            }
        }
    }
}