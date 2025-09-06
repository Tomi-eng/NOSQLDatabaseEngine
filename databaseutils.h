
#define PORT "8080"
#include <sys/socket.h>
#include <unistd.h> // For close()
#include <string>
#include <iostream>
#include <vector>
#include <chrono>

const ssize_t MAX_BUFFER_SIZE = 4096;

class Connection {
public:
    int fd;
    std::vector<u_int8_t> read_buffer;
    std::vector <u_int8_t> write_buffer;
    size_t read_offset;
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

    void update_activity() {
        last_activity = std::chrono::steady_clock::now();
    }
};

int send_data(int fd, const char * message, ssize_t bytes);
int recieve_data(int fd, char * buffer, ssize_t bytes);


