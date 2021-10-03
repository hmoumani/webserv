#include "Response.hpp"
#include "debug.hpp"
#include <algorithm>

Response::Response() : _is_cgi(false), _status(HttpStatus::StatusCode(200)), sent_body(0), _isCgiHeaderFinished(false), cgiHeader("")
{
}

Response::Response(Response const & src) : _is_cgi(false), _status(HttpStatus::StatusCode(200)), sent_body(0), _isCgiHeaderFinished(false), cgiHeader("")
{
	*this = src;
}

Response::~Response(){}

Response &Response::operator=(Response const & src)
{
	buffer_header = src.buffer_header;
	buffer_body = src.buffer_body;
	_status = src._status;
	_server = src._server;
	_is_cgi = src._is_cgi;
	sent_body = src.sent_body;
	return (*this);
}

void Response::reset() {
    Message::reset();
	_is_cgi = false;
	_status = HttpStatus::StatusCode(200);
	buffer_header.resize(0);
	buffer_body.resize(0);
	sent_body = 0;
	_isCgiHeaderFinished = false;
	cgiHeader.str("");
	
}

// Response::Response(Request const & req, const Config * config) 
// {
// }

void Response::handleRequest(Request const & req) {
	this->_server = req.getServerConfig();
	this->_location = req.getLocation();
	// const Config * location = getLocation(req, _server);

	// const std::string request_path = getRequestedPath(req, location);

	// std::cerr <<  "****" << req.getRequestTarget() << std::endl; 
	// std::cerr << _location->root << std::endl;
	// std::cerr << req.getFilename() << std::endl;

	if (_server->location.find(Utils::getFileExtension(req.getFilename())) != _server->location.end()) {
		_is_cgi = true;
		if (req.isBodyFinished()) {
			this->handleCGI(req);
		}
	}
	else if (req.getMethod() == GET)
		this->handleGetRequest(req);
	else if (req.getMethod() == POST)
		this->handlePostRequest(req);
	else if (req.getMethod() == DELETE)
		this->handleDeleteRequest(req);
}

void Response::handleCGI(Request const & req)
{
	std::string filename = req.getFilename();
	std::cerr << _location->cgi << std::endl;
	char buff[101] = {0};
	_is_cgi = true;
	char * const ar[4] = {const_cast<char *>(_location->cgi.c_str()), const_cast<char *>(filename.c_str()), NULL};
	pipe(fd);
	pipe(fd_body);
	// req.setBodySize(req.getBody()->tellp());
	
	pid = fork();
	if (pid == 0) // child process (cgi)
	{
		std::vector<const char *> v;
		v.push_back(strdup((std::string("REQUEST_METHOD") + "=" + req.getMethodName()).c_str()));
		v.push_back(strdup((std::string("PATH") + "=" + getenv("PATH")).c_str()));
		v.push_back(strdup((std::string("TERM") + "=" + getenv("TERM")).c_str()));
		v.push_back(strdup((std::string("HOME") + "=" + getenv("HOME")).c_str()));
		gethostname(buff, 100);
		v.push_back(strdup((std::string("HOSTNAME") + "=" + buff).c_str()));
		getlogin_r(buff, 100);
		v.push_back(strdup((std::string("USER") + "=" + buff).c_str()));
		v.push_back(strdup((std::string("SCRIPT_FILENAME") + "=" + filename).c_str()));
		size_t n = req.getRequestTarget().find_first_of('?');
		n = n == std::string::npos ? req.getRequestTarget().length() : n + 1;
		v.push_back(strdup((std::string("CONTENT_LENGTH") + "=" + Utils::to_str(req.getBodySize())).c_str()));
		// std::cerr << "CONTENT_LENGTH : " << req.getBodySize() << " " << req.getBody()->tellg() << std::endl;
		v.push_back(strdup((std::string("CONTENT_TYPE") + "=" + req.getHeader("Content-Type")).c_str()));
		// std::cerr << "len : " << length << std::endl;
		v.push_back(strdup((std::string("QUERY_STRING") + "=" + req.getRequestTarget().substr(n)).c_str()));
		v.push_back(strdup((std::string("HTTP_COOKIE") + "=" + req.getHeader("Cookie")).c_str()));
		std::cerr << "coockes : " << req.getHeader("Cookie") << std::endl;
		// v.push_back(strdup((std::string("REDIRECT_STATUS") + "=").c_str()));
		v.push_back(NULL);
		close(fd[0]);
		close(fd_body[1]);
		dup2(fd[1], 1);
		dup2(fd_body[0], 0);
		// write(fd[1], "Hello\n", 6);
		// TODO: check execve return; 
		if (execve(ar[0], ar, const_cast<char * const *>(v.data())) == -1)
			exit(42);
	}
	close(fd[1]);
	close(fd_body[0]);
	// dup2(fd_body[1], 1);
	req.getBody()->seekg(0, req.getBody()->beg);
	// req.getBody()->seekp(0, req.getBody()->end);
	// set_cgi_body(req);
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(filename.c_str());
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");
}

