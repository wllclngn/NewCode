#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/socket_interface.h"

class TransmitterSocket : public SocketInterface
{
    int sock;
    sockaddr_in server_addr{};
    std::string message;

public:
    bool initialize() override
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            perror("Socket creation failed");
            return false;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(12345);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        return true;
    }

    void run() override
    {
        if (connect(sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("Connection failed");
            return;
        }

        std::cout << "Enter message to send: ";
        std::getline(std::cin, message); // get entire line

        send(sock, message.c_str(), message.size(), 0);
        std::cout << "Message sent: " << message << std::endl;
    }

    void cleanup() override
    {
        close(sock);
    }
};

int main()
{
    TransmitterSocket tx;
    if (tx.initialize())
    {
        tx.run();
        tx.cleanup();
    }
    return 0;
}
