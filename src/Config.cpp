#include "Config.hpp"

std::vector<Config> servers;

Config::Config() {
    char *cwd = getcwd(NULL, 0);
    port = 80;
    host = "0.0.0.0";
    root = cwd;
    max_body_size = 1000000; // default 1m
    redirect.first = HttpStatus::None;
    listing = false;
    socket = NULL;
    upload = false;
    isPrimary = true;

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
    isPrimary = config.isPrimary;
}

const Config * getConnectionServerConfig(const std::string & hostname, const int port, const std::string & server_name) {
	const Config * default_server = NULL;

	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		if (it->host == hostname && it->port == port) {
			if (default_server == NULL) {
				default_server = &(*it);
				if (server_name.empty()) break;
			}
			if (it->server_name == server_name) {
				return &(*it);
			}
		}
	}
	return default_server;
}


Config::~Config() {
}
