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
    _fd = ::socket(domain, type, protocol);
    if (_fd == -1) {
        error("Failed to create socket");
    }

}


void Socket::setState(TypeSocket type) const {
    if (fcntl(_fd, F_SETFL, type) == -1) {
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
        std::stringstream ss;

        ss << "Failed to bind to " << inet_ntoa(_address.sin_addr) << ":" << ntohs(_address.sin_port);
        error(ss.str());
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
        ::close(_fd);
    }
}

Socket Socket::accept() const {
    socklen_t addrlen = sizeof(_address);
    Socket sock;

	sock._fd = ::accept(_fd, (struct sockaddr *) (&sock._address), &addrlen);

    if (sock._fd == -1 && errno != EAGAIN) {
		error("Failed to grap connection");
	}

    return sock;
}

ssize_t Socket::recv(void *buf, size_t n) const {
    return ::recv(_fd, buf, n, 0);
}

void Socket::send(Response & res) const {
    if (res.buffer.length() > 0) {
        int bytes = ::send(_fd, res.buffer.data + res.buffer.pos, res.buffer.length(), 0);

        if (bytes > 0) {
            res.buffer.pos += bytes;
        } else {
            throw StatusCodeException(HttpStatus::None, NULL);
        }

        if (res.buffer.length() == 0 && res.isReadBodyFinished()) {
            res.setEndChunkSent(true);
        }
    }
}

void Socket::error(std::string message) const {
	std::cerr << message << ". " << std::strerror(errno)  << ". " << errno << std::endl;
	close();
	return exit(EXIT_FAILURE);
}

std::string Socket::getHost() const {
    return inet_ntoa(_address.sin_addr);
}

in_port_t Socket::getPort() const {
    return ntohs(_address.sin_port);
}
