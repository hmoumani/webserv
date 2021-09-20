#include "Request.hpp"

const size_t Request::max_size[] = {6, 2 * 1024, 8, 1 * 1024, 4 * 1024, 16 * 1024};
size_t Request::file_id = 0;

Method getMethodFromName(const std::string & method) {
    if (method == "GET") {
        return GET;
    } else if (method == "POST") {
        return POST;
    } else if (method == "DELETE") {
        return DELETE;
    }
    return UNKNOWN;
}

void Request::reset() {
    Message::reset();
    _parser.current_stat = _parser.METHOD;
    _parser.cr = false;

    _bparser.len = -1;
    _bparser.cr = false;
    _bparser.end = false;

}
Request::Request() {
    reset();
}
Request::Request(const Socket & connection) throw(StatusCodeException) {

    reset();

    size_t max_request_size = 16*1024;

    receive(connection);

    std::cerr << "Body: " << ((std::stringstream *)_body)->str() << "|" << std::endl;
}

void Request::receive(const Socket & connection) {
    ssize_t bytesRead;
    char buffer[BUFFER_SIZE];

    bytesRead = connection.recv(buffer, BUFFER_SIZE);
    write(2, buffer, bytesRead);
    parse(buffer, bytesRead);

}

static std::string & trim(std::string & str) {
    int first = str.find_first_not_of(" \n\r\t");
    int last = str.find_last_not_of(" \n\r\t");

    if (first == std::string::npos)
        first = 0;
    str = str.substr(first, last + 1);
    return str;
}

static const Config * getLocationFromRequest(const Request & req, const Config * server) {
	size_t len = 0;
	const Config * loc = server;
	if (server->location.find(Utils::getFileExtension(req.getFilename())) != server->location.end()) {
		loc = &server->location.find(Utils::getFileExtension(req.getFilename()))->second;
	} else {
		for (std::map<std::string, Config>::const_iterator it = server->location.begin(); it != server->location.end(); ++it) {
			const std::string path = getPathFromUri(req.getRequestTarget());
			if (it->first.length() <= path.length() && it->first.length() > len) {
				if (path.compare(0, it->first.length(), it->first.c_str()) == 0) {
					len = it->first.length();
					loc = &it->second;
				}
			}
		}
	}
	return loc;
}

const std::string getPathFromUri(const std::string & uri) {
	return uri.substr(0, uri.find_first_of('?'));
}

static const std::string getRequestedPath(const std::string & target, const Config * location) {
	const std::string path = getPathFromUri(target);
	struct stat buffer;

	std::string requested_path = location->root;

	requested_path += location->uri != "" ? path.substr(location->uri.length()) : path;
	// dout << "SUBSTR: " << location->uri << " " << location->uri.length() << " " << path.substr(location->uri.length()) << std::endl;

	std::cerr << "Requested File: " << requested_path << std::endl;
	if (requested_path[requested_path.length() - 1] != '/' && stat(requested_path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode)) {
		throw StatusCodeException(HttpStatus::MovedPermanently, path + '/', location);
	}

	return requested_path;
}

static const std::string getIndexFile(const Config * location, const std::string & filename, const std::string & req_taget)
{
	for (int i = 0; i < location->index.size(); ++i) {
		std::string file = (filename + location->index[i]);
		if (access(file.c_str(), F_OK))
			continue ;
		struct stat buffer;
		stat (file.c_str(), &buffer);
		if (!(buffer.st_mode & S_IROTH))
			throw StatusCodeException(HttpStatus::Forbidden, location);
		else
			return (file);
	}
	if (!location->listing)
		throw StatusCodeException(HttpStatus::Forbidden, location);
	throw ListingException(filename, req_taget);
}

void Request::checkRequestTarget() {
	_filename = getRequestedPath(_request_target, _server);
    struct stat fileStat;
    stat (_filename.c_str(), &fileStat);
	Utils::fileStat(_filename, fileStat, _server);
	if (S_ISDIR(fileStat.st_mode)) {
		_filename = getIndexFile(_server, _filename, _request_target);
	}
    _location = getLocationFromRequest(*this, _server);

}

