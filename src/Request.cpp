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
    _parser.str = "";
    _parser.key = "";
    _parser.end = false;
    _parser.buff.resize(0);

    _bparser.len = -1;
    _bparser.cr = false;
    _bparser.end = false;
    _bparser.str = "";

    _filename = "";
    _request_target = "";
    _http_version = "";

    _upload = false;
    // debug << "RESET" << std::endl;
}
Request::Request() {
    reset();
}

// Request::Request(const Socket & connection) throw(StatusCodeException) {

//     reset();

//     size_t max_request_size = 16*1024;

//     receive(connection);

//     debug << "Body: " << ((std::stringstream *)_body)->str() << "|" << std::endl;
// }

// Request::Request(const Request & req) {
//     *this = req;
// }

std::string & trim(std::string & str) {
    size_t first = str.find_first_not_of(" \n\r\t");
    size_t last = str.find_last_not_of(" \n\r\t");

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
	// const std::string path = getPathFromUri(target);
    // debug << "Path: " << target << std::endl;

	std::string requested_path = "";
    std::string file_path = location->root;
	requested_path += "/" + target.substr(location->uri.length());
    // debug << "requested_path: " << location->uri << std::endl;
	// debug << "SUBSTR: " << location->uri << " " << location->uri.length() << " " << path.substr(location->uri.length()) << std::endl;
    file_path += requested_path;
	// debug << "Target: " << target << ", Requested File: " << requested_path << std::endl;
    // std::string 
	// if (stat((file_path).c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode) && method != DELETE) {
    //     if (file_path[file_path.length() - 1] != '/') {
	// 	    throw StatusCodeException(HttpStatus::MovedPermanently, target + '/', location);
    //     // } else {

    //     }
	// }

	return requested_path;
}

static const std::string getIndexFile(const Config * location, const std::string & filename, const std::string & req_taget)
{
	for (size_t i = 0; i < location->index.size(); ++i) {
		std::string file = (filename + location->index[i]);
		if (access(file.c_str(), F_OK))
			continue ;
		struct stat buffer;
		stat (file.c_str(), &buffer);
		if (!(buffer.st_mode & S_IROTH))
			throw StatusCodeException(HttpStatus::Forbidden, location);
		else
			return (location->index[i]);
	}
	if (!location->listing)
		throw StatusCodeException(HttpStatus::Forbidden, location);
	throw ListingException(filename, req_taget);
}

void Request::updateLocation() {
    _location = getLocationFromRequest(*this, _server);

	_filename = getRequestedPath(getPathFromUri(_request_target), _location);
    _file_path = _location->root + _filename;
    _location = getLocationFromRequest(*this, _server);
}

void Request::checkRequestTarget() {

    if (_location->redirect.first != HttpStatus::None) {
		if (HttpStatus::isRedirection(_location->redirect.first)) {
			throw StatusCodeException(_location->redirect.first, _location->redirect.second, _location);
		} else {
			throw StatusCodeException(_location->redirect.first, _location);
		}
	}
    struct stat fileStat;
    stat (_file_path.c_str(), &fileStat);

    std::string target = getPathFromUri(_request_target);
    if (stat((_file_path).c_str(), &fileStat) == 0 && S_ISDIR(fileStat.st_mode) && _method != DELETE) {
        if (_file_path[_file_path.length() - 1] != '/') {
		    throw StatusCodeException(HttpStatus::MovedPermanently, target + '/', _location);
        }
	}

    if (_method != DELETE) {
	    Utils::fileStat(_file_path, fileStat, _location);
    }

	if (S_ISDIR(fileStat.st_mode) && _method != DELETE && !_location->upload) {
		_filename += getIndexFile(_location, _file_path, _request_target);
        _file_path = _location->root + _filename.substr(1);
        // debug << "Filename: " << _filename << std::endl;
	}
    if (_method != DELETE) {
	    Utils::fileStat(_file_path, fileStat, _location);
    }
    _location = getLocationFromRequest(*this, _server);

}

void Request::receive(const Socket & connection) {
    ssize_t bytesRead;
    char buffer[BUFFER_SIZE];

    // _bparser.end = false;
    if (_parser.buff.length() == 0) {
        _parser.buff.resize(BUFFER_SIZE);
        bytesRead = connection.recv(buffer, BUFFER_SIZE);
        if (bytesRead == 0) {
            _parser.end = false;
            _parser.buff.resize(0);
            return;
        } else if (bytesRead == -1) {
            throw StatusCodeException(HttpStatus::None, NULL);
        }

        _parser.buff.setData(buffer, bytesRead);

        // write(2, buffer, bytesRead);
        // debug << connection.getHost() << ":" << connection.getPort() << std::endl;
    }
    parse();
    if (_bparser.end) {
        _body_size = _body->tellp();
        if (_location->upload) {
            // _parser.end = true;
            throw StatusCodeException(HttpStatus::Created, _location);
        }
    }
}

