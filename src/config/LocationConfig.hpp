#ifndef WEBSERV_LOCATIONCONFIG_HPP
#define WEBSERV_LOCATIONCONFIG_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ConfigUtils.hpp"

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
  std::string getCgiPath(const std::string& extension) const;
  const std::map<std::string, std::string>& getCgiHandlers() const;

  // Validation
  bool isMethodAllowed(const std::string& method) const;

  // Debug
  void print() const;

 private:
  std::string path_;                          // /upload, /, /api, etc.
  std::string root_;                          // root ./www
  std::vector<std::string> indexes_;          // index index.html index.htm ...
  std::vector<std::string> allowed_methods_;  // methods GET POST DELETE
  bool autoindex_;                            // autoindex on/off
  std::string upload_store_;                  // upload_store ./uploads
  int redirect_code_;                         // return 301 /new-path (optional)
  std::string redirect_url_;                  // return /new-path (optional)
  int redirect_param_count_;  // Number of params in return directive
  std::map<std::string, std::string> cgi_handlers_;
};

inline std::ostream& operator<<(std::ostream& os,
                                const LocationConfig& location) {
  os << config::colors::cyan << config::colors::bold << "Locations info:\n"
     << config::colors::reset << "\t" << config::colors::yellow
     << "Location Path: " << config::colors::reset << config::colors::green
     << location.getPath() << config::colors::reset << "\n"
     << "\t" << config::colors::yellow << "Root: " << config::colors::reset
     << config::colors::green << location.getRoot() << config::colors::reset
     << "\n"
     << "\t" << config::colors::yellow
     << "Autoindex: " << config::colors::reset;
  if (location.getAutoIndex()) {
    os << config::colors::green << "on";
  } else {
    os << config::colors::red << "off";
  }
  os << config::colors::reset << "\n";

  if (!location.getUploadStore().empty()) {
    os << "\t" << config::colors::yellow
       << "Upload Store: " << config::colors::reset << config::colors::green
       << location.getUploadStore() << config::colors::reset << "\n";
  } else {
    os << "\t" << config::colors::yellow
       << "Upload Store: " << config::colors::reset << config::colors::red
       << "empty" << config::colors::reset << "\n";
  }

  if (location.getRedirectCode() != -1) {
    os << "\t" << config::colors::yellow << "Return ("
       << location.getRedirectParamCount() << " parameter"
       << (location.getRedirectParamCount() == 1 ? "" : "s")
       << "): " << config::colors::reset;
    if (location.getRedirectParamCount() == 1) {
      os << config::colors::green << location.getRedirectUrl()
         << config::colors::reset << "\n";
    } else {
      os << config::colors::green << location.getRedirectCode() << " '"
         << location.getRedirectUrl() << "'" << config::colors::reset << "\n";
    }
  }

  const std::vector<std::string>& indexes = location.getIndexes();
  os << "\t" << config::colors::yellow << "Indexes: " << config::colors::reset;
  if (indexes.empty()) {
    os << config::colors::red << "empty" << config::colors::reset;
  } else {
    for (size_t i = 0; i < indexes.size(); ++i) {
      os << config::colors::green << indexes[i] << config::colors::reset
         << (i == indexes.size() - 1 ? "" : ", ");
    }
  }

  // Imprimir Métodos
  const std::vector<std::string>& methods = location.getMethods();
  os << "\n\t" << config::colors::yellow
     << "Methods: " << config::colors::reset;
  if (methods.empty()) {
    os << config::colors::red << "empty" << config::colors::reset;
  } else {
    for (size_t i = 0; i < methods.size(); ++i) {
      os << config::colors::green << methods[i] << config::colors::reset
         << (i == methods.size() - 1 ? "" : ", ");
    }
  }
  os << "\n";
  // ────────────────────────────────────────────────
  //           BONUS: CGI Handlers
  // ────────────────────────────────────────────────
  os << "\n\t" << config::colors::magenta << config::colors::bold
     << "CGI Handlers (bonus):" << config::colors::reset << "\n";

  const std::map<std::string, std::string>& cgi =
      location.getCgiHandlers();  // Necesitas tener este getter

  if (cgi.empty()) {
    os << "\t" << config::colors::yellow << "  CGI: " << config::colors::reset
       << config::colors::red << "none configured" << config::colors::reset
       << "\n";
  } else {
    std::map<std::string, std::string>::const_iterator it = cgi.begin();
    for (; it != cgi.end(); ++it) {
      os << "\t" << config::colors::yellow << "[" << it->first
         << "]: " << config::colors::reset << config::colors::green
         << it->second << config::colors::reset << "\n";
    }
  }

  return os;
}

#endif  // WEBSERV_LOCATIONCONFIG_HPP
