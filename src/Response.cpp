#include "Response.hpp"
#include "debug.hpp"
#include <algorithm>

Response::Response() : _send_end_chunk(false)
{
	reset();
}

Response::Response(Response const & src) : _send_end_chunk(false)
{
	reset();
	*this = src;
}

Response::~Response(){
	// close(fd[0]);
	// close(fd_body[1]);
}

Response &Response::operator=(Response const & src)
{
	buffer_header = src.buffer_header;
	buffer_body = src.buffer_body;
	_status = src._status;
	_server = src._server;
	_is_cgi = src._is_cgi;
	sent_body = src.sent_body;
	_send_end_chunk = src._send_end_chunk;
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

	fd_body[1] = -1;
	fd[0] = -1;
	_send_end_chunk = false;

	_send_header = false;
	_send_body = false;
	_is_request_handled = false;

}

// Response::Response(Request const & req, const Config * config) 
// {
// }

void Response::handleRequest(Request const & req, Socket const & sock) {
	this->_server = req.getServerConfig();
	this->_location = req.getLocation();
	// const Config * location = getLocation(req, _server);

	// const std::string request_path = getRequestedPath(req, location);

	// debug <<  "****" << req.getRequestTarget() << std::endl;
	// debug << _location->root << std::endl;
	// debug << req.getFilename() << std::endl;
	_send_end_chunk = false;
	if (_server->location.find(Utils::getFileExtension(req.getFilename())) != _server->location.end()) {
		_is_cgi = true;
		if (req.isBodyFinished()) {
			this->handleCGI(req, sock);
		} else {
			return;
		}
	} else if (req.getMethod() == GET) {
		this->handleGetRequest(req);
	} else if (req.getMethod() == POST) {
		this->handlePostRequest(req);
	} else if (req.getMethod() == DELETE) {
		this->handleDeleteRequest(req);
	} else {
		return ;
	}
	_is_request_handled = true;
}

void Response::handleCGI(Request const & req, Socket const & sock)
{
	std::string filename = req.getFilename();
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
		v.push_back(strdup((std::string("PATH") + "=" + (getenv("PATH") ?: "")).c_str()));
		v.push_back(strdup((std::string("AUTH_TYPE") + "=null").c_str()));
		v.push_back(strdup((std::string("TERM") + "=" + (getenv("TERM") ?: "")).c_str()));
		v.push_back(strdup((std::string("HOME") + "=" + (getenv("HOME") ?: "")).c_str()));
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
		v.push_back(strdup((std::string("GATEWAY_INTERFACE") + "=CGI/1.1").c_str()));
		// std::cerr << "hostname: " << req.getLocation()->port << std::endl;
		// std::cerr << "filename: " << filename << std::endl;
		v.push_back(strdup((std::string("PATH_INFO") + "=" + req.getRequestTarget().substr(0, n)).c_str()));
		v.push_back(strdup((std::string("PATH_TRANSLATED") + "=" + filename).c_str()));
		v.push_back(strdup((std::string("REMOTE_ADDR") + "=" + sock.getHost()).c_str()));
		v.push_back(strdup((std::string("REMOTE_HOST") + "=" + req.getHeader("host").substr(0, req.getHeader("host").find_first_of(':'))).c_str()));
		v.push_back(strdup((std::string("SERVER_NAME") + "=" + req.getLocation()->server_name).c_str()));
		v.push_back(strdup((std::string("SERVER_PORT") + "=" + Utils::to_str(req.getLocation()->port)).c_str()));
		v.push_back(strdup((std::string("SERVER_PROTOCOL") + "=HTTP/1.1").c_str()));
		v.push_back(strdup((std::string("SERVER_SOFTWARE") + "=" + SERVER_NAME).c_str()));
		// std::cerr << "len : " << length << std::endl;
		// for (std::multimap<std::string, std::string>::const_iterator it = req.getHeader().begin(); it != req.getHeader().end(); ++it)
		// {
		// 	std::cout << "head : " << it->first << " " << it->second << " " <<  req.getServerConfig()->host << "\n";
		// }
		v.push_back(strdup((std::string("QUERY_STRING") + "=" + req.getRequestTarget().substr(n)).c_str()));
		v.push_back(strdup((std::string("HTTP_COOKIE") + "=" + req.getHeader("Cookie")).c_str()));
		v.push_back(strdup((std::string("REDIRECT_STATUS") + "=").c_str()));
		v.push_back(NULL);
		close(fd[0]);
		close(fd_body[1]);
		dup2(fd[1], 1);
		dup2(fd_body[0], 0);
		// close(fd[1]);
		// close(fd_body[0]);
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
	if (type)
		insert_header("Content-Type", type);
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
	if (type)
		insert_header("Content-Type", type);
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");
}

