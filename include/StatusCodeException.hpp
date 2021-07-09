#pragma once

#include <exception>
#include "StatusCode.hpp"

class StatusCodeException : public std::exception {
    private:
        HttpStatus::StatusCode _code;
    public:
        StatusCodeException(HttpStatus::StatusCode code) : _code(code) {}
        const char * what() const throw ()
        {
            return HttpStatus::reasonPhrase(_code);
        }

        HttpStatus::StatusCode getStatusCode() const {
            return _code;
        }
};