#ifndef MESSAGE_HPP
# define MESSAGE_HPP


#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include "Config.hpp"

class Config;
struct ci_less : std::binary_function<std::string, std::string, bool>
  {
    // case-independent (ci) compare_less binary function
    struct nocase_compare : public std::binary_function<unsigned char,unsigned char,bool> 
    {
      bool operator() (const unsigned char& c1, const unsigned char& c2) const {
          return tolower (c1) < tolower (c2); 
      }
    };
    bool operator() (const std::string & s1, const std::string & s2) const {
      return std::lexicographical_compare 
        (s1.begin (), s1.end (),   // source range
        s2.begin (), s2.end (),   // dest range
        nocase_compare ());  // comparison
    }
};

class Message
{
protected:

	std::multimap<std::string, std::string, ci_less> _headers;
	std::iostream * _body;
	size_t		_body_size;
	bool _isBodyFile;

	const Config * _server;
	const Config * _location;

public:
	Message();
	Message(const std::string & message);
	~Message();
	const std::multimap<std::string, std::string, ci_less> & getHeader() const;
	const std::string getHeader(const std::string & key) const;
	std::iostream * getBody() const; 
	void insert_header(std::string const & key, std::string const & val);
	size_t getBodySize() const; 
	void setBodySize(size_t size); 

	void setServerConfig(const Config * config);
	const Config * getServerConfig() const;

	const Config * getLocation() const;
	void reset();
	void setHeader(const std::string & key, const std::string & val);

};



#endif