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
#include <sstream>
#include <ctime>
#include <fstream>
#include "Socket.hpp"
#include "Message.hpp"
#include "Request.hpp"
#include "MimeTypes.hpp"

typedef int sockid_t;

static const std::string wday_name[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const std::string mon_name[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
#include "Response.hpp"
#include "Utils.hpp"
// int sockfd;
// sockaddr_in address;
#endif