
#include "ServerSocket.h"
#include "SocketException.h"

ServerSocket::ServerSocket(int port) {
    if(!Socket::create())   { throw SocketException("Couldn't create server socket."); }
    if(!Socket::bind(port)) { throw SocketException("Couldn't bind to port."); }
    if(!Socket::listen())   { throw SocketException("Couldn't listen to server socket."); }
}

ServerSocket::~ServerSocket() {
}

const ServerSocket& ServerSocket::operator << (const std::string& s) const {
    if(!Socket::send(s))    { throw SocketException("Couldn't write to socket."); }
    return *this;
}

const ServerSocket& ServerSocket::operator >> (std::string& s) const {
    if(!Socket::recv(s))    { throw SocketException("Couldn't read."); }
    return *this;
}

void ServerSocket::accept(ServerSocket& sock) {
    if(!Socket::accept(sock)) { throw SocketException("Couldn't accept socket."); }
}
