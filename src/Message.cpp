#include "Message.hpp"

Message::Message() {
    _body = NULL;
    _body_size = 0;
    reset();
}

// static void trim(std::string & str) {
//     int first = str.find_first_not_of(" \n\r\t");
//     int last = str.find_last_not_of(" \n\r\t");

//     if (first == std::string::npos)
//         first = 0;
//     str = str.substr(first, last + 1);
// }

// Message::Message(const std::string & message) {

//     size_t start = 0;
//     size_t end ;
//     int stat = 0;

//     while ((end = message.find('\n', start)) != std::string::npos) {
//         std::string str = message.substr(start, end - start);
//         trim(str);
//         if (stat == 1 && str == "")
//             ++stat;
//         start = end + 1;
//         if (stat == 0) {
//             _start_line = str;
//             ++stat;
//         } else if(stat == 1) {
//             size_t column_pos = str.find(':');
//             if (column_pos != std::string::npos) {

//                 std::string key = str.substr(0, column_pos);
//                 trim(key);
//                 std::string value = str.substr(column_pos + 1);
//                 trim(value);

//                 _headers.insert(std::make_pair(key, value));
//             }

//         }
//         if (stat == 2) {
//             break;
//         }
//     }
//     if (start < message.length()) {
//         _body = message.substr(start);
//     }

// }

Message::~Message() {
    if (_isBodyFile) {
        ((std::fstream *) _body)->close();
    }
    delete _body;
}

const std::multimap<std::string, std::string, ci_less> & Message::getHeader() const {
    return this->_headers;
}

std::iostream * Message::getBody() const {
    return this->_body;
}

const std::string Message::getHeader(const std::string & key) const
{
    std::multimap<std::string, std::string>::const_iterator it =  this->_headers.find(key);
    if (it == _headers.end())
        return ("");
    return it->second;
}

bool isKeyEqual(std::string s1, std::string s2)
{
    std::transform(s1.begin(), s1.end(), s1.begin(), tolower);
    std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
    return s1 == s2;
}

void Message::insert_header(std::string const & key, std::string const & val)
{
    if (isKeyEqual(key, "set-cookie") || (_headers.find(key) == _headers.end()))
        this->_headers.insert(std::pair<std::string, std::string>(key, val));
    
}

void Message::setHeader(const std::string & key, const std::string & val) {
    std::multimap<std::string, std::string>::iterator it = _headers.find(key);
    if (isKeyEqual(key, "set-cookie") || (it == _headers.end()))
        this->_headers.insert(std::pair<std::string, std::string>(key, val));
    else if (it != _headers.end())
        it->second = val;
}

const Config * Message::getServerConfig() const {
	return this->_server;
}

void Message::setServerConfig(const Config * config) {
	this->_server = config;
    if (!_location)
        _location = _server;
}

const Config * Message::getLocation() const {
    return _location;
}

void Message::reset() {
    _headers.clear();
    if (_body)
	    delete _body;
    _body = new std::stringstream();
	_isBodyFile = false;

	_server = NULL;
	_location = NULL;
}


size_t Message::getBodySize() const {
    return _body_size;
}

void Message::setBodySize(size_t size) {
    _body_size = size;
}
