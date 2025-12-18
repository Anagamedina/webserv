#ifndef WEBSERV_LOCATIONCONFIG_HPP
#define WEBSERV_LOCATIONCONFIG_HPP

#include <string>
#include <vector>

class LocationConfig
{
private:
	std::string path_;                    // /upload, /, /api, etc.
	std::string root_;                    // root ./www
	std::string index_;                   // index index.html
	std::vector<std::string> methods_;    // methods GET POST DELETE
	bool autoindex_;                      // autoindex on/off
	std::string upload_store_;            // upload_store ./uploads
	std::string redirect_;                // return 301 /new-path (optional)

public:
	
	// Validation
	bool isMethodAllowed(const std::string& method) const;
	bool isValid() const;
	
	// Debug
	void print() const;
};

#endif //WEBSERV_LOCATIONCONFIG_HPP