#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#define CRLF "\r\n"
#define PORT 8080
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <cstring>
#include <string>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <map>
#include <sstream>
#include <ctime>
#include <fstream>
#include <csignal>

#include <cctype>

#include <set>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

#include "Response.hpp"
#include "Config.hpp"

#define MAX_CONNECTION 128

typedef int sockid_t;
// extern std::vector<Config> servers;


void error(std::string message);
void open_config_file(int argc, char *argv[]);

struct Connection {
    Socket sock;
    const Socket & parent;
    Request request;
    Response response;
    // Connection() {};
    Connection(const Socket & p) : parent(p) {};
    // Connection(const Connection & c) : sock(c.sock), parent(c.parent), request(c.request), response(c.response) {};
    ~Connection() {};
};

std::map<int, Connection *> connections;
// int sockfd;
// sockaddr_in address;
#endif