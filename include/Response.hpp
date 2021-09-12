#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <fstream>
# include <string>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <sstream>
# include <stdlib.h>

# include "Buffer.hpp"
# include "Request.hpp"
# include "Socket.hpp"
# include "Utils.hpp"
# include "MimeTypes.hpp"
// #include

class Response : public Message
{
    private:
        HttpStatus::StatusCode status;
        std::ifstream file;
        struct stat fileStat;
        std::string basePath;

        const Config * server;

        const std::string getRequestedPath(const Request &, const Config *);
    public:
        Buffer buffer;
        Response();
        Response(Response const &);
        Response(Request const &, const Config *);
        ~Response();
        Response &operator= (Response const &);
        void handleGetRequest(Request const &, const Config *, const std::string &);
        void handlePostRequest(Request const &, const Config *);
        void handleDeleteRequest(Request const &, const Config *);
        std::string HeadertoString() const;
        void    send_file(Socket & connection);
        void    readFile();
        const std::ifstream & getFile() const;
        std::string getIndexFile(const Config *, const std::string &, const std::string &);
        void setServerConfig(Config * config);
};

std::string errorPage(const StatusCodeException & e);
std::string listingPage(const ListingException & e);
static const Config * getLocation(const Request & req, const Config * server);
static const std::string getPathFromUri(const std::string & uri);

#endif