void Response::handlePostRequest(Request const & req)
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
	if (type)
		insert_header("Content-Type", type);
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");
}

static void deleteDirectoryFiles(DIR * dir, const std::string & path) {
	struct dirent * entry = NULL;
	struct stat st;
	std::string filepath;
	DIR * dirp;
	
	while ((entry = readdir(dir))) {

		filepath = path + entry->d_name;

		if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		if (stat(filepath.c_str(), &st) == -1) {
			std::cerr << "stat(): " << filepath << ": " << strerror(errno) << std::endl;
		}

		if (S_ISDIR(st.st_mode)) {
			filepath += "/";
			if ((dirp = opendir(filepath.c_str()))) {
				deleteDirectoryFiles(dirp, filepath.c_str());
			} else {
				std::cerr << "opendir(): " << filepath.c_str() << ": " << strerror(errno) << std::endl;
			}
		} else {
			if (remove(filepath.c_str()) == -1) {
				std::cerr << "remove() file: " << filepath.c_str() << ": " << strerror(errno) << std::endl;
			}
		}
	}
	if (remove(path.c_str()) == -1) {
		std::cerr << "remove() dir: " << path.c_str() << ": " << strerror(errno) << std::endl;
	}

}

void Response::handleDeleteRequest(Request const & req)
{
	struct stat st;
	DIR * dirp = NULL;

	errno = 0;
	if (lstat(req.getFilename().c_str(), &st) == -1) {
		if (errno == ENOTDIR) {
			throw StatusCodeException(HttpStatus::Conflict, _location);
		} else {
			throw StatusCodeException(HttpStatus::NotFound, _location);
		}
	}

	if (S_ISDIR(st.st_mode)) {
		if (req.getRequestTarget().at(req.getRequestTarget().length() - 1) != '/') {
			throw StatusCodeException(HttpStatus::Conflict, _location);
		} else {
			if ((dirp = opendir(req.getFilename().c_str()))) {
				deleteDirectoryFiles(dirp, req.getFilename());
			}
		}
	} else {
		remove(req.getFilename().c_str());
	}

	if (errno) {
		perror("");
	}
	if (errno == ENOENT || errno == ENOTDIR || errno == ENAMETOOLONG) {
		throw StatusCodeException(HttpStatus::NotFound, _location);
    } else if (errno == EACCES || errno == EPERM) {
		throw StatusCodeException(HttpStatus::Forbidden, _location);
    } else if (errno == EEXIST) {
		throw StatusCodeException(HttpStatus::MethodNotAllowed, _location);
    } else if (errno == ENOSPC) {
		throw StatusCodeException(HttpStatus::InsufficientStorage, _location);
    } else if (errno) {
		throw StatusCodeException(HttpStatus::InternalServerError, _location);
    } else {
		throw StatusCodeException(HttpStatus::NoContent, _location);
	}

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

	// debug << this->_status << std::endl;
	response << "HTTP/1.1 " << this->_status << " " << reasonPhrase(this->_status) << CRLF;
	// debug << response.str() << std::endl;
	for (std::multimap<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << CRLF;
	}
	response << CRLF;
	return (response.str());
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

	std::stringstream ss;
	ssize_t size;

	buffer_body.resize(1024 + 7);
	if (!_is_cgi || (getBodySize() != 0 && getBodySize() != std::string::npos))
	{
		_body->read(buffer_body.data + 5, buffer_body.size - 7);
		size = ((*_body) ? buffer_body.size - 7 : _body->gcount());    
	}
	else
	{
		pollfd pfd = (pollfd){fd[0], POLLIN, 0};

		// buffer_body.resize(1024);

		int pret = poll(&pfd, 1, 0);
		if (pfd.revents == 0) {
			buffer_body.resize(0);
			return ;
		}
		if (pret == -1) {
			error("poll failed");
		}
		// debug << pret << " " << pfd.revents << std::endl;
		size = read(fd[0], buffer_body.data + 5, buffer_body.size - 7);
		debug << "bs:" << size << "\n" << std::endl;
		if (size == 0) {
			// debug << "Close " << fd[0] << std::endl;
			_is_cgi = false;
			closeFd();
		} else if (size == -1) {
			debug << "1" << std::endl;
			return;
		}
	}

	if (size == 0) {
		// close(fd[0]);
		// close(fd_body[1]);
		_send_end_chunk = true;
	}
	buffer_body.data[size + 7 - 2] = '\r';
	buffer_body.data[size + 7 - 1] = '\n';

	ss << std::setfill('0') << std::setw(3) << std::hex << size << CRLF;

	std::memcpy(buffer_body.data, ss.str().c_str(), ss.str().length());
	buffer_body.resize(ss.str().length() + size + 2);
}

