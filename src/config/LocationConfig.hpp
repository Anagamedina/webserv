#ifndef WEBSERV_LOCATIONCONFIG_HPP
#define WEBSERV_LOCATIONCONFIG_HPP

#include <iostream>
#include <string>
#include <vector>

#include "ConfigUtils.hpp"

class LocationConfig
{
private:
	std::string path_; // /upload, /, /api, etc.
	std::string root_; // root ./www
	std::vector<std::string> index_; // index index.html index.htm ...
	std::vector<std::string> allowed_methods_; // methods GET POST DELETE
	bool autoindex_; // autoindex on/off
	std::string upload_store_; // upload_store ./uploads
	std::string redirect_; // return 301 /new-path (optional)

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
	void setRedirect(const std::string& redirect);

	// Getters
	const std::string& getPath() const;
	const std::string& getRoot() const;
	const std::vector<std::string>& getIndexes() const;
	const std::vector<std::string>& getMethods() const;
	bool getAutoIndex() const;
	const std::string& getUploadStore() const;
	const std::string& getRedirect() const;

	// Validation
	bool isMethodAllowed(const std::string& method) const;
	// bool isValid() const;

	// Debug
	void print() const;
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
	if (!location.getRedirect().empty())
	{
		os << "\tRedirect: " << location.getRedirect() << "\n";
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
