#include "Response.hpp"
#include "debug.hpp"

Response::Response() : _is_cgi(false)
{
}

Response::Response(Response const & src) : _is_cgi(false)
{
	*this = src;
}

Response::~Response(){}

Response &Response::operator=(Response const & src)
{
	buffer_header = src.buffer_header;
	buffer_body = src.buffer_body;
	return (*this);
}

Response::Response(Request const & req, const Config * config) : status(HttpStatus::StatusCode(200)), server(config), _is_cgi(false)
{
	const Config * location = getLocation(req, config);
	location = location ?: server;


	const std::string request_path = getRequestedPath(req, location);


	std::cout <<  "****" << req.getRequestTarget() << std::endl;
	std::cout << location->root << std::endl;
	std::cout << request_path << std::endl;

	if (req.getMethod() == GET)
		this->handleGetRequest(req, location, request_path);
	if (req.getMethod() == POST)
		this->handlePostRequest(req, location);
	if (req.getMethod() == DELETE)
		this->handleDeleteRequest(req, location);
}

const std::string Response::getRequestedPath(const Request & req, const Config * location) {
	const std::string path = getPathFromUri(req.getRequestTarget());
	struct stat buffer;

	std::string requested_path = location->root;
	// requested_path += (location->uri != "" && location->uri[location->uri.length() - 1] != '/') ?  "/" : "";
	requested_path += location->uri != "" ?  path.substr(location->uri.length()) : path;
	// dout << "SUBSTR: " << location->uri << " " << location->uri.length() << " " << path.substr(location->uri.length()) << std::endl;

	std::cerr << "Requested File: " << requested_path << std::endl;
	if (requested_path[requested_path.length() - 1] != '/' && stat(requested_path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode)) {
		throw StatusCodeException(HttpStatus::MovedPermanently, path + '/');
	}

	return requested_path;
}

static const std::string getPathFromUri(const std::string & uri) {
	return uri.substr(0, uri.find_first_of('?'));
}
static const Config * getLocation(const Request & req, const Config * server) {
	size_t len = 0;
	const Config * loc = NULL;
	for (std::map<std::string, Config>::const_iterator it = server->location.begin(); it != server->location.end(); ++it) {
		const std::string path = getPathFromUri(req.getRequestTarget());
		if (it->first.length() <= path.length() && it->first.length() > len) {
			if (path.compare(0, it->first.length(), it->first.c_str()) == 0) {
				len = it->first.length();
				loc = &it->second;
			}
		}
	}
	return loc;
}

std::string Response::getIndexFile(const Config * location, const std::string & filename, const std::string & req_taget)
{
	for (int i = 0; i < location->index.size(); ++i)
	{
		std::string file = (filename + location->index[i]);
		if (access(file.c_str(), F_OK))
			continue ;
		struct stat buffer;
		stat (file.c_str(), &buffer);
		if (!(buffer.st_mode & S_IROTH))
			throw StatusCodeException(HttpStatus::Forbidden);
		else
			return (file);
	}
	if (!location->listing)
		throw StatusCodeException(HttpStatus::Forbidden);
	throw ListingException(filename, req_taget);
}

void Response::handleGetRequest(Request const & req, const Config * location, const std::string & request_path)
{
	std::string filename = request_path;
	std::ostringstream oss("");
	std::map<std::string, Config>::const_iterator cgi_location;

	file.open(filename.c_str());
	stat (filename.c_str(), &this->fileStat);
	Utils::fileStat(filename, fileStat);
	if (S_ISDIR(fileStat.st_mode))
	{
		filename = getIndexFile(location, filename, req.getRequestTarget());
		file.close();
		file.open(filename.c_str());
		stat (filename.c_str(), &this->fileStat);
	}
	for (std::map<std::string, Config>::const_iterator it = server->location.begin(); it != server->location.end(); ++it) {
		if (it->first == Utils::getFileExtension(filename)) {
			location = &it->second;
			break;
		}
	}
	std::cout << "lol: " << location->cgi << std::endl;
	if (Utils::getFileExtension(filename) == ".php")
	{
		_is_cgi = true;
		file.close();


		char s1[100] = "/goinfre/hmoumani/.brew/opt/php@7.2/bin/php-cgi";
		char * const ar[3] = {const_cast<char *>(location->cgi.c_str()), const_cast<char *>(request_path.c_str()), NULL};
		pipe(fd);
		pid = fork();
		if (pid == 0)
		{
			close(fd[0]);
			dup2(fd[1], 1);
			execve(s1, ar, NULL);
			close(fd[1]);
			return ;
		}
	}
	oss << this->fileStat.st_size;
	// insert_header("Content-Length", oss.str());
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(this->fileStat));
	insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(filename.c_str());
	type = type ?: "text/plain";
	insert_header("Content-Type", type);
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");
}

void Response::handlePostRequest(Request const & req, const Config * location)
{

}

void Response::handleDeleteRequest(Request const & req, const Config * location)
{

}

