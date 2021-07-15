#include "Message.hpp"

Message::Message(){}

static void trim(std::string & str) {
    int first = str.find_first_not_of(" \n\r\t");
    int last = str.find_last_not_of(" \n\r\t");

    if (first == std::string::npos)
        first = 0;
    str = str.substr(first, last + 1);
}

Message::Message(const std::string & message) {

    size_t start = 0;
    size_t end ;
    int stat = 0;

    while ((end = message.find('\n', start)) != std::string::npos) {
        std::string str = message.substr(start, end - start);
        trim(str);
        if (stat == 1 && str == "")
            ++stat;
        start = end + 1;
        if (stat == 0) {
            _start_line = str;
            ++stat;
        } else if(stat == 1) {
            size_t column_pos = str.find(':');
            if (column_pos != std::string::npos) {

                std::string key = str.substr(0, column_pos);
                trim(key);
                std::string value = str.substr(column_pos + 1);
                trim(value);

                _headers.insert(std::make_pair(key, value));
            }

        }
        if (stat == 2) {
            break;
        }
    }
    if (start < message.length()) {
        _body = message.substr(start);
    }

}

Message::~Message() {
}

const std::string & Message::getStartLine() const { return this->_start_line; }
const std::unordered_map<std::string, std::string> & Message::getHeader() const { return this->_headers; }
const std::string & Message::getBody() const { return this->_body; };
const std::string Message::getHeader(const std::string & key) const
{
    std::unordered_map<std::string, std::string>::const_iterator it =  this->_headers.find(key);
    if (it == _headers.end())
        return ("");
    return it->second;
}

void Message::insert_header(std::string const & key, std::string const & val)
{
    this->_headers.insert(std::pair<std::string, std::string>(key, val));
}



