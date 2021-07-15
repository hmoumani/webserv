#pragma once

#include <exception>
#include "StatusCode.hpp"

class StatusCodeException : public std::exception {
    private:
        HttpStatus::StatusCode _code;
        std::string _location;
    public:
        StatusCodeException(HttpStatus::StatusCode code) : _code(code), _location("") {}
        StatusCodeException(HttpStatus::StatusCode code, std::string const & location) : _code(code), _location(location) {}
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
    virtual ~StatusCodeException() throw() {return ;}
};