#ifndef SOCKET_HPP
# define SOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sstream>
#include "Buffer.hpp"
#include "Response.hpp"

class Response;

enum TypeSocket {BlockingSocket, NonBlockingSocket};

class Socket {
private:
    int _fd;
    sockaddr_in _address;

public:
    Socket();
    Socket(int domain, int type, int protocol);
    ~Socket();

    void create(int domain, int type, int protocol);

    int getFD() const;
    const sockaddr_in & getAddress() const;

    void setFD(int fd);
    void setState(TypeSocket type) const;
    void setOption(int level, int option_name, const void *option_value, socklen_t option_len) const;
    void setAddress(sa_family_t family, in_addr_t s_addr, in_port_t sin_port);
    void bind() const;
    void listen(int backlog) const;
    void close() const;
    void error(std::string message) const;
    Socket accept() const;
    // std::string receive() const;
    ssize_t recv(void *buf, size_t n) const;

    void send(Response & response) const;
    std::string getHost() const;
    in_port_t getPort() const;

    // void send(std::vector<char> vec) const;

};

#endif