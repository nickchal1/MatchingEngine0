#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <stdio.h>

#include "Protocol.h"

int main() {

    struct addrinfo *servinfo;

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(nullptr, "8080", &hints, &servinfo);
    if (status != 0) {
        std::cerr << "get addr info: " << gai_strerror(status) << "\n";
        return 1;
    }

    int socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_fd == -1) {
        perror("socket");
        return 1;
    }

    int conn = connect(socket_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (conn == -1) {
        perror("connection");
        return 1;
    }

    freeaddrinfo(servinfo);

    std::vector<std::uint8_t> buff;
    NewOrder newOrder{5, Side::Buy, 100, 1000};
    encode(newOrder, buff);

    int sen = send(socket_fd, buff.data(), NewOrderSize, 0);
    if (sen == -1) {
        perror("send");
        return 1;
    }

    close(socket_fd);
}