#ifndef MESSAGE_CPP
# define MESSAGE_CPP


#include <iostream>
#include <string>
#include <unordered_map>
// #include "webserv.hpp"

class Message
{
protected:
	std::string _start_line;
	std::unordered_map<std::string, std::string> _headers;
	std::string _body;

public:
	Message();
	Message(const std::string & message);
	~Message();
	const std::string & getStartLine() const;
	const std::unordered_map<std::string, std::string> & getHeader() const;
	const std::string & getHeader(const std::string & key) const;
	const std::string & getBody() const; 
	void insert_header(std::string const & key, std::string const & val);
};



#endif