bool Request::parse(const char * buff, size_t size) {
    size_t i;

    for (i = 0; i < size; i++) {
        if (_parser.cr && buff[i] == '\n') {
            if (_parser.current_stat != _parser.HTTP_VER && _parser.current_stat != _parser.HEADER_KEY && _parser.current_stat != _parser.HEADER_VALUE) {
                throw StatusCodeException(HttpStatus::BadRequest, _server);
            }
            if (_parser.current_stat == _parser.HEADER_KEY) {
                _parser.str.clear();
            }
        }
        if ((_parser.key.size() + _parser.str.size()) > max_size[_parser.current_stat]) {
            throw StatusCodeException(HttpStatus::BadRequest, _server);
        } else if (_parser.current_stat == _parser.METHOD && buff[i] == ' ') {
            _method = getMethodFromName(_parser.str);
            if (_method == UNKNOWN) {
                throw StatusCodeException(HttpStatus::BadRequest, _server);
            }
            _parser.current_stat = _parser.REQUEST_TARGET;
        } else if (_parser.current_stat == _parser.REQUEST_TARGET && buff[i] == ' ') {
            if (_parser.str.empty() || _parser.str[0] != '/') {
                throw StatusCodeException(HttpStatus::BadRequest, _server);
            }
            _request_target = _parser.str;
            _parser.current_stat = _parser.HTTP_VER;
        } else if (_parser.current_stat == _parser.HTTP_VER && _parser.cr && buff[i] == '\n') {
            _http_version = _parser.str;
            if (_http_version != "HTTP/1.1") {
                throw StatusCodeException(HttpStatus::BadRequest, _server);
            }
            _parser.current_stat = _parser.HEADER_KEY;
        } else if (_parser.current_stat == _parser.HEADER_KEY && buff[i] == ':') {
            _parser.key = trim(_parser.str);
            _parser.current_stat = _parser.HEADER_VALUE;
        } else if (_parser.current_stat == _parser.HEADER_VALUE && _parser.cr && buff[i] == '\n') {
            insert_header(_parser.key, trim(_parser.str));
            if (_parser.key == "Host") {
                _server = getConnectionServerConfig(_server->host, _server->port, getHeader("Host"));
            }
            _parser.current_stat = _parser.HEADER_KEY;
            _parser.key.clear();
        } else if (_parser.current_stat == _parser.HEADER_KEY && _parser.str.empty() && _parser.cr && buff[i] == '\n') {
            _parser.current_stat = _parser.BODY;
            if (_headers.find("Transfer-Encoding") == _headers.end() && _headers.find("Content-Length") == _headers.end()) {
                _bparser.end = true;
            }
            if (_headers.find("Host") == _headers.end()) {
                throw StatusCodeException(HttpStatus::BadRequest, _server);
            }
            checkRequestTarget();
        } else if (_parser.current_stat == _parser.BODY) {
            size_t s = receiveBody(buff + i, size - i);
            i += s;
            if (s == 0) {
                return true;
            }
        } else {
            if (buff[i] == '\r')
                _parser.cr = true;
            else {
                _parser.str += buff[i];
                _parser.cr = false;
            }
            continue;
        }
        _parser.cr = false;
        _parser.str.clear();
    }
    if (_parser.str.size() > max_size[_parser.current_stat]) {
        throw StatusCodeException(HttpStatus::BadRequest, _server);
    }
    return false;
}

bool isValidHex(const std::string hex) {
    for (int i = 0; i < hex.length(); ++i) {
        if (std::string("0123456789ABCDEFabcdef").find(hex[i]) == std::string::npos) {
            return false;
        }
    }
    return true;
}

bool isValidDecimal(const std::string decimal) {
    for (int i = 0; i < decimal.length(); ++i) {
        if (std::string("0123456789").find(decimal[i]) == std::string::npos) {
            return false;
        }
    }
    return true;
}

