#ifndef WEBSERV_CONFIGUTILS_HPP
#define WEBSERV_CONFIGUTILS_HPP

#include "../common/namespaces.hpp"

namespace config
{
	namespace utils
	{
		std::string trimLine(const std::string& line);
		void removeComments(std::string& line);
		std::string removeSpacesAndTabs(std::string& line);
		std::string normalizeSpaces(const std::string& line);
	}

}

#endif //WEBSERV_CONFIGUTILS_HPP