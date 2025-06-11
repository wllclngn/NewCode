#ifndef SOCKET_INTERFACE_H
#define SOCKET_INTERFACE_H

class SocketInterface
{
public:
    virtual bool initialize() = 0;
    virtual void run() = 0;
    virtual void cleanup() = 0;
    virtual ~SocketInterface() {}
};

#endif