size_t Request::receiveBody(const char * buff, size_t size) {
    int i = 0;

    if (_headers.find("Transfer-Encoding") != _headers.end() && _headers["Transfer-Encoding"] == "chunked") {
        for (i = 0; i < size; ++i) {
            if (_bparser.len > 0) {
                size_t write_len = std::min((size_t)_bparser.len, size - i);
                _body->write(buff + i, write_len);
                _bparser.len -= write_len;
                i += write_len - 1;
            } else if (_bparser.len == 0 && _bparser.cr && buff[i] == '\n') {
                if (_bparser.end) {
                    return 0;
                }
                _bparser.len = -1;
            } else if (_bparser.len == -1) {
                if (_bparser.cr && buff[i] == '\n') {
                    std::stringstream ss;
                    size_t last = _bparser.str.find_first_of(';');
                    last = last == std::string::npos ? _bparser.str.length() : last;
                    _bparser.str = _bparser.str.substr(0, last);
                    if (!isValidHex(_bparser.str)) {
                        throw StatusCodeException(HttpStatus::BadRequest, _server);
                    }
                    ss << std::hex << _bparser.str;
                    ss >> _bparser.len;
                    _bparser.str.clear();
                    
                    _bparser.end = _bparser.len == 0;

                    if (!_isBodyFile && (_bparser.len + _body->tellp()) > max_size[_parser.BODY]) {
                        openBodyFile();
                    }

                    std::cerr << "chunked size: " << _bparser.len << std::endl;
                } else if (buff[i] != '\r') {
                    _bparser.str += buff[i];
                }
            }
            if (buff[i] == '\r') _bparser.cr = true; else _bparser.cr = false;
        }
    } else if (_headers.find("Content-Length") != _headers.end()) {
        if (isValidDecimal(_headers["Content-Length"])) {
            if (_bparser.len == -1) {
                _bparser.len = std::atoi(_headers["Content-Length"].c_str());
                if (_bparser.len > max_size[_parser.BODY]) {
                    openBodyFile();
                }
            }
            size_t write_len = std::min((size_t)_bparser.len, size);
            _body->write(buff + i, write_len);
            _bparser.len -= write_len;
            i += write_len - 1;
            if (_bparser.len == 0) {
                _bparser.end = true;
                return 0;
            }
        } else {
            throw StatusCodeException(HttpStatus::BadRequest, _server);
        }
    } else {
        _bparser.end = true;
    }
    return i;
}

void Request::openBodyFile() {
    std::stringstream * body_ss = (std::stringstream *)_body;
    std::stringstream filename;
    filename << "/tmp/" << std::setfill('0') << std::setw(10) << file_id++;
    std::cerr << "body filename: " << filename.str() << std::endl;
    std::cerr << "Body: " << body_ss->str() << std::endl;
    _body = new std::fstream(filename.str().c_str(), std::fstream::in | std::fstream::out | std::fstream::trunc);
    // _body->write("Hello", 5);
    
    _body->write(body_ss->str().c_str(), body_ss->str().length());
    _isBodyFile = true;
    delete body_ss;
}

bool Request::isFinished() const {
    return _bparser.end;
}

// Request::Request(const std::string & message) throw(StatusCodeException) : Message(message) {
//     size_t start = 0;
//     size_t end;

//     for (int i = 0; i < 3; ++i) {
//         end = _start_line.find_first_of(' ', start);
//         std::string token = _start_line.substr(start, end - start);
//         start = _start_line.find_first_not_of(' ', end);
//         if (i == 0) {
//             _method = getMethodFromName(token);
//         } else if (i == 1) {
//             _request_target = token;
//         } else {
//             _http_version = token;
//         }
//     }

//     if (_request_target.empty() || _http_version != "HTTP/1.1") {
//         throw StatusCodeException(HttpStatus::BadRequest);
//     }
// }

Request::~Request() {
}

const Method Request::getMethod() const {
    return this->_method;
}

const std::string & Request::getRequestTarget() const {
    return this->_request_target;
}

const std::string & Request::getHTTPVersion() const {
    return this->_http_version;
}

const std::string & Request::getFilename() const {
    return _filename;
}
