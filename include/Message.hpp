#ifndef MESSAGE_HPP
# define MESSAGE_HPP


#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <fstream>

// #include "webserv.hpp"

class Message
{
protected:
	std::map<std::string, std::string> _headers;
	std::iostream * _body;
	bool _isBodyFile;
public:
	Message();
	Message(const std::string & message);
	~Message();
	const std::map<std::string, std::string> & getHeader() const;
	const std::string getHeader(const std::string & key) const;
	const std::iostream * getBody() const; 
	void insert_header(std::string const & key, std::string const & val);

	// std::streampos getBodySize() const;
};



#endif