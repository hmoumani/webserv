#include "Response.hpp"

Response::Response()
{
}

Response::Response(Response const & src)
{
	*this = src;
}

Response::~Response(){}

Response &Response::operator=(Response const & src)
{
	(void)src;
	return (*this);
}

Response::Response(Request const & req) : status(HttpStatus::StatusCode(200))
{
	if (req.getMethod() == GET)
		this->handleGetRequest(req);
	if (req.getMethod() == POST)
		this->handlePostRequest(req);
	if (req.getMethod() == DELETE)
		this->handleDeleteRequest(req);
}

void Response::handleGetRequest(Request const & req)
{
	std::string filename = req.getRequestTarget().substr(1);
	stat (filename.c_str(), &this->buffer);
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(this->buffer));
	insert_header("Transfer-Encoding", "chunked");
	insert_header("Content-Type", MimeTypes::getType(filename.c_str()));
	std::cout << "gwGERGERGregergergr" <<std::endl;
	insert_header("Connection", "Closed");

}

void Response::handlePostRequest(Request const & req)
{

}

void Response::handleDeleteRequest(Request const & req)
{

}

std::string Response::HeadertoString() const
{
	std::ostringstream response("");

	response << "HTTP/1.1 " << this->status << " " << reasonPhrase(this->status) << "\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.cbegin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << "\n";
	}
	response << "\n";
	return (response.str());
}

void    Response::send_file(Request const & req, Socket & connection)
{
	std::ostringstream oss("");
	std::string line;
	const int SIZE = 1024;
	std::vector<char> buffer (SIZE + 1, 0);
	std::cout << req.getRequestTarget() << "\n";
	std::ifstream file(req.getRequestTarget().substr(1));

	while(1)
	{
		file.read(buffer.data(), SIZE);
		std::streamsize s = ((file) ? SIZE : file.gcount());
		oss << std::hex << s << CRLF;
		connection.send(oss.str());
		oss.str("");
		buffer[s] = 0;
		oss << buffer.data() << CRLF;
		connection.send(oss.str());
		oss.str("");
		if(!file) break;
	}
	oss.str("");
	oss << "0" << CRLF << CRLF;

	std::cerr << oss.str() << std::endl;
	connection.send(oss.str());



}
