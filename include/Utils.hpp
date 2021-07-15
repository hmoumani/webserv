#ifndef UTILS_HPP
# define UTILS_HPP
# include <iostream>

namespace Utils {
	/**
	* FUNCTION: getFilePath 
	* USE: Returns the path from a given file path
	* @param path: The path of the file
	* @return: The path from the given file path
	*/
	
	inline std::string getFilePath(const std::string & path) {
		size_t pathEnd = path.find_last_of("/\\");
		std::string pathName = pathEnd == std::string::npos ? path : path.substr(0, pathEnd);
		return pathName;
	}

	/**
	* FUNCTION: getFileName 
	* USE: Returns the file name from a given file path
	* @param path: The path of the file
	* @return: The file name from the given file path
	*/
	inline std::string getFileName(const std::string & path) {
		size_t fileNameStart = path.find_last_of("/\\");
		std::string fileName = fileNameStart == std::string::npos ? path : path.substr(fileNameStart + 1);
		return fileName;
	}

	inline int op_custom(unsigned char c) { 
		return static_cast<unsigned char>(std::tolower(c)); 
	}

	/**
	* FUNCTION: getFileExtension 
	* USE: Returns the file extension from a given file path
	* @param path: The path of the file
	* @return: The file extension from the given file path
	*/
	inline std::string getFileExtension(const std::string& path) {
		std::string fileName = getFileName(path);
		size_t extStart = fileName.find_last_of('.');
		std::string ext = extStart == std::string::npos ? "" : fileName.substr(extStart + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), op_custom);
		return ext;
	}
	inline bool file_exists (const std::string& name) {
  		struct stat buffer;   
  		return (stat (name.c_str(), &buffer) == 0); 
	}

	/**
	* FUNCTION: fileStat 
	* USE: throw the appropriate exception for a given file
	* @param path: The path of the file
	* @return: void
	*/
	inline void	fileStat(std::string const & filename, struct stat buffer)
	{
		// struct stat buffer;   
		// stat (filename.c_str(), &buffer);
		if (!(buffer.st_mode & S_IROTH))
			throw StatusCodeException(HttpStatus::Forbidden);
		if (access( filename.c_str(), F_OK)){
			throw StatusCodeException(HttpStatus::NotFound);
		}
	}

	inline std::string	time_last_modification(struct stat buffer)
	{
		tm *ltm = gmtime(&buffer.st_mtime);

		std::ostringstream date;

		date << wday_name[ltm->tm_wday] << ", " << ltm->tm_mday << " "
			<< mon_name[ltm->tm_mon] << " " << (ltm->tm_year + 1900) << " " 
			<< (ltm->tm_hour) % 24 << ":" << ltm->tm_min << ":" << ltm->tm_sec << " GMT";
		return date.str();
	}

	inline std::string chuncked_transfer_encoding(std::string & body) {
		std::ostringstream chuncked_body;

		chuncked_body << std::hex << body.size() << CRLF
					<< body << CRLF
					<< "0" << CRLF
					<< CRLF;
		return chuncked_body.str();
	}

	inline std::string		getDate(void)
	{
		time_t now = time(0);
		tm *ltm = gmtime(&now);

		std::ostringstream date;

		date << wday_name[ltm->tm_wday] << ", " << ltm->tm_mday << " "
			<< mon_name[ltm->tm_mon] << " " << (ltm->tm_year + 1900) << " " 
			<< (ltm->tm_hour) % 24 << ":" << ltm->tm_min << ":" << ltm->tm_sec << " GMT";
		return date.str();
	}

	inline int findNthOccur(std::string const & str, char ch, int N)
	{
		int occur = 0;

		for (int i = 0; i < str.length(); i++) {
			if (str[i] == ch) {
				occur += 1;
			}
			if (occur == N)
				return i;
		}
		return -1;
	}

	inline std::string getRoute(std::string const & str)
	{
		int pos = findNthOccur(str, '/', 3);
		if (pos == -1)
			return ("");
		return str.substr(pos);
	}
}

#endif