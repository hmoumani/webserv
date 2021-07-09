#ifndef RESPONSE_HPP
# define RESPONSE_HPP
# include "StatusCode.hpp"
# include "webserv.hpp"

class Response : public Message
{
    private:
        HttpStatus::StatusCode status;
        struct stat buffer;
    public:
        Response();
        Response(Response const &);
        Response(Request const &);
        ~Response();
        Response &operator= (Response const &);
        void handleGetRequest(Request const &);
        void handlePostRequest(Request const &);
        void handleDeleteRequest(Request const &);
        std::string HeadertoString() const;
        void    send_file(Request const & req, Socket & connection);
};

#endif