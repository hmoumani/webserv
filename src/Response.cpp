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
	buffer = src.buffer;
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
	std::ostringstream oss("");

	file.open(filename);

	stat (filename.c_str(), &this->fileStat);
	Utils::fileStat(filename, fileStat);
	oss << this->fileStat.st_size;
	std::cerr << "SIZE: " <<  this->fileStat.st_size << std::endl;
	std::cerr << "FILE: " <<  filename.c_str() << std::endl;
	insert_header("Content-Length", oss.str());
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(this->fileStat));
	// insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(filename.c_str());
	type = type ? type : "text/plain";
	insert_header("Content-Type", type);
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

const std::ifstream & Response::getFile() const {
	return file;
}

void    Response::send_file(Socket & connection)
{
	const int SIZE = 2;
	char buff[SIZE];	

	ssize_t sent = 0;
	ssize_t temp = 0;
	file.read(buff, SIZE);
	std::streamsize s = ((file) ? SIZE : file.gcount());

	int ret = ::send(connection.getFD(), buff, s, 0);

	if (ret != -1) {
		std::streampos lost = s - ret;
		file.seekg(file.tellg() - lost);
	}

	std::cerr << ret << "\n";
}

void    Response::readFile() {

	buffer.resize(1024);
	file.read(buffer.data, buffer.size);
	std::streamsize s = ((file) ? buffer.size : file.gcount());
	buffer.resize(s);
}

// bool send_buffer(Socket & connection) {
// 	connection.send()
// }