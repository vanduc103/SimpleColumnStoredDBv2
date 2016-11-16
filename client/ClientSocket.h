
#ifndef CLIENT_SOCKET_H
#define CLIENT_SOCKET_H

#include "Socket.h"
#include "SocketException.h"

class ClientSocket : private Socket {
public:
    ClientSocket(std::string host, int port);
    virtual ~ClientSocket() {};

    const ClientSocket& operator << (const std::string&) const;
    const ClientSocket& operator >> (std::string&) const;
};

#endif
