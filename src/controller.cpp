#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>
#include <sstream>
#include "../include/socket_interface.h"

class Controller : public SocketInterface
{
    int sock;
    sockaddr_in server_addr{};
    std::string command;
    std::string initial_command;
    bool running = true;

public:
    Controller(const std::string &cmd = "") : initial_command(cmd) {}

    bool initialize() override
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            perror("socket");
            return false;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(54000);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        return true;
    }

    void run() override
    {
        if (connect(sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("connect");
            return;
        }

        // Start listener for heartbeats and other messages
        std::thread listener_thread(&Controller::listen_for_messages, this);
        listener_thread.detach();

        if (!initial_command.empty())
        {
            std::string transformed_command = transform_command(initial_command);
            send(sock, transformed_command.c_str(), transformed_command.size(), 0);
            std::cout << "Command sent: " << transformed_command << std::endl;

            if (transformed_command == "exit")
            {
                running = false;
                return;
            }
        }

        // Interactive mode
        while (running)
        {
            std::cout << "Enter command (or exit to quit): ";
            std::getline(std::cin, command);

            if (command.empty())
                continue;

            std::string transformed_command = transform_command(command);
            send(sock, transformed_command.c_str(), transformed_command.size(), 0);
            std::cout << "Command sent: " << transformed_command << std::endl;

            if (transformed_command == "exit")
            {
                running = false;
                break;
            }
        }
    }

    std::string transform_command(const std::string &raw_command)
    {
        std::istringstream iss(raw_command);
        std::string program, subcommand;
        iss >> program >> subcommand;

        if (program == "./mapreduce" && (subcommand == "mapper" || subcommand == "reducer"))
        {
            std::string new_command = (subcommand == "mapper") ? "map" : "reduce";
            std::string remaining_args;
            std::getline(iss, remaining_args);
            return new_command + remaining_args; // include the leading space from getline
        }

        // If it's not a ./mapreduce command, send as-is
        return raw_command;
    }

    void listen_for_messages()
    {
        char buffer[1024];

        while (running)
        {
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);

            if (bytes_received <= 0)
            {
                std::cout << "Connection closed or error." << std::endl;
                running = false;
                break;
            }

            std::string msg(buffer);
            std::cout << "\n[Worker Message] " << msg << std::endl;
        }
    }

    void cleanup() override
    {
        close(sock);
    }
};

int main(int argc, char *argv[])
{
    std::string initial_command;

    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            initial_command += argv[i];
            if (i < argc - 1)
                initial_command += " ";
        }
    }

    Controller ctrl(initial_command);
    if (ctrl.initialize())
    {
        ctrl.run();
        ctrl.cleanup();
    }
    return 0;
}
