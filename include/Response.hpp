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
# include <algorithm>
# include "Utils.hpp"
# include "MimeTypes.hpp"
# include "Config.hpp"
# include <cstring>
#include <sys/types.h>
#include <sys/wait.h>

class Config;
class Request;
class Socket;

#define SERVER_NAME "WebServer (MacOs)"
class Response : public Message
{
    private:
        HttpStatus::StatusCode	_status;
        bool					_is_cgi;
        pid_t					pid;
        int						fd[2];
        int						fd_body[2];
        std::streamsize			body_size_cgi;
        std::streamsize			sent_body;
        bool					_isCgiHeaderFinished;
		std::stringstream		cgiHeader;

        bool _read_body_finished;
        bool _send_end_chunk;

        bool _is_request_handled;
    public:
        Buffer buffer;
        Response();
        Response(Response const &);
        ~Response();
        Response &operator= (Response const &);
        void handleRequest(Request const &, Socket const & sock);
        void handleCGI(Request const &, Socket const & sock);
        void handleGetRequest(Request const &);
        void handlePostRequest(Request const &);
        void handleDeleteRequest(Request const &);
        std::string HeadertoString();
        HttpStatus::StatusCode getStatusCode() const;
        void    send_file(Socket & connection);
        void    readFile();
        std::string getIndexFile(const Config * location, const std::string & filename, const std::string & req_taget);
        void setErrorPage(const StatusCodeException & e, const Config * location);
        void listingPage(const ListingException & e);
        bool is_cgi() const ;
        bool isSendingBodyFinished(const Request & request) const;
        bool isCgiHeaderFinished() const;
        void setCgiHeaderFinished(bool stat);

		void readCgiHeader();
        void set_cgi_body(const Request & request);
        void reset();
        bool isEndChunkSent() const;
        void setEndChunkSent(bool isSent);
        void closeFdBody();
        void closeFd();
        bool isReadBodyFinished() const;        
        bool isRequestHandled() const;


};

std::stringstream * errorPage(const StatusCodeException & e);
void error(std::string message);
#endif