void Response::handleGetRequest(Request const & req)
{
	std::string filename = req.getFilename();
	struct stat fileStat;

	std::fstream * file = new std::fstream();
	file->open(filename.c_str());
	delete _body;
	_body = file;
	stat (filename.c_str(), &fileStat);
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(fileStat));
	insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(filename.c_str());
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");
}

void Response::handlePostRequest(Request const & req)
{
	std::cout << "upload pass :" << req.getServerConfig()->upload << std::endl;
	std::string filename = req.getFilename();
	struct stat fileStat;

	std::fstream * file = new std::fstream();
	file->open(filename.c_str());
	delete _body;
	_body = file;
	stat (filename.c_str(), &fileStat);
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(fileStat));
	insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(filename.c_str());
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");
}

void Response::handleDeleteRequest(Request const & req)
{

}

std::string Response::HeadertoString()
{
	std::ostringstream response("");

	// if (is_cgi())
	// {
	// 	char s[2050] = {0};
	// 	// buff.resize(1025);
	// 	// buff.data[1024] = 0;
	// 	int ret = 0, total = 0;
	// 	size_t pos;
	// 	while (true)
	// 	{
	// 		// std::cerr << "fd: " << fd[0] << std::endl;
	// 		pollfd pfd = (pollfd){fd[0], POLLIN};
	// 		// std::cerr << "Before" << std::endl;
	// 		int pret = poll(&pfd, 1, -1);
	// 		// std::cerr << "After" << std::endl;
	// 		if(pret == -1)
	// 			error("poll failed");
	// 		ret = read(fd[0], s + total, 2049 - total);
	// 		// std::cerr << "ret: " << ret << std::endl;
	// 		// if (ret <= 0)
	// 		// 	return "";
	// 		total += ret;
	// 		// s[total] = 0;
	// 		if ((pos = std::string(s).find("\r\n\r\n")) != std::string::npos){
	// 			pos += 4;
	// 			break ;
	// 		}
	// 		if ((pos = std::string(s).find("\n\n")) != std::string::npos){
	// 			pos += 2;
	// 			break ;
	// 		}
	// 	}

	// 	buffer_body.setData(std::string(s).substr(pos).c_str(), std::string(s).substr(pos).length());
	// 	s[pos] = '\0';
	// 	std::istringstream iss(s);
	// 	std::string line;
	// 	while (std::getline(iss, line))
	// 	{
	// 		size_t start = line.find_first_of(':');
	// 		size_t end = line.find_first_of(':') + 2;
	// 		if (start == std::string::npos || end == std::string::npos)
	// 			continue ;
	// 		std::string str = line.substr(line.find_first_of(':') + 2);
	// 		insert_header(line.substr(0, start), trim(str));
	// 		// _headers[line.substr(0, start)] = trim(str);
	// 	}
	// }
	response << "HTTP/1.1 " << this->_status << " " << reasonPhrase(this->_status) << CRLF;
	std::cerr << response.str() << std::endl;
	for (std::multimap<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << CRLF;
	}
	response << CRLF;
	return (response.str());
}

const std::iostream * Response::getFile() const {
	return _body;
}

// void    Response::send_file(Socket & connection)
// {
// 	const int SIZE = 2;
// 	char buff[SIZE];	

// 	ssize_t sent = 0;
// 	ssize_t temp = 0;
// 	stream->read(buff, SIZE);
// 	std::streamsize s = ((*stream) ? SIZE : stream->gcount());

// 	int ret = ::send(connection.getFD(), buff, s, 0);

// 	if (ret != -1) {
// 		std::streampos lost = s - ret;
// 		stream->seekg(stream->tellg() - lost);
// 	}

// 	// dout << ret << "\n";
// }

