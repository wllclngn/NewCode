#include "../include/socket_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

SocketClient::SocketClient(int port) : port(port) {}

SocketClient::~SocketClient()
{
    cleanup();
}

bool SocketClient::initialize()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket failed");
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(port);

    // Allow reuse of the port immediately after program exits
    int opt = 1;

    // Set SO_REUSEADDR
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        return false;
    }

    // Set SO_REUSEPORT (optional, but if you want it, you must set it separately)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt SO_REUSEPORT failed");
        return false;
    }

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        return false;
    }

    if (listen(server_fd, 1) < 0)
    {
        perror("listen failed");
        return false;
    }

    std::cout << "Server listening on port " << port << std::endl;

    // Accept one client connection (blocking)
    socklen_t addrlen = sizeof(address);
    client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_fd < 0)
    {
        perror("accept failed");
        return false;
    }

    std::cout << "Client connected" << std::endl;
    return true;
}

std::string SocketClient::receive()
{
    if (client_fd < 0)
        return "";

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t valread = read(client_fd, buffer, sizeof(buffer) - 1);
    if (valread <= 0)
    {
        if (valread == 0)
            std::cout << "Client disconnected" << std::endl;
        else
            perror("read failed");
        return "";
    }

    // Return received string (trim trailing \n or \r if needed)
    std::string msg(buffer);
    if (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
        msg.pop_back();

    return msg;
}

void SocketClient::run()
{
    // Could implement continuous loop here if desired
}

void SocketClient::cleanup()
{
    if (client_fd >= 0)
    {
        close(client_fd);
        client_fd = -1;
    }

    if (server_fd >= 0)
    {
        close(server_fd);
        server_fd = -1;
    }
}

bool SocketClient::transmit(const std::string &message)
{
    if (client_fd < 0)
    {
        std::cerr << "Client socket is not connected.\n";
        return false;
    }

    ssize_t bytes_sent = send(client_fd, message.c_str(), message.length(), 0);
    if (bytes_sent < 0)
    {
        perror("send failed");
        return false;
    }

    return bytes_sent == (ssize_t)message.length();
}
