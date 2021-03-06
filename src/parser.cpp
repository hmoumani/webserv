#include "Config.hpp"
#include "Request.hpp"

static Config * curr_server = NULL;
static Config * curr_location = NULL;

static bool duplicate_listen = false;

enum ParseError {
    UNEXPECTED_SYMBOL, UNEXPECTED_EOF, UNKNOWN_DIRECTIVE, DIRECTIVE_NOT_ALLOWED,
    DIRECTIVE_HAS_NO_OPENING, DIRECTIVE_NOT_TERMINATED, INVALID_ARGUMENTS, DUPLICATE_LISTEN,
    DUPLICATE_SERVER_NAME, INVALID_PORT, INVALID_ERROR_CODE, INVALID_STATUS_CODE,
    INVALID_METHOD, DUPLICATE_METHOD, DUPLICATE_LOCATION
};

static const size_t NUM_DIRECTIVES = 13;
static const std::string directives[] = {
    "server", "location", "listen", "server_name", // Server directives
    "error_page", "max_body_size", "root", "listing", "index", "http_method",  // Server & Location directives
    "redirect", "cgi_pass", "upload_pass"};  // Location directives

static const size_t server_index = 4;
static const size_t location_index = 10;

static void error(ParseError err, const std::string arg) {
    if (err == UNEXPECTED_SYMBOL) {
        std::cerr << "webserv: unexpected \"" << arg << "\"" << std::endl; 
    } else if (err == UNEXPECTED_EOF) {
        std::cerr << "webserv: unexpected end of file, expecting \"" << arg << "\"" << std::endl;
    } else if (err == UNKNOWN_DIRECTIVE) {
        std::cerr << "webserv: unknown directive \"" << arg << "\" " << std::endl;
    } else if (err == DIRECTIVE_NOT_ALLOWED) {
        std::cerr << "webserv: \"" << arg << "\" directive is not allowed here" << std::endl;
    } else if (err == DIRECTIVE_HAS_NO_OPENING) {
        std::cerr << "webserv: directive \"" << arg << "\" has no opening \"{\" " << std::endl;
    } else if (err == DIRECTIVE_NOT_TERMINATED) {
        std::cerr << "webserv: directive \"" << arg << "\" is not terminated by \";\"" << std::endl;
    } else if (err == INVALID_ARGUMENTS) {
        std::cerr << "webserv: invalid number of arguments in \"" << arg << "\" directive" << std::endl;
    } else if (err == INVALID_PORT) {
        std::cerr << "webserv: invalid port \"" << arg << "\"" << std::endl;
    } else if (err == INVALID_ERROR_CODE) {
        std::cerr << "webserv: invalid error code \"" << arg << "\"" << std::endl;
    } else if (err == INVALID_STATUS_CODE) {
        std::cerr << "webserv: invalid status code \"" << arg << "\"" << std::endl;
    } else if (err == INVALID_METHOD) {
        std::cerr << "webserv: invalid method \"" << arg << "\"" << std::endl;
    } else if (err == DUPLICATE_METHOD) {
        std::cerr << "webserv: duplicate method \"" << arg << "\"" << std::endl;
    } else if (err == DUPLICATE_LOCATION) {
        std::cerr << "webserv: duplicate location \"" << arg << "\"" << std::endl;
    } else if (err == DUPLICATE_SERVER_NAME) {
        std::cerr << "webserv: conflicting server name \"" << arg << "\"" << std::endl;
    } else if (err == DUPLICATE_LISTEN) {
        std::cerr << "webserv: a duplicate listen \"" << arg << "\"" << std::endl;
    }
    exit(EXIT_FAILURE);
}

static bool isServer(const std::string & directive) {
    for (size_t i = 1; i < location_index; ++i) {
        if (directive == directives[i]) {
            return true;
        }
    }
    return false;
}

static bool isLocation(const std::string & directive) {
    for (size_t i = server_index; i < NUM_DIRECTIVES; ++i) {
        if (directive == directives[i]) {
            return true;
        }
    }
    return false;
}

static bool isDirective(const std::string & dir) {
    for (size_t i = 0; i < NUM_DIRECTIVES; ++i) {
        if (dir == directives[i]) {
            return true;
        }
    }
    return false;
}

