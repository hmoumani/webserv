#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "Message.hpp"
#include "StatusCodeException.hpp"
#include <algorithm>
#include <iomanip>
#include "ListingException.hpp"
#include "Socket.hpp"
// #include "webserv.hpp"
// #define BUFFER_SIZE 128

class Socket;

class Request : public Message {
private:
    Method _method;
    std::string _request_target;
    std::string _http_version;

    struct {
        enum stat {
            METHOD, REQUEST_TARGET, HTTP_VER, HEADER_KEY, HEADER_VALUE, BODY
        } current_stat;
        std::string str;
        std::string key;
        bool cr;
    } _parser;

    struct {
        ssize_t len;
        std::string str;
        bool cr;
        bool end;
    } _bparser;
    static const size_t max_size[];
    static size_t file_id;
    bool parse(const char * buff, size_t size);
    size_t receiveBody(const char * buff, size_t size);
    void openBodyFile();

public:
    Request(const Socket & connection) throw (StatusCodeException);
    ~Request();

    const Method getMethod() const;
    const std::string & getRequestTarget() const;
    const std::string & getHTTPVersion() const;

    bool isFinished() const;
    void receive(const Socket & connection);


};

Method getMethodFromName(const std::string & method);


#endif