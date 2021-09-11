#ifndef MESSAGE_HPP
# define MESSAGE_HPP


#include <iostream>
#include <string>
#include <map>
// #include "webserv.hpp"

class Message
{
protected:
	std::string _start_line;
	std::map<std::string, std::string> _headers;
	std::string _body;

public:
	Message();
	Message(const std::string & message);
	~Message();
	const std::string & getStartLine() const;
	const std::map<std::string, std::string> & getHeader() const;
	const std::string getHeader(const std::string & key) const;
	const std::string & getBody() const; 
	void insert_header(std::string const & key, std::string const & val);
};



#endif