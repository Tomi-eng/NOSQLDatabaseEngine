#include <cstring>
#include <iostream>
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
#include "databaseutils.h"


struct addrinfo hints;
struct addrinfo *servinfo;

int main(){
    int status;
    int sock_fd;
    char *hostname = "localhost";

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    if ((status = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
        exit(1);
    }

    // servinfo now points to a linked list of 1 or more
    // struct addrinfos
    // loop through all the results and bind to the first we can
    struct addrinfo *s;
    int p = 1;

    for(s = servinfo; s != NULL; s->ai_next){
        sock_fd = socket(s->ai_family, s->ai_socktype, s->ai_protocol);
        if (sock_fd < 0) {
            continue;
        }
        if((connect(sock_fd, s->ai_addr, s->ai_addrlen) < 0)){
            continue;
        }
        break;
    }

    if (s == NULL){
        exit(1);
    }

    // ... do everything until you don't need servinfo anymore ....
    freeaddrinfo(servinfo); // free the linked-list
    std::cout << "Database client successfully connected" << std::endl;
}