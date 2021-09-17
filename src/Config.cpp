#include "Config.hpp"

Config::Config() {
    char *cwd = getcwd(NULL, 0);
    port = 80;
    host = "0.0.0.0";
    root = cwd;
    max_body_size = 1000000; // default 1m
    redirect.first = static_cast<HttpStatus::StatusCode>(0);
    listing = false;
    socket = NULL;
    free(cwd);
}

Config::Config(const Config & config) {
    port = config.port;
    host = config.host;
    server_name = config.server_name;
    error_page = config.error_page;
    max_body_size = config.max_body_size;
    location = config.location;
    methods = config.methods;
    root = config.root;
    redirect = config.redirect;
    listing = config.listing;
    index = config.index;
    cgi = config.cgi;
    upload = config.upload;
    socket = config.socket;
    uri = config.uri;
}

Config::~Config() {
}
