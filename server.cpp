#include <cstring>
#include <iostream>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/epoll.h>
#include "databaseutils.h"
#include "server_func.h"

const int MAX_USERS = 10;
const int TIMEOUT = 2500;


int main(){
    int status;
    int sock_fd;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        exit(1);
    }

    // servinfo now points to a linked list of 1 or more
    // struct addrinfos
    // loop through all the results and bind to the first we can
    struct addrinfo *s;
    int p = 1;

    for(s = servinfo; s != NULL; s->ai_next){
        sock_fd = socket(s->ai_family, s->ai_socktype, s->ai_protocol);
        if (sock_fd == -1) {
            continue;
        }
        if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &p, sizeof(int)) == -1) {
            continue;
        }

        if ((bind(sock_fd, s->ai_addr, s->ai_addrlen)) == -1){
            continue;
        }
        break;
    }

    if (s == NULL){
        throw std::runtime_error("No available sockets to bind on port ");
    }

    // ... do everything until you don't need servinfo anymore ....
    freeaddrinfo(servinfo); // free the linked-list
    if ((listen(sock_fd, MAX_USERS)) == -1){
        throw std::runtime_error("Failed to listen on server socket");
    }

    make_non_blocking(sock_fd);
    std::cout << "Server listening on port " << PORT << std::endl;
    
    std::vector<struct pollfd> poll_fds;
    struct pollfd listener = {.events=POLL_IN, .fd= sock_fd};
    poll_fds.push_back(listener);

    std::unordered_map<int, std::unique_ptr<Connection>> connections;
    while(1){
        int poll_events = poll(poll_fds.data(), (nfds_t)poll_fds.size(), -1);
        if (poll_events < 0){
            std::cerr << "Error: invalid message with only message header"<< std::endl;
            break;
        }

        // process poll events
        process_poll_events(sock_fd, poll_fds, connections);
    }
}