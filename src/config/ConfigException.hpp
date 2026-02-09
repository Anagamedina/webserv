#ifndef WEBSERV_CONFIGEXCEPTION_HPP
#define WEBSERV_CONFIGEXCEPTION_HPP

#include <exception>
#include <string>

class ConfigException : public std::exception {
  private:
    std::string message_;

  public:
    explicit ConfigException(const std::string& msg);
    virtual ~ConfigException() throw();
    virtual const char* what() const throw();
};

#endif // WEBSERV_CONFIGEXCEPTION_HPP
