#ifndef WEBSERV_LOCATIONCONFIG_HPP
#define WEBSERV_LOCATIONCONFIG_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

/**
 * @brief Represents the configuration for a specific location within a server
 * block.
 *
 * Handles parsing and storing settings defined in a 'location block',
 * such as:
 * - content root directory
 * - allowed HTTP methods (GET, POST, DELETE, HEAD)
 * - default index files in a vector
 * - autoindex status boolean
 * - file upload directory
 * - HTTP redirection
 * - CGI handlers like a map
 */
class LocationConfig {
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
  void setRedirectParamCount(int count);
  void setMaxBodySize(size_t size);
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
  int getRedirectParamCount() const;
  size_t getMaxBodySize() const;
  std::string getCgiPath(const std::string& extension) const;
  const std::map<std::string, std::string>& getCgiHandlers() const;

  // Validation
  bool isMethodAllowed(const std::string& method) const;

  // Debug Helper
  void print() const;

 private:
  std::string path_;
  std::string root_;
  std::vector<std::string> indexes_;
  std::vector<std::string> allowed_methods_;
  bool autoindex_;
  std::string upload_store_;
  int redirect_code_;
  std::string redirect_url_;
  int redirect_param_count_;
  size_t max_body_size_;
  std::map<std::string, std::string> cgi_handlers_;
};

std::ostream& operator<<(std::ostream& os,
						const LocationConfig& location);

#endif  // WEBSERV_LOCATIONCONFIG_HPP
