//databaseutils.cpp
#include "databaseutils.h"

int send_data(int fd, const char * message, ssize_t bytes){
    ssize_t bytes_sent = 0;
    ssize_t s;
    while (bytes_sent < bytes){
        s = send(fd, message, bytes - bytes_sent, 0);
        if(s < 0){
            std::cerr << "Error: " << strerror(errno) << std::endl;
            return -1;
        }
        bytes_sent += s;
        message += s;
    }
    return 0;
}

int recieve_data(int fd, char * buffer, ssize_t bytes){
    ssize_t bytes_recieved = 0;
    ssize_t s;
    while( bytes_recieved < bytes){
        s = recv(fd, buffer, bytes - bytes_recieved, 0);
        if(s < 0){
            std::cerr << "Error: " << strerror(errno) << std::endl;
            return -1;
        }
        bytes_recieved += s;
        buffer += s;
    }
    return 0;
}

bool send_message(int fd, Message msg){
    MessageHeader header;
    header.messageSize = sizeof(MessageHeader) + msg.body.size();

    // 2. Allocate the buffer and copy data
    std::vector<char> messageBuffer(header.messageSize);
    memcpy(messageBuffer.data(), &header, sizeof(MessageHeader));
    memcpy(messageBuffer.data() + sizeof(MessageHeader), msg.body.data(), msg.body.size());

    if (send_data(fd, messageBuffer.data(), header.messageSize) < 0)
        return false;
    return true;
}

bool recieve_message(int fd, Message& msg){
    std::vector<char> headerBuffer(sizeof(MessageHeader));
    int s = recieve_data(fd, headerBuffer.data(), sizeof(MessageHeader));
    if (s < 0){
        std::cerr << "Error: " << strerror(errno) << std::endl;
        return false;
    }
    MessageHeader *ptr = reinterpret_cast<MessageHeader *>(headerBuffer.data());
    msg.header = *ptr;

    if (msg.header.messageSize <= sizeof(MessageHeader)) {
        std::cerr << "Error: invalid message with only message header"<< std::endl;
        return false; // Invalid size
    }

    int s = recieve_data(fd, msg.body.data(), msg.header.messageSize);
    if (s < 0){
        std::cerr << "Error: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}