std::stringstream * errorTemplate(const StatusCodeException & e) {
	std::stringstream * alloc = new std::stringstream("");
	std::stringstream & body = *alloc;

	if (e.getStatusCode() >= 400) {
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
	}

	return &body;
}


void Response::setErrorPage(const StatusCodeException & e, const Config * location) {
	_status = e.getStatusCode();
	// std::cerr << "S: " << this->_status << std::endl;

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
	}


	const std::map<int, std::string> & error_page = location->error_page.empty() ? _server->error_page : location->error_page;

	std::fstream * errPage = NULL;

	if (error_page.find(_status) != error_page.end()) {
		errPage = new std::fstream();
		std::string errPath = error_page.find(_status)->second;
		if (errPath[0] == '/') {
			errPage->open(errPath.c_str());
		} else {
			std::string filename = location->root;

			filename += errPath;
			errPage->open(filename.c_str());
		}
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


void Response::listingPage(const ListingException & e)
{
	std::stringstream header("");
	// std::stringstream *body = _body;

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
	*_body << "<!DOCTYPE html>\n" ;
	*_body << "<html>\n";
	*_body << "<head><title>Index of " << e.getReqTarget() << "</title></head>\n";
	*_body << "<body bgcolor=\"white\">\n";
	*_body << "<h1>Index of " << e.getReqTarget() << "</h1><hr><pre><a href=\"" << e.getReqTarget().substr(0, e.getReqTarget().find_last_of('/', e.getReqTarget().length() - 2)) << "/\">../</a>\n";

	if ((dir = opendir (e.what())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			struct stat attr;
			char format[250];
			std::string file_path = e.getPath() + std::string(ent->d_name);
			if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name) || !(stat(file_path.c_str(), &attr) == 0 && S_ISDIR(attr.st_mode)))
				continue ;
			*_body << "<a href=\"" << ent->d_name << "/\">" << ent->d_name << "/</a> " << std::setw(69 - strlen(ent->d_name)) << getFileCreationTime(file_path.c_str(), format);
			*_body << "                   -\n";
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
			*_body << "<a href=\"" << ent->d_name << "\">" << ent->d_name << "</a> " << std::setw(70 - strlen(ent->d_name)) << getFileCreationTime(file_path.c_str(), format);
			*_body << std::setw(20);
			*_body << attr.st_size;
			*_body << "\n";
		}
		closedir (dir);
	}
	*_body << "</pre><hr></body>\n</html>\n";
	// header << "Content-Length: " << (*body).str().length() << CRLF << CRLF;
	// header << (*body).str();
	// _body = body;
	// return header.str();
}