static bool isDelimiter(char c) {
    if (c == '{' || c == '}' || c == ';') {
        return true;
    }
    return false;
}

static std::string getToken(std::ifstream & file) {
    char c;
    bool isWord = false;
    std::string token;
    while ((c = file.get()) != EOF) {
        if (!isspace(c)) {
            token.push_back(c);
            isWord = true;
            if (isDelimiter(c)) {
                break;
            }
        } else if (isWord) {
            break;
        }

        if (isWord && (c = file.peek()) != EOF && isDelimiter(c)) {
            break;
        }
    }
    return token;
}

static size_t convertSize(const std::string & str) {
    size_t len = str.length();
    std::string measures = "kmg";
    size_t size = std::atol(str.c_str());
    char measure = std::tolower(str[len - 1]);
    if (isdigit(str[0]) && (isdigit(measure) || measures.find(measure) != std::string::npos)) {
        for (size_t i = 0; i < len - 1; ++i) {
            if (!isdigit(str[i])) {
                return std::string::npos;
            }
        }
    } else {
        return std::string::npos;
    }

    if (measure == 'g') {
        size *= 1000*1000*1000;
    } else if (measure == 'm') {
        size *= 1000*1000;
    } else if (measure == 'k') {
        size *= 1000;
    }
    return size;
}

static bool isnumber(const std::string & number) {
    for (size_t i = 0; i < number.length(); ++i) {
        if (!std::isdigit(number[i])) {
            return false;
        }
    }
    return true;
}

static std::vector<std::string> getDirective(std::ifstream & file) {
    
    std::vector<std::string> directive;
    std::string token;

    while ((token = getToken(file)) != "") {
        directive.push_back(token);
        if (token == "{" || token == "}" || token == ";") {
            break;
        }
    }
    return (directive);
}

static void checkError(std::vector<std::string> & directive) {
    if (isDelimiter(directive[0][0]) || directive.back() == "}") {
        if ((curr_location || curr_server) && directive[0] == "}") {
            return;
        }
        error(UNEXPECTED_SYMBOL, directive[0]);
    } else if (!isDirective(directive[0])) {
        error(UNKNOWN_DIRECTIVE, directive[0]);
    } else if ((curr_location && !isLocation(directive[0])) || (curr_server && !curr_location && !isServer(directive[0])) || (directive[0] != "server" && !curr_server)) {
        error(DIRECTIVE_NOT_ALLOWED, directive[0]);
    } else if (!isDelimiter(directive.back()[0])) {
        error(UNEXPECTED_EOF, directive[0]);
    } else if ((directive[0] == "server" || directive[0] == "location") && directive.back() != "{") {
        error(DIRECTIVE_HAS_NO_OPENING, directive[0]);
    } else if ((directive[0] != "server" && directive[0] != "location" && directive.back() != ";")) {
        error(DIRECTIVE_NOT_TERMINATED, directive[0]);
    }
}

void conflictingServerName() {
    if (curr_server) {
        for (std::vector<Config>::const_iterator it = servers.begin(); &(*it) != curr_server; ++it) {
            if (it->host == curr_server->host && it->port == curr_server->port && it->server_name == curr_server->server_name) {
                std::cerr << "webserv: conflicting server name '" << it->server_name << "' on " << it->host << ":" << it->port << ", ignored" << std::endl;
                servers.pop_back();
            }
        }
    }
}