bool Request::parse() {
    // size_t i = _parser.buff.pos;
    // size_t size = _parser.buff.size;
    // char * buff = _parser.buff.data;
    bool end = false;
    char c;
    while (_parser.buff.length()) {
        if (_parser.current_stat != _parser.BODY)
            c = _parser.buff.getc();
        if (_parser.cr && c == '\n') {
            if (_parser.current_stat != _parser.HTTP_VER && _parser.current_stat != _parser.HEADER_KEY && _parser.current_stat != _parser.HEADER_VALUE && _parser.current_stat != _parser.BODY) {
                throw StatusCodeException(HttpStatus::BadRequest, _server);
            }
            if (_parser.current_stat == _parser.HEADER_KEY) {
                _parser.str.clear();
            }
        }
        if (_parser.buff.length() == 0 && _parser.buff.size != BUFFER_SIZE) {
            end = true;
        }
        if ((_parser.key.size() + _parser.str.size()) > max_size[_parser.current_stat]) {
            throw StatusCodeException(HttpStatus::BadRequest, _server);
        } else if ((_parser.current_stat == _parser.METHOD && (c == ' ' || end))) {
            _method = getMethodFromName(_parser.str);
            if (_method == UNKNOWN) {
                throw StatusCodeException(HttpStatus::NotImplemented, _server);
            }
            _parser.current_stat = _parser.REQUEST_TARGET;
        } else if ((_parser.current_stat == _parser.REQUEST_TARGET && (c == ' ' || end))) {
            if (_parser.str.empty() || _parser.str[0] != '/') {
                throw StatusCodeException(HttpStatus::BadRequest, _server);
            }
            _request_target = _parser.str;
            _parser.current_stat = _parser.HTTP_VER;
        } else if ((_parser.current_stat == _parser.HTTP_VER && ((_parser.cr && c == '\n') || end))) {
            _http_version = _parser.str;
            if (_http_version != "HTTP/1.1") {
                throw StatusCodeException(HttpStatus::BadRequest, _server);
            }
            _parser.current_stat = _parser.HEADER_KEY;
        } else if ((_parser.current_stat == _parser.HEADER_KEY && _parser.str.empty() && ((_parser.cr && c == '\n') || end))) {
            _parser.current_stat = _parser.BODY;
            if (_headers.find("Host") == _headers.end()) {
                throw StatusCodeException(HttpStatus::BadRequest, _server);
            }
            updateLocation();
            if (_headers.find("Content-Length") != _headers.end()) {
                _bparser.len = std::atol(getHeader("Content-Length").c_str());
                if (_bparser.len == 0) {
                    _bparser.end = true;
                }
                if ((unsigned long)_bparser.len > _location->max_body_size) {
                    throw StatusCodeException(HttpStatus::PayloadTooLarge, _location);
                }
            } else if (_headers.find("Transfer-Encoding") == _headers.end()) {
                _bparser.end = true;
            }
            checkRequestTarget();
            if (_location->methods.find(_method) == _location->methods.end()) {
                throw StatusCodeException(HttpStatus::MethodNotAllowed, _location);
            }
            if ((_location->upload && _method == POST) || _bparser.len > (ssize_t)max_size[_parser.BODY]) {
                openBodyFile();
            }
            if (!_location->upload)
                _parser.end = true;
        } else if ((_parser.current_stat == _parser.HEADER_KEY && (c == ':' || end))) {
            _parser.key = trim(_parser.str);
            _parser.current_stat = _parser.HEADER_VALUE;
        } else if ((_parser.current_stat == _parser.HEADER_VALUE && ((_parser.cr && c == '\n') || end))) {
            insert_header(_parser.key, trim(_parser.str));
            if (_parser.key == "Host") {
                _server = getConnectionServerConfig(_server->host, _server->port, getHeader("Host"));
            }
            _parser.current_stat = _parser.HEADER_KEY;
            _parser.key.clear();
        } else if (_parser.current_stat != _parser.BODY) {
            if (c == '\r')
                _parser.cr = true;
            else {
                _parser.str += c;
                _parser.cr = false;
            }
            continue;
        }
        if (_parser.current_stat == _parser.BODY) {
            receiveBody();
            // i += s;
            // if (s == 0) {
            break;
            // }
        } 
        _parser.cr = false;
        _parser.str.clear();
    }
    if (_parser.current_stat != _parser.BODY && _parser.str.size() > max_size[_parser.current_stat]) {
        throw StatusCodeException(HttpStatus::BadRequest, _server);
    }
    return false;
}

