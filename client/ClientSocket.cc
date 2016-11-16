
#include "ClientSocket.h"

ClientSocket::ClientSocket(std::string host, int port) {
    if(!Socket::create())            { throw SocketException("Couldn't create client socket."); }
    if(!Socket::connect(host, port)) { throw SocketException("Couldn't connect."); }
}

const ClientSocket& ClientSocket::operator << (const std::string& s) const {
    if(!Socket::send(s))             { throw SocketException("Couldn't write to socket."); }
    return *this;
}

const ClientSocket& ClientSocket::operator >> (std::string& s) const {
    if(!Socket::recv(s))             { throw SocketException("Couldn't read."); }
    return *this;
}