void	Response::readFile() {

	if (!_is_cgi)
	{
		buffer_body.resize(1024);
		_body->read(buffer_body.data, buffer_body.size);
		std::streamsize s = ((*_body) ? buffer_body.size : _body->gcount());
		buffer_body.resize(s);
	}
	else
	{
		pollfd pfd = (pollfd){fd[0], POLLIN};
		close(fd[1]);
		// waitpid(pid, NULL, 0);
		buffer_body.resize(1024);
		int pret = poll(&pfd, 1, -1);
		if (pret == -1)
			error("poll failed");
		int ret = read(fd[0], buffer_body.data, 1024);
		if (ret == 0) {
			_is_cgi = false;
			close(fd[0]);
		}
		if (ret != 1024) {
			buffer_body.resize(ret);
		}
	}
}

std::stringstream * errorTemplate(const StatusCodeException & e) {
	std::stringstream * alloc = new std::stringstream("");
	std::stringstream & body = *alloc;

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

	return &body;
}


void Response::setErrorPage(const StatusCodeException & e, const Config * location) {
	_status = e.getStatusCode();

	// _headers["Connection"] = "keep-alive";
	// _headers["Content-Type"] = "text/html";
	// _headers["Date"] = Utils::getDate();
	// _headers["Server"] = SERVER_NAME;
	insert_header("Connection", "keep-alive");
	insert_header("Content-Type", "text/html");
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);

	if (e.getLocation() != ""){
		// _headers["Location"] = e.getLocation();
		insert_header("Location", e.getLocation());
		dout << "Location: " << e.getLocation() <<  CRLF;
	}

	std::stringstream * err;

	const std::map<int, std::string> & error_page = location->error_page.empty() ? _server->error_page : location->error_page;

	std::fstream * errPage = NULL;

	if (error_page.find(_status) != error_page.end()) {
		errPage = new std::fstream();
		std::string filename = location->root;

		filename += error_page.find(_status)->second;
		errPage->open(filename.c_str());
	}

	delete _body;
	if (!errPage || !errPage->is_open()) {
		_body = errorTemplate(e);
		if (errPage) {
			delete errPage;
	}
	} else {
		_body = errPage;
	}

	// _body->seekg(0, _body->beg);
	// std::cerr << "Content-Length: " << _body->tellp() << std::endl;
	// _headers["Content-Length"] = Utils::to_str(_body->tellp());
	// _headers["Transfer-Encoding"] = "chunked";
	insert_header("Transfer-Encoding", "chunked");

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


std::string Response::listingPage(const ListingException & e)
{
	std::stringstream header("");
	std::stringstream *body = new std::stringstream("");

	// _headers["Connection"] = "keep-alive";
	// _headers["Content-Type"] = "text/html";
	// _headers["Date"] = Utils::getDate();
	// _headers["Server"] = SERVER_NAME;
	// _headers["Transfer-Encoding"] = "chunked";
	insert_header("Connection", "keep-alive");
	insert_header("Content-Type", "text/html");
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Transfer-Encoding", "chunked");
	

	DIR *dir;
	struct dirent *ent;
	*body << "<!DOCTYPE html>\n" ;
	*body << "<html>\n";
	*body << "<head><title>Index of " << e.getReqTarget() << "</title></head>\n";
	*body << "<body bgcolor=\"white\">\n";
	*body << "<h1>Index of " << e.getReqTarget() << "</h1><hr><pre><a href=\"" << e.getReqTarget().substr(0, e.getReqTarget().find_last_of('/', e.getReqTarget().length() - 2)) << "/\">../</a>\n";

	if ((dir = opendir (e.what())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			struct stat attr;
			char format[250];
			std::string file_path = e.getPath() + std::string(ent->d_name);
			if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name) || !(stat(file_path.c_str(), &attr) == 0 && S_ISDIR(attr.st_mode)))
				continue ;
			*body << "<a href=\"" << ent->d_name << "/\">" << ent->d_name << "/</a> " << std::setw(69 - strlen(ent->d_name)) << getFileCreationTime(file_path.c_str(), format);
			*body << "                   -\n";
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
			*body << "<a href=\"" << ent->d_name << "\">" << ent->d_name << "</a> " << std::setw(70 - strlen(ent->d_name)) << getFileCreationTime(file_path.c_str(), format);
			*body << std::setw(20);
			*body << attr.st_size;
			*body << "\n";
		}
		closedir (dir);
	}
	*body << "</pre><hr></body>\n</html>\n";
	// header << "Content-Length: " << (*body).str().length() << CRLF << CRLF;
	header << (*body).str();
	_body = body;
	return header.str();
}

