#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#define SERVER_NAME "WebServer (MacOs)"
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

#include "Socket.hpp"
// #include "Message.hpp"
// #include "Request.hpp"
// #include "MimeTypes.hpp"
#include "Config.hpp"
#include "Response.hpp"
// #include "Utils.hpp"
#include <dirent.h>
typedef int sockid_t;
extern std::vector<Config> servers;

void error(std::string message);
void open_config_file(int argc, char *argv[]);


// int sockfd;
// sockaddr_in address;
#endif