
#define PORT "8080"
#include <sys/socket.h>
#include <unistd.h> // For close()
#include <string>
#include <iostream>
#include <vector>

const ssize_t MAX_BUFFER_SIZE = 4096;
// Header for our message protocol
struct MessageHeader {
    uint32_t messageSize; // Total size of the message (header + body)
};

// Represents a complete message with a header and a body
struct Message {
    MessageHeader header;
    std::vector<char> body;
};



class Connection {
public:
    int fd;
    std::vector<char> read_buffer;
    std::vector <char> write_buffer;
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

int send_data(int fd, const char * message, ssize_t bytes);
int recieve_data(int fd, char * buffer, ssize_t bytes);
bool send_message(int fd, Message msg);
bool recieve_message(int fd, Message& msg);


