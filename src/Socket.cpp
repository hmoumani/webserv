#include "Socket.hpp"

Socket::Socket() : _fd (-1) {
    memset(&_address, 0, sizeof(_address));
}

Socket::Socket(int domain, int type, int protocol) {
    create(domain, type, protocol);
}

Socket::~Socket() {

}

void Socket::create(int domain, int type, int protocol) {
    std::cout << "Init Socket" << std::endl;
    _fd = ::socket(domain, type, protocol);
    std::cout << "SOCKET: " << _fd << std::endl;
    if (_fd == -1) {
        error("Failed to create socket");
    }

}


void Socket::setState(TypeSocket type) const {
    int flags = fcntl(_fd, F_GETFL, 0);

    if (flags == -1) {
        error("Failed to get socket state");
    }

    flags = type ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);

    if (fcntl(_fd, F_SETFL, flags) == -1) {
        error("Failed to change socket state");
    }
}

void Socket::setOption(int level, int option_name, const void *option_value, socklen_t option_len) const {
    if (setsockopt(_fd, level, option_name, option_value, option_len)) {
        error("Failed to change socket options");
	}
}

void Socket::setAddress(sa_family_t family, in_addr_t s_addr, in_port_t sin_port) {
    _address.sin_family = family;
    _address.sin_addr.s_addr = s_addr;
    _address.sin_port = htons(sin_port);
}

void Socket::bind() const {
	if (::bind(_fd, (struct sockaddr *)&_address, sizeof(_address)) == -1) {
        error("Failed to bind to port");
	}
}

void Socket::listen(int backlog) const {
	if (::listen(_fd, backlog) == -1) {
        error("Failed to listen on socket");
	}
}

int Socket::getFD() const {
    return _fd;
}

void Socket::setFD(int fd) {
    _fd = fd;
}

const sockaddr_in & Socket::getAddress() const {
    return _address;
}

void Socket::close() const {
    if (_fd != -1) {
        std::cout << "Close Socket" << std::endl;
        ::close(_fd);
    }
}

Socket Socket::accept() const {
    socklen_t addrlen = sizeof(_address);
    Socket sock;

	sock._fd = ::accept(_fd, (sockaddr *) & _address, &addrlen);

    if (sock._fd == -1 && errno != EAGAIN) {
		error("Failed to grap connection");
	}

    return sock;
}

std::string Socket::receive() const {
    char buffer[BUFFER_SIZE + 1];
    std::string message;
    int bytesRead;

    while ((bytesRead = recv(_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytesRead] = 0;
        message += buffer;
    }

    return message;
}

void Socket::send(std::vector<char> vec) const {
    int bytes = ::send(_fd, vec.data(), vec.size(), 0);
    if (bytes == -1) {
        error("Failed to send response");
    }
}

void Socket::send(std::string message) const {
    int bytes = ::send(_fd, message.c_str(), message.length(), 0);

    if (bytes == -1) {
        error("Failed to send response");
    }
}

void Socket::error(std::string message) const {
	std::cerr << message << ". " << std::strerror(errno) << std::endl;
	close();
	return exit(EXIT_FAILURE);
}
