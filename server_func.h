#include <fcntl.h>
#include <iostream>
#include <poll.h>

enum ConnectionStatus {
STATE_READ= 0,
STATE_WRITE= 1,
STATE_END = 2
};

void make_non_blocking(int fd);
void process_poll_events(int server_fd, std::vector<struct pollfd> &poll_fds, std::unordered_map<int, std::unique_ptr<Connection>> &connections);