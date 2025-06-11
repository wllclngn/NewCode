#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/socket_client.h" // Use the concrete class

SocketClient client(0); // Will initialize with correct port in main

void sendStatus(const std::string &status)
{
    std::string message = "status:" + status;
    client.transmit(message);
    std::cout << "[Worker Status] " << status << std::endl;
}

void fork_and_run(const std::string &prog, const std::string &args)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        std::istringstream iss(args);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token)
        {
            tokens.push_back(token);
        }

        char *argv[tokens.size() + 2];
        argv[0] = const_cast<char *>(prog.c_str());
        for (size_t i = 0; i < tokens.size(); ++i)
        {
            argv[i + 1] = const_cast<char *>(tokens[i].c_str());
        }
        argv[tokens.size() + 1] = nullptr;

        execvp(prog.c_str(), argv);
        perror("execvp failed");
        exit(1);
    }
    else if (pid > 0)
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
    else
    {
        perror("fork failed");
    }
}

void handleMessage(const std::string &message)
{
    std::istringstream iss(message);
    std::string command;
    iss >> command;

    if (command == "map")
    {
        std::string args;
        std::getline(iss, args);
        if (!args.empty() && args[0] == ' ')
            args = args.substr(1); // Trim leading space

        sendStatus("job started");
        sendStatus("job processing");
        fork_and_run("./mapreduce", "mapper " + args);
        sendStatus("job completed");
    }
    else if (command == "reduce")
    {
        std::string args;
        std::getline(iss, args);
        if (!args.empty() && args[0] == ' ')
            args = args.substr(1); // Trim leading space

        sendStatus("job started");
        sendStatus("job processing");
        fork_and_run("./mapreduce", "reducer " + args);
        sendStatus("job completed");
    }
    else if (command == "heartbeat")
    {
        sendStatus("alive");
    }
    else
    {
        std::cout << "[Worker Message] " << message << ": unknown" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: worker_stub <controller_port>" << std::endl;
        return 1;
    }

    int controller_port = std::stoi(argv[1]);
    client = SocketClient(controller_port);

    if (!client.initialize())
    {
        std::cerr << "Failed to initialize worker socket client." << std::endl;
        return 1;
    }

    std::cout << "Worker connected to controller on port " << controller_port << std::endl;

    while (true)
    {
        std::string message = client.receive();
        if (message.empty())
        {
            std::cerr << "Connection lost. Exiting." << std::endl;
            break;
        }

        handleMessage(message);
    }

    client.cleanup();
    return 0;
}
