#pragma once 

#include <exception>
#include <iostream>

class ListingException : public std::exception {
    private:
        std::string _path;
        std::string _req_taget;
    public:
        ListingException(std::string const & path, std::string const & req_taget) : _path(path), _req_taget(req_taget) {}
        const char * what() const throw ()
        {
            return _path.c_str();
        }

        std::string const & getPath() const {
            return _path;
        }

        std::string const & getReqTarget() const {
            return _req_taget;
        }

    virtual ~ListingException() throw() {return ;}
};