static void parse(std::ifstream & file) {
    std::vector<std::string> directive;
    
    while (!(directive = getDirective(file)).empty()) {
        checkError(directive);
        Config * config = curr_location ? curr_location : curr_server;

        if (directive[0] == "http_method" && directive.size() > 2) {
            config->methods.clear();
            for (size_t i = 1; i < directive.size() - 1; ++i) {
                Method method = getMethodFromName(directive[i]);
                if (method == UNKNOWN) {
                    error(INVALID_METHOD, directive[i]);
                } else if (std::find(config->methods.begin(), config->methods.end(), method) != config->methods.end()) {
                    error(DUPLICATE_METHOD, directive[i]);
                }
                config->methods.insert(method);
            }
        } else if (directive[0] == "index" && directive.size() > 2) {
            config->index.clear();
            for (size_t i = 1; i < directive.size() - 1; ++i) {
                config->index.push_back(directive[i]);
            }
        } else if (directive[0] == "error_page" && directive.size() > 3) {
            for (size_t i = 1; i < directive.size() - 2; ++i) {
                HttpStatus::StatusCode code = (HttpStatus::StatusCode) std::atoi(directive[i].c_str());
                if (!isnumber(directive[i]) || !isVadilCode(code)) {
                    error(INVALID_ERROR_CODE, directive[i]);
                }
                config->error_page.insert(make_pair(code, directive[directive.size() - 2]));
            }
        } else if (directive.size() == 1 && directive[0] == "}") {
            if (curr_location) {
                curr_location = NULL;
            } else if (curr_server) {
                conflictingServerName(); // ignore the server if its already exists
                curr_server = NULL;
                duplicate_listen = false;
            }
        } else if (directive.size() == 2) {
            if (directive[0] == "server") {
                servers.push_back(Config());
                curr_server = &servers.back();
            } else {
                error(INVALID_ARGUMENTS, directive[0]);
            }
        } else if ((directive.size() == 3 || directive.size() == 4) && directive[0] == "redirect") {
            HttpStatus::StatusCode code = (HttpStatus::StatusCode) std::atoi(directive[1].c_str());
            if (!isnumber(directive[1]) || !isVadilCode(code)) {
                error(INVALID_STATUS_CODE, directive[1]);
            }
            config->redirect.first = code;
            config->redirect.second = directive.size() == 4 ? directive[2] : "";
        } else if (directive.size() == 3) {
            if (directive[0] == "location") {
                if (curr_server->location.find(directive[1]) == curr_server->location.end()) {
                    curr_server->location.insert(make_pair(directive[1], Config(*curr_server)));
                    curr_location = &curr_server->location[directive[1]];
                    curr_location->uri = directive[1];
                    curr_location->error_page.clear();
                } else {
                    error(DUPLICATE_LOCATION, directive[1]);
                }
            } else if (directive[0] == "server_name") {
                config->server_name = directive[1];
            } else if (directive[0] == "root") {
                config->root = directive[1];
            } else if (directive[0] == "listing") {
                config->listing = directive[1] == "on";
            } else if (directive[0] == "max_body_size") {
                config->max_body_size = convertSize(directive[1]);
            } else if (directive[0] == "upload_pass") {
                config->upload = directive[1] == "on";
            } else if (directive[0] == "cgi_pass") {
                config->cgi = directive[1];
            } else {
                error(INVALID_ARGUMENTS, directive[0]);
            }
        } else if (directive.size() == 4) {
            if (directive[0] == "listen") {
                std::string host = directive[1] == "localhost" ? "127.0.0.1" : directive[1];

                if (duplicate_listen) {
                    if (config->host == host && config->port == std::atoi(directive[2].c_str())) {
                        error(DUPLICATE_LISTEN, host + ":" + directive[2]);
                    }
                } else {
                    config->host = host;
                    if (!isnumber(directive[2])) {
                        error(INVALID_PORT, directive[2]);
                    }
                    config->port = std::atoi(directive[2].c_str());
                    duplicate_listen = true;
                }
            } else {
                error(INVALID_ARGUMENTS, directive[0]);
            }
        } else {
            error(INVALID_ARGUMENTS, directive[0]);
        }
    }
    if (curr_server || curr_location) {
        error(UNEXPECTED_EOF, "}");
    }
}


void open_config_file(int argc, char *argv[]) {
    std::ifstream file;

    if (argc == 2) {
        file.open(argv[1]);
        if (file.is_open()) {
            parse(file);
            file.close();
            if (servers.empty()) {
                std::cerr << argv[0] << ": there is no server!" << std::endl;
                exit(EXIT_FAILURE);
            }
        } else {
            std::cerr << argv[0] << ": open(): " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        std::cerr << argv[0] << ": Wrong number of arguments!" << std::endl;
        exit(EXIT_FAILURE);
    }
}
