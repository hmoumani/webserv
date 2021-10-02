#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "Message.hpp"
#include "StatusCodeException.hpp"
#include <algorithm>
#include <iomanip>
#include "ListingException.hpp"
#include "Socket.hpp"
// #include "webserv.hpp"
#define BUFFER_SIZE 1024 * 1024

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
        bool end;

        Buffer buff;
    } _parser;

    struct {
        ssize_t len;
        std::string str;
        bool cr;
        bool end;
    } _bparser;
    static const size_t max_size[];
    static size_t file_id;

    std::string _filename;

    bool parse();
    size_t receiveBody();
    void openBodyFile();
    void checkRequestTarget();


public:
    Request();

    Request(const Socket & connection) throw (StatusCodeException);
    ~Request();

    const Method getMethod() const;
    std::string getMethodName() const;
    const std::string & getRequestTarget() const;
    const std::string & getHTTPVersion() const;
    const std::string & getFilename() const;
    Buffer & getBuffer();

    bool isHeadersFinished() const;
    bool isBodyFinished() const;
    void setHeaderFinished(bool isFinished);
    void setBodyFinished(bool isFinished);
    void receive(const Socket & connection);
    void reset();



};

Method getMethodFromName(const std::string & method);
const std::string getPathFromUri(const std::string & uri);
std::string & trim(std::string & str);


#endif