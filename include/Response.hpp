#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <fstream>
# include <string>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <sstream>
# include <dirent.h>
# include <poll.h>
# include <ctype.h>
# include "Buffer.hpp"
# include "Request.hpp"
// # include "Socket.hpp"
# include <algorithm>
# include "Utils.hpp"
# include "MimeTypes.hpp"
#include "Config.hpp"
// #include
class Config;
class Request;
class Socket;

#define SERVER_NAME "WebServer (MacOs)"

class Response : public Message
{
    private:
        HttpStatus::StatusCode status;
        std::iostream * stream;
        struct stat fileStat;
        std::string basePath;
        bool        _is_cgi;
        pid_t       pid;
        int         fd[2];
        const Config * server;

        const std::string getRequestedPath(const Request &, const Config *);
    public:
        Buffer buffer_header;
        Buffer buffer_body;
        Response();
        Response(Response const &);
        Response(Request const &, const Config *);
        ~Response();
        Response &operator= (Response const &);
        void handleGetRequest(Request const &, const Config *, const std::string &);
        void handlePostRequest(Request const &, const Config *);
        void handleDeleteRequest(Request const &, const Config *);
        std::string HeadertoString();
        void    send_file(Socket & connection);
        void    readFile();
        const std::iostream * getFile() const;
        std::string getIndexFile(const Config * location, const std::string & filename, const std::string & req_taget);
        void setErrorPage(const StatusCodeException & e, const Config * location);

        void setServerConfig(const Config * config);
        const Config * getServerConfig() const;
        bool is_cgi() const ;
        std::string listingPage(const ListingException & e);

};

std::stringstream * errorPage(const StatusCodeException & e);
static const Config * getLocation(const Request & req, const Config * server);
static const std::string getPathFromUri(const std::string & uri);
static const void handleRequest(const Request & req, const Config * location);
void error(std::string message);

#endif