std::string Response::HeadertoString() const
{
	std::ostringstream response("");

	response << "HTTP/1.1 " << this->status << " " << reasonPhrase(this->status) << CRLF;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << CRLF;
	}
	// if (!is_cgi())
		response << CRLF;
	return (response.str());
}

const std::ifstream & Response::getFile() const {
	return file;
}

void	Response::send_file(Socket & connection)
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

	// std::cerr << ret << "\n";
}

void	Response::readFile() {

	if (!_is_cgi)
	{
		buffer_body.resize(1024);
		file.read(buffer_body.data, buffer_body.size);
		std::streamsize s = ((file) ? buffer_body.size : file.gcount());
		buffer_body.resize(s);
	}
	else
	{
		close(fd[1]);
		// waitpid(pid, NULL, 0);
		buffer_body.resize(1024);
		int ret = read(fd[0], buffer_body.data, 1024);
		if (ret == 0) {
			_is_cgi = false;
		}
		if (ret != 1024) {
			buffer_body.resize(ret);
		}
		// buffer << std::hex << ret;
		// buffer_body.data[ret] = 0;
		// printf("%d -> %s", ret,buffer.data);
	}
}

std::string errorPage(const StatusCodeException & e) {
	std::ostringstream header("");
	std::ostringstream body("");
	
	header << "HTTP/1.1 " << e.getStatusCode() << " " << reasonPhrase(e.getStatusCode()) << CRLF;
	header << "Connection: keep-alive" << CRLF;
	header << "Content-Type: text/html" << CRLF;
	if (e.getLocation() != ""){
		body << "Location: " << e.getLocation() <<  CRLF;
		std::cerr << "Location: " << e.getLocation() <<  CRLF;
	}
	header << "Date: " << Utils::getDate() <<  CRLF;
	header << "Server: " << SERVER_NAME << CRLF;

	body << "<!DOCTYPE html>\n" ;
	body << "<html lang=\"en\">\n";
	body << "<head>\n";
    body << "<title>" << e.getStatusCode() << "</title>\n";
	body << "</head>\n";
	body << "<body>\n";
    body << "<h1 style=\"text-align:center\">" << e.getStatusCode() << " - " << HttpStatus::reasonPhrase(e.getStatusCode()) << "</h1>\n";
	body << "<hr>\n";
	body << "<h4 style=\"text-align:center\">WebServer</h4>\n";
	body << "</body>\n";

	header << "Content-Length: " << body.str().length() << CRLF << CRLF;

	header << body.str();

	return header.str();
}

char* formatdate(char* str, time_t val)
{
        strftime(str, 250, "%d-%b-%Y %H:%M", localtime(&val));
        return str;
}

char *getFileCreationTime(const char *path, char *format) 
{
    struct stat attr;
    stat(path, &attr);
	return formatdate(format, attr.st_mtime);
}


std::string listingPage(const ListingException & e)
{
	std::ostringstream body("");

	body << "HTTP/1.1 " << 200 << " " << "OK" << CRLF;
	body << "Connection: keep-alive" << CRLF;
	body << "Content-Type: text/html" << CRLF;
	body << "Date: " << Utils::getDate() <<  CRLF;
	body << "Server: " << SERVER_NAME << CRLF << CRLF;

	DIR *dir;
	struct dirent *ent;
	body << "<!DOCTYPE html>\n" ;
	body << "<html>\n";
	body << "<head><title>Index of " << e.getReqTarget() << "</title></head>\n";
	body << "<body bgcolor=\"white\">\n";
	body << "<h1>Index of " << e.getReqTarget() << "</h1><hr><pre><a href=\"" << e.getReqTarget() << "\">../</a>\n";

	if ((dir = opendir (e.what())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			struct stat attr;
			char format[250];
			std::string file_path = e.getPath() + std::string(ent->d_name);
			if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name) || !(stat(file_path.c_str(), &attr) == 0 && S_ISDIR(attr.st_mode)))
				continue ;
			body << "<a href=\"" << ent->d_name << "\">" << ent->d_name << "</a> " << std::setw(70 - strlen(ent->d_name)) << getFileCreationTime(file_path.c_str(), format);
			body << "                   -\n";
		}
		closedir (dir);
	}

	if ((dir = opendir (e.what())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			char format[250];
			struct stat attr;
			std::string file_path = e.getPath() + std::string(ent->d_name);
			if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name) || (stat(file_path.c_str(), &attr) == 0 && S_ISDIR(attr.st_mode)))
				continue ;
			body << "<a href=\"" << ent->d_name << "\">" << ent->d_name << "</a> " << std::setw(70 - strlen(ent->d_name)) << getFileCreationTime(file_path.c_str(), format);
			body << std::setw(20);
			body << attr.st_size;
			body << "\n";
		}
		closedir (dir);
	}
	body << "</pre><hr></body>\n</html>\n";
	return body.str();
}

bool Response::is_cgi() const
{
	return this->_is_cgi;
}