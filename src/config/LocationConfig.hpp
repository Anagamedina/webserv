#ifndef WEBSERV_LOCATIONCONFIG_HPP
#define WEBSERV_LOCATIONCONFIG_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ConfigUtils.hpp"

class LocationConfig
{
public:
	LocationConfig();
	LocationConfig(const LocationConfig& other);
	LocationConfig& operator=(const LocationConfig& other);
	~LocationConfig();

	// Setters
	void setPath(const std::string& path);
	void setRoot(const std::string& root);
	void addIndex(const std::string& index);
	void addMethod(const std::string& method);
	void setAutoIndex(bool autoindex);
	void setUploadStore(const std::string& store);
	void setRedirectCode(int integerCode);
	void setRedirectUrl(const std::string& redirectUrl);
	void addCgiHandler(const std::string& extension,
						const std::string& binaryPath);

	// Getters
	const std::string& getPath() const;
	const std::string& getRoot() const;
	const std::vector<std::string>& getIndexes() const;
	const std::vector<std::string>& getMethods() const;
	bool getAutoIndex() const;
	const std::string& getUploadStore() const;
	int getRedirectCode() const;
	const std::string& getRedirectUrl() const;
	std::string getCgiPath(const std::string& extension) const;
	const std::map<std::string, std::string>& getCgiHandlers() const;

	// Validation
	bool isMethodAllowed(const std::string& method) const;

	// Debug
	void print() const;

private:
	std::string path_; // /upload, /, /api, etc.
	std::string root_; // root ./www
	std::vector<std::string> indexes_; // index index.html index.htm ...
	std::vector<std::string> allowed_methods_; // methods GET POST DELETE
	bool autoindex_; // autoindex on/off
	std::string upload_store_; // upload_store ./uploads
	int redirect_code_; // return 301 /new-path (optional)
	std::string redirect_url_; // /new-path (optional)
	std::map<std::string, std::string> cgi_handlers_;
};

inline std::ostream& operator<<(std::ostream& os,
								const LocationConfig& location)
{
	os << config::colors::cyan << "Locations info:\n"
		<< config::colors::reset << "\tLocation Path: " << location.getPath()
		<< "\n"
		<< "\tRoot: " << location.getRoot() << "\n"
		<< "\tAutoindex: " << (location.getAutoIndex() ? "on" : "off") << "\n";

	if (!location.getUploadStore().empty())
	{
		os << "\tUpload Store: " << location.getUploadStore() << "\n";
	}
	if (!location.getRedirectCode())
	{
		os << "\tRedirect: " << location.getRedirectCode() << "\n";
	}

	const std::vector<std::string>& indexes = location.getIndexes();

	os << "\tIndexes: ";
	for (size_t i = 0; i < indexes.size(); ++i)
	{
		os << indexes[i] << (i == indexes.size() - 1 ? "" : ", ");
	}
	// Imprimir Métodos
	const std::vector<std::string>& methods = location.getMethods();
	os << "\n\tMethods: ";
	for (size_t i = 0; i < methods.size(); ++i)
	{
		os << methods[i] << (i == methods.size() - 1 ? "" : ", ");
	}
	os << "\n";

	return os;
}

#endif // WEBSERV_LOCATIONCONFIG_HPP
