#include "Response.hpp"
#include "debug.hpp"
#include <algorithm>

Response::Response()
{
	reset();
}

Response::Response(Response const & src)
{
	reset();
	*this = src;
}

Response::~Response(){
}

Response &Response::operator=(Response const & src)
{
	buffer = src.buffer;
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
	buffer.resize(0);
	sent_body = 0;
	_isCgiHeaderFinished = false;
	cgiHeader.str("");

	fd_body[1] = -1;
	fd[0] = -1;
	_send_end_chunk = false;

	_is_request_handled = false;
	_read_body_finished = false;

}

void Response::handleRequest(Request const & req, Socket const & sock) {
	this->_server = req.getServerConfig();
	this->_location = req.getLocation();
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
	std::string file_path = req.getFilePath();
	char buff[101] = {0};
	_is_cgi = true;
	char * const ar[4] = {const_cast<char *>(_location->cgi.c_str()), const_cast<char *>(file_path.c_str()), NULL};
	pipe(fd);
	pipe(fd_body);
	
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

		size_t n = req.getRequestTarget().find_first_of('?');
		n = n == std::string::npos ? req.getRequestTarget().length() : n + 1;
		v.push_back(strdup((std::string("CONTENT_LENGTH") + "=" + Utils::to_str(req.getBodySize())).c_str()));

		v.push_back(strdup((std::string("CONTENT_TYPE") + "=" + req.getHeader("Content-Type")).c_str()));
		v.push_back(strdup((std::string("GATEWAY_INTERFACE") + "=CGI/1.1").c_str()));

		v.push_back(strdup((std::string("PATH_INFO") + "=" + req.getRequestTarget().substr(0, n)).c_str()));
		v.push_back(strdup((std::string("PATH_TRANSLATED") + "=" + file_path).c_str()));
		v.push_back(strdup((std::string("REMOTE_ADDR") + "=" + sock.getHost()).c_str()));
		v.push_back(strdup((std::string("REMOTE_HOST") + "=" + req.getHeader("host").substr(0, req.getHeader("host").find_first_of(':'))).c_str()));

		v.push_back(strdup((std::string("SERVER_NAME") + "=" + req.getLocation()->server_name).c_str()));
		v.push_back(strdup((std::string("SERVER_PORT") + "=" + Utils::to_str(req.getLocation()->port)).c_str()));
		v.push_back(strdup((std::string("SERVER_PROTOCOL") + "=HTTP/1.1").c_str()));
		v.push_back(strdup((std::string("SERVER_SOFTWARE") + "=" + SERVER_NAME).c_str()));

		v.push_back(strdup((std::string("QUERY_STRING") + "=" + req.getRequestTarget().substr(n)).c_str()));
		v.push_back(strdup((std::string("HTTP_COOKIE") + "=" + req.getHeader("Cookie")).c_str()));
		v.push_back(strdup((std::string("REDIRECT_STATUS") + "=").c_str()));
		v.push_back(NULL);
		close(fd[0]);
		close(fd_body[1]);
		dup2(fd[1], 1);
		dup2(fd_body[0], 0);
		if (execve(ar[0], ar, const_cast<char * const *>(v.data())) == -1) {
			std::cerr << "Child Closed\n";
			exit(42);
		}
	}
	close(fd[1]);
	close(fd_body[0]);
	req.getBody()->seekg(0, req.getBody()->beg);
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(file_path.c_str());
	if (type)
		insert_header("Content-Type", type);
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");
}

void Response::handleGetRequest(Request const & req)
{
	std::string file_path = req.getFilePath();
	struct stat fileStat;

	std::fstream * file = new std::fstream();
	file->open(file_path.c_str());
		delete _body;
		_body = file;
	stat (file_path.c_str(), &fileStat);
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(fileStat));
	insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(file_path.c_str());
	if (type)
		insert_header("Content-Type", type);
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");
}

void Response::handlePostRequest(Request const & req)
{
	std::string file_path = req.getFilePath();
	struct stat fileStat;

	std::fstream * file = new std::fstream();
	file->open(file_path.c_str());
	delete _body;
	_body = file;
	stat (file_path.c_str(), &fileStat);
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(fileStat));
	insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(file_path.c_str());
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
	if (lstat(req.getFilePath().c_str(), &st) == -1) {
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
			if ((dirp = opendir(req.getFilePath().c_str()))) {
				deleteDirectoryFiles(dirp, req.getFilePath());
			}
		}
	} else {
		remove(req.getFilePath().c_str());
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

	response << "HTTP/1.1 " << this->_status << " " << reasonPhrase(this->_status) << CRLF;
	for (std::multimap<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << CRLF;
	}
	response << CRLF;
	return (response.str());
}