bool isValidHex(const std::string hex) {
    for (size_t i = 0; i < hex.length(); ++i) {
        if (std::string("0123456789ABCDEFabcdef").find(hex[i]) == std::string::npos) {
            return false;
        }
    }
    return true;
}

bool isValidDecimal(const std::string decimal) {
    for (size_t i = 0; i < decimal.length(); ++i) {
        if (std::string("0123456789").find(decimal[i]) == std::string::npos) {
            return false;
        }
    }
    return true;
}

size_t Request::receiveBody() {
    // int i = 0;
    char c = -1;
    if (_headers.find("Transfer-Encoding") != _headers.end() && getHeader("Transfer-Encoding") == "chunked") {
        while (_parser.buff.length()) {
            if (_bparser.len > 0) {
                size_t write_len = std::min((size_t)_bparser.len, _parser.buff.length());
                _body->write(_parser.buff.data + _parser.buff.pos, write_len);
                _parser.buff.pos += write_len - 1;
                _bparser.len -= write_len;
                // i += write_len - 1;
            } else {
                c = _parser.buff.getc();
                if (_bparser.len == 0 && _bparser.cr && c == '\n') {
                    if (_bparser.end) {
                        return 0;
                    }
                    _bparser.len = -1;
                } else if (_bparser.len == -1) {
                    if (_bparser.cr && c == '\n') {
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

                        if (_bparser.len + _body->tellp() > (long long)_location->max_body_size) {
                            throw StatusCodeException(HttpStatus::PayloadTooLarge, _location);
                        }
                        if ((_location->upload && _method == POST) || (!_isBodyFile && (_bparser.len + _body->tellp()) > (long long)max_size[_parser.BODY])) {
                            openBodyFile();
                        }

                    } else if (c != '\r') {
                        _bparser.str += c;
                    }
                }
            }
            if (c == '\r') _bparser.cr = true; else _bparser.cr = false;
        }
    } else if (_headers.find("Content-Length") != _headers.end()) {
        if (isValidDecimal(getHeader("Content-Length"))) {

            size_t write_len = std::min((size_t)_bparser.len, _parser.buff.length());
            _body->write(_parser.buff.data + _parser.buff.pos, write_len);
            _parser.buff.pos += write_len;

            _bparser.len -= write_len;
            if (_bparser.len == 0) {
                _bparser.end = true;
            }
        } else {
            throw StatusCodeException(HttpStatus::BadRequest, _server);
        }
    } else {
        _bparser.end = true;
    }
    return 0;
}

void Request::openBodyFile() {
    std::stringstream * body_ss = (std::stringstream *)_body;
    std::stringstream filename;

    if (_location->upload && _method == POST) {
        filename << _file_path;
        _upload = true;
    } else {
        filename << "/tmp/" << std::setfill('0') << std::setw(10) << file_id++;
    }

    _body = new std::fstream(filename.str().c_str(), std::fstream::in | std::fstream::out | std::fstream::trunc);

    if (_body->fail()) {
        perror("fstream()");
        if (errno == EACCES || errno == EPERM) {
            throw StatusCodeException(HttpStatus::Forbidden, _location);
        } else if (errno == ENOENT || errno == ENAMETOOLONG || errno == EFAULT) {
            throw StatusCodeException(HttpStatus::NotFound, _location);
        } else if (errno == ENOTDIR || errno == EISDIR) {
            throw StatusCodeException(HttpStatus::Conflict, _location);
        } else {
            throw StatusCodeException(HttpStatus::InternalServerError, _location);
        }
    }

    _body->write(body_ss->str().c_str(), body_ss->str().length());
    _isBodyFile = true;
    delete body_ss;
}

bool Request::isBodyFinished() const {
    return _bparser.end;
}

bool Request::isHeadersFinished() const {
    return _parser.end;
}

void Request::setHeaderFinished(bool isFinished) {
    _parser.end = isFinished;
}
void Request::setBodyFinished(bool isFinished) {
    // if (isFinished) {
    //     reset();
    // }
    _bparser.end = isFinished;
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

Method Request::getMethod() const {
    return this->_method;
}

const std::string & Request::getRequestTarget() const {
    return this->_request_target;
}

const std::string & Request::getHTTPVersion() const {
    return this->_http_version;
}

const std::string & Request::getFilePath() const {
    return _file_path;
}
const std::string & Request::getFilename() const {
    return _filename;
}
Buffer & Request::getBuffer() {
    return _parser.buff;
}

std::string Request::getMethodName() const {
    if (_method == GET)
        return "GET";
    else if (_method == POST)
        return "POST";
    else if (_method == DELETE)
        return "DELETE";
    else
        return "UNKNOWN";
}
