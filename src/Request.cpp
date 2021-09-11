#include "Request.hpp"

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

Request::Request(const std::string & message) throw(StatusCodeException) : Message(message) {
    size_t start = 0;
    size_t end;

    for (int i = 0; i < 3; ++i) {
        end = _start_line.find_first_of(' ', start);
        std::string token = _start_line.substr(start, end - start);
        start = _start_line.find_first_not_of(' ', end);
        if (i == 0) {
            _method = getMethodFromName(token);
        } else if (i == 1) {
            _request_target = token;
        } else {
            _http_version = token;
        }
    }

    if (_request_target.empty() || _http_version != "HTTP/1.1") {
        throw StatusCodeException(HttpStatus::BadRequest);
    }
}

Request::~Request() {
}


Method Request::getMethod() const { return this->_method; }
const std::string & Request::getRequestTarget() const { return this->_request_target; }
const std::string & Request::getHTTPVersion() const { return this->_http_version; }

