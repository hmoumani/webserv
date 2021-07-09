#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "Message.hpp"
#include "StatusCodeException.hpp"
// #include "webserv.hpp"

enum Method {
	GET,
	POST,
	DELETE,
	UNKNOWN
};

class Request : public Message {
private:
    Method _method;
    std::string _request_target;
    std::string _http_version;
public:
    Request(const std::string & message) throw (StatusCodeException);
    ~Request();
    Method getMethod() const;
    const std::string & getRequestTarget() const;
    const std::string & getHTTPVersion() const;

};

#endif