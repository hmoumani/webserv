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
	// std::cout << "lol: " << location-> << std::endl;
	// if (Utils::getFileExtension(filename) == ".php")
	if (server->location.find(Utils::getFileExtension(filename)) != server->location.end())
	{
		char buff[101] = {0};
		_is_cgi = true;
		file.close();

		// std::map<std::string, std::string> env;
		gethostname(buff, 100);
		// env.insert(std::make_pair("REQUEST_METHOD", "GET"));
		// env.insert(std::make_pair("PATH", getenv("PATH")));
		// env.insert(std::make_pair("TERM", getenv("TERM")));
		// env.insert(std::make_pair("HOME", getenv("HOME")));
		// env.insert(std::make_pair("HOSTNAME", hostname));
		// env.insert(std::make_pair("QUERY_STRING", req.getRequestTarget().substr(req.getRequestTarget().find_first_of('?'))));

		
		char * const ar[4] = {const_cast<char *>(location->cgi.c_str()), const_cast<char *>(request_path.c_str()), NULL};
		pipe(fd);
		pid = fork();
		if (pid == 0)
		{
			std::vector<const char *> v;
			v.push_back(strdup((std::string("REQUEST_METHOD") + "=" + "GET").c_str()));
			v.push_back(strdup((std::string("PATH") + "=" + getenv("PATH")).c_str()));
			v.push_back(strdup((std::string("TERM") + "=" + getenv("TERM")).c_str()));
			v.push_back(strdup((std::string("HOME") + "=" + getenv("HOME")).c_str()));
			gethostname(buff, 100);
			v.push_back(strdup((std::string("HOSTNAME") + "=" + buff).c_str()));
			getlogin_r(buff, 100);
			v.push_back(strdup((std::string("USER") + "=" + buff).c_str()));
			v.push_back(strdup((std::string("SCRIPT_FILENAME") + "=" + request_path).c_str()));
			size_t n = req.getRequestTarget().find_first_of('?') + 1;
			n = n == std::string::npos ? req.getRequestTarget().length() : n;
			v.push_back(strdup((std::string("QUERY_STRING") + "=" + req.getRequestTarget().substr(n)).c_str()));
			v.push_back(NULL);		
			close(fd[0]);
			dup2(fd[1], 1);
			execve(ar[0], ar, const_cast<char * const *>(v.data()));
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
	if (_is_cgi)
		type = "text/html; charset=UTF-8";
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

std::string Response::HeadertoString()
{
	std::ostringstream response("");
	// Buffer buff;

	response << "HTTP/1.1 " << this->status << " " << reasonPhrase(this->status) << CRLF;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << CRLF;
	}

	if (is_cgi())
	{
		char s[2050] = {0};
		// buff.resize(1025);
		// buff.data[1024] = 0;
		int ret = 0, total = 0;
		size_t pos;
		while (true)
		{
			ret = read(fd[0], s + total, 2049 - total);
			total += ret;
			s[total] = 0;
			if ((pos = std::string(s).find("\r\n\r\n")) != std::string::npos || (pos = std::string(s).find("\n\n")) != std::string::npos )
				break ;
		}
		// size_t pos = std::string(s).find("\r\n\r\n");
		std::string token = std::string(s).substr(0, pos);
		buffer_body.setData(std::string(s).substr(pos).c_str(), std::string(s).substr(pos).length());
		response << token << CRLF;

		// strcpy(buffer_body.data, std::string(buff.data).substr(0, pos).c_str());
	}
	// if (is_cgi())
	// {
	// 	while (true)
	// 	{
	// 		int ret = read(fd[0], &c, 1);
	// 		std::cout << ret << std::endl;
	// 		if ((c == '\n' && header_line.length() == 0) || ret <= 0)
	// 			break ;
	// 		if (c == '\n')
	// 		{
	// 			response << header_line << CRLF;
	// 			header_line.clear();
	// 		}
	// 		header_line += c;
	// 	}
	// }
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
		header << "Location: " << e.getLocation() <<  CRLF;
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
	std::ostringstream header("");
	std::ostringstream body("");

	header << "HTTP/1.1 " << 200 << " " << "OK" << CRLF;
	header << "Connection: keep-alive" << CRLF;
	header << "Content-Type: text/html" << CRLF;
	header << "Date: " << Utils::getDate() <<  CRLF;
	header << "Server: " << SERVER_NAME << CRLF ;

	DIR *dir;
	struct dirent *ent;
	body << "<!DOCTYPE html>\n" ;
	body << "<html>\n";
	body << "<head><title>Index of " << e.getReqTarget() << "</title></head>\n";
	body << "<body bgcolor=\"white\">\n";
	body << "<h1>Index of " << e.getReqTarget() << "</h1><hr><pre><a href=\"" << e.getReqTarget().substr(0, e.getReqTarget().find_last_of('/', e.getReqTarget().length() - 2)) << "/\">../</a>\n";

	if ((dir = opendir (e.what())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			struct stat attr;
			char format[250];
			std::string file_path = e.getPath() + std::string(ent->d_name);
			if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name) || !(stat(file_path.c_str(), &attr) == 0 && S_ISDIR(attr.st_mode)))
				continue ;
			body << "<a href=\"" << ent->d_name << "/\">" << ent->d_name << "/</a> " << std::setw(69 - strlen(ent->d_name)) << getFileCreationTime(file_path.c_str(), format);
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
	header << "Content-Length: " << body.str().length() << CRLF << CRLF;
	header << body.str();
	return header.str();
}

bool Response::is_cgi() const
{
	return this->_is_cgi;
}