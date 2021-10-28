#pragma once

#include <exception>
#include "StatusCode.hpp"

class Config;

class StatusCodeException : public std::exception {
    private:
        HttpStatus::StatusCode _code;
        std::string _location;
        const Config * _server;
    public:
        StatusCodeException(HttpStatus::StatusCode code, std::string const & location, const Config * server) : 
            _code(code), _location(location), _server(server) {}
        StatusCodeException(HttpStatus::StatusCode code, const Config * server) : _code(code), _location(""), _server(server) {}
        const char * what() const throw ()
        {
            return HttpStatus::reasonPhrase(_code);
        }

        HttpStatus::StatusCode getStatusCode() const {
            return _code;
        }

        std::string const & getLocation() const {
            return this->_location;
        }

        const Config * getServer() const {
            return this->_server;
        }
    virtual ~StatusCodeException() throw() {return ;}
};