#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "Order.h"
#include "OrderBook.h"
#include "Protocol.h"

int main() {

    OrderBook book{};

    addrinfo hints{};
    addrinfo* res = nullptr;
    sockaddr_storage their_addr{};
    socklen_t addr_size = sizeof their_addr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; //tcp
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(nullptr, "8080", &hints, &res);
    if (status != 0) {
        std::cerr << "get addr info: " << gai_strerror(status) << "\n";
        return 1;
    }

    int listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("setsockopt");
        close(listen_fd);
        return 1;
    }

    if (bind(listen_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    freeaddrinfo(res);

    if (listen(listen_fd, 10) == -1) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    std::cout << "waiting for connection\n";

    while (1) {
        addr_size = sizeof their_addr;
        int client_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&their_addr), &addr_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        auto* client = reinterpret_cast<sockaddr_in*>(&their_addr);
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client->sin_addr, ip, sizeof ip);
        std::cout << "client connected from " << ip << ":" << ntohs(client->sin_port) << "\n";

        //buffer
        char buff[1024];
        std::vector<std::uint8_t> pending;

        bool error = false;

        while (!error) {
            int n = recv(client_fd, buff, sizeof buff, 0);

            if (n > 0) {
                // append all bytes recv
                pending.insert(pending.end(), buff, buff + n);
                std::cout << "received " << n << "bytes\n";
                // send(client_fd, buff, n, 0);
               
                while (pending.size() >= NewOrderSize) {
                    //ITS STD OPTIONAL
                    std::vector<std::uint8_t> currOrder(pending.begin(), pending.begin() + NewOrderSize);
                    auto newOrder = decode(currOrder);
                    if (!newOrder) {
                        std::cout << "error\n";
                        error = true;
                        break;
                    }
                    // ClientOrderId id = newOrder->clientId;
                    Quantity quantity = newOrder->quantity;
                    Price price = newOrder->price;
                    Side side = newOrder->side;

                    // id, q, p, side
                    auto submissionResult = book.addOrder(quantity, price, side);
                    if (!submissionResult) {
                        std::cout << "rejected\n";
                    }
                    else {
                        std::cout << submissionResult->orderId << '\n';
                    }
                    pending.erase(pending.begin(), pending.begin() + NewOrderSize);
                }
            }
            else if (n == 0) {
                std::cout << "disconnected\n";
                break;
            }
            else {
                perror("recv");
                break;
            }
        }
        close(client_fd);
    }
}
