#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/socket_interface.h"

class ReceiverSocket : public SocketInterface
{
    int server_fd, new_socket;
    sockaddr_in address{};
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};

public:
    bool initialize() override
    {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0)
        {
            perror("Socket failed");
            return false;
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(12345);

        if (bind(server_fd, (sockaddr *)&address, sizeof(address)) < 0)
        {
            perror("Bind failed");
            return false;
        }

        if (listen(server_fd, 1) < 0)
        {
            perror("Listen failed");
            return false;
        }

        return true;
    }

    void run() override
    {
        std::cout << "Waiting for connection..." << std::endl;
        new_socket = accept(server_fd, (sockaddr *)&address, &addrlen);
        if (new_socket < 0)
        {
            perror("Accept failed");
            return;
        }

        read(new_socket, buffer, sizeof(buffer));
        std::cout << "Received message: " << buffer << std::endl;
    }

    void cleanup() override
    {
        close(new_socket);
        close(server_fd);
    }
};

int main()
{
    ReceiverSocket rx;
    if (rx.initialize())
    {
        rx.run();
        rx.cleanup();
    }
    return 0;
}