bool Response::is_cgi() const
{
	return this->_is_cgi;
}

bool Response::isSendingBodyFinished(const Request & request) const
{
	return request.getBodySize() == (unsigned long)sent_body || request.getBodySize() == 0;
}

void Response::set_cgi_body(const Request & request)
{
	char buff[BUFFER_SIZE] = {0};
	ssize_t ret;
	// close(fd_body[0]);
	// sent_body = request.getBody()->tellg();
	// std::cerr << "tellg " << request.getBody()->tellg() << std::endl;
	// std::cerr  << "sent " << sent_body << "/" << request.getBodySize() << std::endl;
	// while (sent_body < request.getBodySize()){

	pollfd pfd = (pollfd){fd_body[1], POLLOUT, 0};

	int pret = poll(&pfd, 1, 0);
	if (pfd.revents == 0) {
		return ;
	}

	if (pret == -1)
		error("poll failed");

	
	std::streampos n = std::min((std::streampos)(request.getBodySize() - request.getBody()->tellg()), (std::streampos)(BUFFER_SIZE));
	request.getBody()->read(buff, n);
	// std::streamsize n = 47; // request.getBody()->tellg();
	sent_body += n;
	// std::cerr  << "sent :" << n << "=>" <<  sent_body << "/" << request.getBodySize() << std::endl;
	if (n > 0) {
		ret = write(fd_body[1], buff, n);
		debug << ret << " " << errno << std::endl;
		if (ret == 0 || ret == -1) {
			debug << "2" << std::endl;

			throw StatusCodeException(HttpStatus::InternalServerError, _location);
		}
	}
	// }
	// close(fd_body[1]);
}

void Response::closeFdBody() {
	if (fd_body[1] != -1) {
		close(fd_body[1]);
		fd_body[1] = -1;
	}
}
void Response::closeFd() {
	if (fd[0] != -1) {
		debug << "Close FD: " << fd[0] << std::endl;
		close(fd[0]);
		fd[0] = -1;
	}
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
		debug << "3" << std::endl;
		throw StatusCodeException(HttpStatus::InternalServerError, _location);
	}
	if (!isCgiHeaderFinished())
	{
		char s[2050] = {0};
		// buff.resize(1025);
		// buff.data[1024] = 0;
		int ret = 0;
		// size_t pos;
		pollfd pfd = (pollfd){fd[0], POLLIN, 0};
		int pret = poll(&pfd, 1, 0);
		if (pfd.revents == 0 || !(pfd.revents & POLLIN)) {
			return ;
		}
		if(pret == -1)
			error("poll failed");
		debug << pret << " " << pfd.revents << std::endl;
		ret = read(fd[0], s, 2049);
		debug << ret << "\n" << s << std::endl;

		if (ret == -1) {
			throw StatusCodeException(HttpStatus::InternalServerError, _location);
		} else if (ret != 0) {
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
		}
		if (isCgiHeaderFinished())
		{
			// buffer_body.setData(temp.substr(pos).c_str(), temp.substr(pos).length());
			std::string sub = temp.substr(pos);
			_body->write(sub.c_str(), sub.length());
			setBodySize(sub.length());
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

bool Response::isEndChunkSent() const {
	return _send_end_chunk;
}

void Response::setEndChunkSent(bool isSent) {
	_send_end_chunk = isSent;
}

void Response::setHeaderSent(bool is_sent) {
	_send_header = is_sent;
}
void Response::setBodySent(bool is_sent) {
	_send_body = is_sent;
}

bool Response::isHeaderSent() const {
	return _send_header;
}
bool Response::isBodySent() const {
	return _send_body;
}

bool Response::isRequestHandled() const {
	return _is_request_handled;
}
