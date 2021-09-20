#ifndef MESSAGE_HPP
# define MESSAGE_HPP


#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <fstream>

// #include "webserv.hpp"

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

	std::map<std::string, std::string, ci_less> _headers;
	std::iostream * _body;
	bool _isBodyFile;
public:
	Message();
	Message(const std::string & message);
	~Message();
	const std::map<std::string, std::string, ci_less> & getHeader() const;
	const std::string getHeader(const std::string & key) const;
	const std::iostream * getBody() const; 
	void insert_header(std::string const & key, std::string const & val);

	// std::streampos getBodySize() const;
};



#endif