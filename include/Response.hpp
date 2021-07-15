#ifndef RESPONSE_HPP
# define RESPONSE_HPP
# include "StatusCode.hpp"
# include "webserv.hpp"
# include "Buffer.hpp"


class Response : public Message
{
    private:
        HttpStatus::StatusCode status;
        std::ifstream file;
        struct stat fileStat;
        std::string basePath;
    public:
        Buffer buffer;
        Response();
        Response(Response const &);
        Response(Request const &);
        ~Response();
        Response &operator= (Response const &);
        void handleGetRequest(Request const &);
        void handlePostRequest(Request const &);
        void handleDeleteRequest(Request const &);
        std::string HeadertoString() const;
        void    send_file(Socket & connection);
        void    readFile();
        const std::ifstream & getFile() const;
        std::string getIndexFile(std::string);

};

std::string errorPage(const StatusCodeException & e);
#endif