bool Response::is_cgi() const
{
	return this->_is_cgi;
}

bool Response::isSendingBodyFinished(const Request & request) const
{
	return request.getBodySize() == sent_body || request.getBodySize() == 0;
}

void Response::set_cgi_body(const Request & request)
{
	char buff[BUFFER_SIZE] = {0};
	// close(fd_body[0]);
	// sent_body = request.getBody()->tellg();
	// std::cerr << "tellg " << request.getBody()->tellg() << std::endl;
	// std::cerr  << "sent " << sent_body << "/" << request.getBodySize() << std::endl;
	// while (sent_body < request.getBodySize()){
		std::streampos n = std::min((std::streampos)(request.getBodySize() - request.getBody()->tellg()), (std::streampos)(BUFFER_SIZE));
		request.getBody()->read(buff, n);
		// std::streamsize n = 47; // request.getBody()->tellg();
		sent_body += n;
		// std::cerr  << "sent :" << n << "=>" <<  sent_body << "/" << request.getBodySize() << std::endl;
		write(fd_body[1], buff, n);
		if (isSendingBodyFinished(request)) {
			close(fd_body[1]);
		}
	// }
	// close(fd_body[1]);
}


HttpStatus::StatusCode Response::getStatusCode() const {
	return _status;
}

bool Response::isCgiHeaderFinished() const
{
	return _isCgiHeaderFinished;
}

void Response::readCgiHeader()
{
	std::string temp;
	size_t pos;

	int status = 0;
	int ret = waitpid(pid, &status, WNOHANG);
	if (ret == pid && WEXITSTATUS(status) == 42) {
		throw StatusCodeException(HttpStatus::InternalServerError, _location);
	}
	if (!isCgiHeaderFinished())
	{
		char s[2050] = {0};
		// buff.resize(1025);
		// buff.data[1024] = 0;
		int ret = 0;
		// size_t pos;
		// std::cerr << "fd: " << fd[0] << std::endl;
		pollfd pfd = (pollfd){fd[0], POLLIN};
		// std::cerr << "Before" << std::endl;
		int pret = poll(&pfd, 1, -1);
		if (pfd.revents & 0) {
			return ;
		}
		// std::cerr << "After" << std::endl;
		if(pret == -1)
			error("poll failed");
		ret = read(fd[0], s, 2049);
		if (ret == -1)
			return ;
		cgiHeader << s;
		// std::cerr << "ret: " << ret << std::endl;
		// if (ret <= 0)
		// 	return "";
		// s[total] = 0;
		temp = cgiHeader.str();
		if ((pos = temp.find("\r\n\r\n")) != std::string::npos){
			pos += 4;
			_isCgiHeaderFinished = true;
			
		}
		else if ((pos = temp.find("\n\n")) != std::string::npos){
			pos += 2;
			_isCgiHeaderFinished = true;
		}
		if (isCgiHeaderFinished()) 
		{
			buffer_body.setData(temp.substr(pos).c_str(), temp.substr(pos).length());
			std::stringstream ss(temp.substr(0, pos));
			std::string line;
			while (std::getline(ss, line))
			{
				size_t start = line.find_first_of(':');
				size_t end = line.find_first_of(':');
				if (start == std::string::npos || end == std::string::npos)
					continue ;
				std::string str = line.substr(line.find_first_of(':') + 2);
				insert_header(line.substr(0, start), trim(str));
				// _headers[line.substr(0, start)] = trim(str);
			}
		}
	}
	// if (isCgiHeaderFinished()) 
	// {
	// 	buffer_body.setData(temp.substr(pos).c_str(), temp.substr(pos).length());
	// 	std::stringstream ss(temp);
	// 	std::string line;
	// 	while (std::getline(ss, line))
	// 	{
	// 		size_t start = line.find_first_of(':');
	// 		size_t end = line.find_first_of(':') + 2;
	// 		if (start == std::string::npos || end == std::string::npos)
	// 			continue ;
	// 		std::string str = line.substr(line.find_first_of(':') + 2);
	// 		insert_header(line.substr(0, start), trim(str));
	// 		// _headers[line.substr(0, start)] = trim(str);
	// 	}
	// }
}
