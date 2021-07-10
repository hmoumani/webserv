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
	Utils::fileStat(filename, buffer);
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(this->buffer));
	insert_header("Transfer-Encoding", "chunked");
	insert_header("Content-Type", MimeTypes::getType(filename.c_str()));
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");

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

	response << "HTTP/1.1 " << this->status << " " << reasonPhrase(this->status) << CRLF;
	for (std::unordered_map<std::string, std::string>::const_iterator it = _headers.cbegin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << CRLF;
	}
	response << CRLF;
	return (response.str());
}

void    Response::send_file(Request const & req, Socket & connection)
{
	std::ostringstream oss("");
	std::string line;
	const int SIZE = 1024;
	std::vector<char> buffer (SIZE, 0);
	std::cout << req.getRequestTarget() << "\n";
	std::ifstream file(req.getRequestTarget().substr(1));

	while(1)
	{
		file.read(buffer.data(), SIZE);
		std::streamsize s = ((file) ? SIZE : file.gcount());
		oss << std::hex << s << CRLF;
		connection.send(oss.str());
		oss.str("");
		// oss << buffer << CRLF;
		// connection.send(buffer);
		::send(connection.getFD(), buffer.data(), s, 0);
		connection.send(CRLF);
		// oss.str("");
		if(!file) break;
	}
	// oss.str("");
	// oss << s;
	// std::cerr << "***" <<oss.str() << std::endl;
	// insert_header("Content-Length", oss.str());
	oss << "0" << CRLF << CRLF;
	connection.send(oss.str());

	std::string head = HeadertoString();
	// std::cout << head << std::endl;
	// connection.send(head);
	// connection.send(buffer);



}
