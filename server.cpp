#include "databaseutils.h"
#include "server_func.h"

const int MAX_USERS = 10;
const int TIMEOUT = 2500;


int main(){
    int status, listener_fd, epoll_fd, poll_events;
    struct addrinfo hints, *servinfo;
    std::string errorMessage;
    std::vector<struct epoll_event> poll_fds;
    struct epoll_event ev;

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
        listener_fd = socket(s->ai_family, s->ai_socktype, s->ai_protocol);
        if (listener_fd == -1) {
            continue;
        }
        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &p, sizeof(int)) == -1) {
            continue;
        }

        if ((bind(listener_fd, s->ai_addr, s->ai_addrlen)) == -1){
            continue;
        }
        break;
    }

    if (s == NULL){
        throw std::runtime_error("No available sockets to bind on port ");
    }

    // ... do everything until you don't need servinfo anymore ....
    freeaddrinfo(servinfo); // free the linked-list
    if ((listen(listener_fd, MAX_USERS)) == -1){
        throw std::runtime_error("Failed to listen on server socket");
    }

    make_non_blocking(listener_fd);
    std::cout << "Server listening on port " << PORT << std::endl;

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open an epoll file descriptor.");
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listener_fd; 
    poll_fds.push_back(ev);

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener_fd, &ev) < 0){
        throw std::system_error(errno, std::generic_category(), "Failed to open an epoll file descriptor.");
    }

    std::unordered_map<int, std::unique_ptr<Connection>> connections;
    for (;;){
        int poll_events = epoll_wait(epoll_fd, poll_fds.data(), poll_fds.size(), -1);
        if (poll_events < 0){
            throw std::system_error(errno, std::generic_category(), "Error waiting on epoll event.");
        }

        // process poll events
        process_poll_events(listener_fd, poll_fds, connections);
    }
}