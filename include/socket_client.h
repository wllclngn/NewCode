#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

#include "socket_interface.h"
#include <string>

class SocketClient : public SocketInterface
{
private:
    int port;
    int server_fd = -1; // Server socket file descriptor
    int client_fd = -1; // Client socket file descriptor

public:
    SocketClient(int port);
    ~SocketClient();

    bool initialize() override;
    std::string receive(); // Custom receive function returning string
    void run() override;   // Not used for now, but must be implemented
    void cleanup() override;

    bool transmit(const std::string &message);
};

#endif
