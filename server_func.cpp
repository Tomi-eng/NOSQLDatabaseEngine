#include "server_func.h"
#include <sys/socket.h>
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


void handle_new_connection(int server_fd, std::unordered_map<int, std::unique_ptr<Connection>> &connections){
    int client_fd;
    socklen_t socklen;
    struct sockaddr_storage client_addr;

    socklen = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &socklen);
    if (client_fd < 0) {
        throw std::runtime_error("Error accepting new connection");
    }

}

void handle_client_connection(int server_fd){
    int client_fd;
    socklen_t socklen;
    struct sockaddr_storage client_addr;

    socklen = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &socklen);
    if (client_fd < 0) {
        throw std::runtime_error("Failed to set socket as non blocking");
    }
}

void process_poll_events(int server_fd, std::vector<struct pollfd> &poll_fds, std::unordered_map<int, std::unique_ptr<Connection>> &connections){
    for(int i = poll_fds.size() - 1; i >= 0 ; i = i - 1) {
        // Check if someone's ready to read
        struct pollfd event = poll_fds[i];
        if (event.revents & (POLLIN | POLLHUP)) {
            if (event.fd == server_fd) {
                // If we're the listener, it's a new connection
                handle_new_connection(server_fd, connections);
            } else {
                // handle client connections
                handle_client_connection(server_fd, fd_count, *pfds, &i);
            }
        }
    }
}