#include <fcntl.h>
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
#include <sys/epoll.h>
#include <unordered_map>
#include <memory>

enum ConnectionStatus {
STATE_READ= 0,
STATE_WRITE= 1,
STATE_END = 2
};

void make_non_blocking(int fd);
void process_poll_events(int server_fd, std::vector<struct epoll_event> &poll_fds, std::unordered_map<int, std::unique_ptr<Connection>> &connections);