void	Response::readFile() {

	char buff[BUFFER_SIZE];

	std::stringstream ss;
	ssize_t size;
	if (!_is_cgi || (getBodySize() != 0 && getBodySize() != std::string::npos))
	{
		_body->read(buff, BUFFER_SIZE);
		size = ((*_body) ? BUFFER_SIZE : _body->gcount()); 
	}
	else
	{
		pollfd pfd = (pollfd){fd[0], POLLIN, 0};

		int pret = poll(&pfd, 1, 0);
		if (pfd.revents == 0) {
			return ;
		}
		if (pret == -1) {
			error("poll failed");
		}
		size = read(fd[0], buff, BUFFER_SIZE);
		if (size == 0) {
			_is_cgi = false;
			closeFd();
		} else if (size == -1) {
			return;
		}
	}

	if (size == 0) {
		_read_body_finished = true;
	}

	ss << std::setfill('0') << std::setw(3) << std::hex << size << CRLF;
	buffer.push(ss.str().c_str(), ss.str().length());
	if (size) {
		buffer.push(buff, size);
	}

	buffer.push("\r\n", 2);
	if (size && size < BUFFER_SIZE && buffer.size < BUFFER_SIZE) {
		readFile();
	}
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

	insert_header("Connection", "keep-alive");
	insert_header("Content-Type", "text/html");
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);

	if (e.getLocation() != ""){
		insert_header("Location", e.getLocation());
	}


	const std::map<int, std::string> & error_page = location->error_page.empty() ? _server->error_page : location->error_page;

	std::fstream * errPage = NULL;

	if (error_page.find(_status) != error_page.end()) {
		errPage = new std::fstream();
		std::string errPath = error_page.find(_status)->second;
		if (errPath[0] == '/' || (errPath[0] == '.' && errPath[1] == '/')) {
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
	pollfd pfd = (pollfd){fd_body[1], POLLOUT, 0};

	int pret = poll(&pfd, 1, 0);
	if (pfd.revents == 0) {
		return ;
	}

	if (pret == -1)
		error("poll failed");

	
	std::streampos n = std::min((std::streampos)(request.getBodySize() - request.getBody()->tellg()), (std::streampos)(BUFFER_SIZE));
	request.getBody()->read(buff, n);
	sent_body += n;
	if (n > 0) {
		ret = write(fd_body[1], buff, n);
		if (ret == 0 || ret == -1) {
			throw StatusCodeException(HttpStatus::InternalServerError, _location);
		}
	}
}

void Response::closeFdBody() {
	if (fd_body[1] != -1) {
		close(fd_body[1]);
		fd_body[1] = -1;
	}
}
void Response::closeFd() {
	if (fd[0] != -1) {
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

void Response::setCgiHeaderFinished(bool stat)
{
	_isCgiHeaderFinished = stat;
}

void Response::readCgiHeader()
{
	std::string temp;
	size_t pos;

	int status = 0;
	int ret = waitpid(pid, &status, WNOHANG);
	// std::cerr << "ret: " << ret << "\n";

	if (ret == pid && WEXITSTATUS(status) == 42) {
		throw StatusCodeException(HttpStatus::InternalServerError, _location);
	}
	// exit(EXIT_SUCCESS);
	if (!isCgiHeaderFinished())
	{
		char s[2050] = {0};
		int ret = 0;
		pollfd pfd = (pollfd){fd[0], POLLIN, 0};
	
		int pret = poll(&pfd, 1, 0);
	
		if (pfd.revents == 0 || !(pfd.revents & POLLIN)) {
			return ;
		}
		if(pret == -1)
			error("poll failed");
		// debug << pret << " " << pfd.revents << std::endl;
		ret = read(fd[0], s, 2049);

		if (ret == -1) {
			throw StatusCodeException(HttpStatus::InternalServerError, _location);
		} else if (ret != 0) {
			cgiHeader << s;
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
				std::string str = line.substr(line.find_first_of(':') + 1);
				insert_header(line.substr(0, start), trim(str));
			}
			std::multimap<std::string, std::string>::const_iterator it = _headers.find("Status");
			if (it != _headers.end()) {
				_status = (HttpStatus::StatusCode)atoi(it->second.c_str());
			}
		}
	}
}

bool Response::isEndChunkSent() const {
	return _send_end_chunk;
}

void Response::setEndChunkSent(bool isSent) {
	_send_end_chunk = isSent;
}

bool Response::isReadBodyFinished() const {
	return _read_body_finished;
}

bool Response::isRequestHandled() const {
	return _is_request_handled;
}
