#ifndef WEBSERV_CONFIGEXCEPTION_HPP
#define WEBSERV_CONFIGEXCEPTION_HPP

#include <exception>
#include <string>

/**
 * @brief Custom exception for configuration file parsing errors
 *
 * Thrown when the configuration file contains syntax errors,
 * invalid directives, missing required values, duplicate servers,
 * unbalanced brackets, or other validation failures.
 */
class ConfigException : public std::exception
{
public:
	/**
	 * @brief Construct exception with error message
	 * @param msg Descriptive error message
	 */
	explicit ConfigException(const std::string& msg);

	/**
	 * @brief Virtual destructor
	 */
	virtual ~ConfigException() throw();

	/**
	 * @brief Returns the error message
	 * @return C-string containing the exception description
	 */
	virtual const char* what() const throw();
private:
	std::string message_;

};

#endif // WEBSERV_CONFIGEXCEPTION_HPP