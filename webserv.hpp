#ifndef WEBSERV_HPP
# define WEBSERV_HPP
# include <iostream>
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include <fcntl.h>
#include <sstream>
# define PORT 8080
int posix_fadvise(int fd, off_t offset, off_t len, int advice);
#endif