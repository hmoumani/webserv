#pragma once

#include <string>
#include <vector>
#include <map>

class Config {
public:

    std::vector<std::string> index;
    int PORT;
    std::string root;
    std::string server_name;
    std::string host;
    std::vector<Config> sever;
    std::vector<Config> location;
    bool directory_listing;
    bool upload;
    std::string upload_path;
    std::string cgi;
    std::map<int, std::string> error_page;
    size_t limit_body_size;    
};
