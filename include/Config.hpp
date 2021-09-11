// #pragma once

#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include "Request.hpp"
#include "StatusCode.hpp"
#include <iostream>
#include <fstream>
#include <cctype>
#include <set>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>
#include <algorithm>
#include "Socket.hpp"

class Config {
public:
    Config();
    Config(const Config &);
    ~Config();
    int port;
    std::string host;

    std::string server_name;
    std::map<int, std::string> error_page;
    size_t max_body_size;    

    std::map<std::string, Config> location;

    std::vector<Method> methods;
    std::string root;
    std::pair<HttpStatus::StatusCode, std::string> redirect;
    bool listing;
    std::vector<std::string> index;
    std::string cgi;
    std::string upload;

    Socket *socket;
    std::string uri;
};

#endif