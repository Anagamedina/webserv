#include "ConfigUtils.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <unistd.h>

#include "ConfigException.hpp"

namespace config
{
	namespace utils
	{
		/**
		 * remove in line: space, tab, newline and carriage return
		 * @param line The string to trim
		 * @return New string without leading/trailing whitespace
		 *
		 *   "  hello  " -> "hello"
		 *   "\t\ntest\r\n" -> "test"
		 */
		std::string trimLine(const std::string& line)
		{
			const std::string whitespace = "\t\n\r";

			const size_t start = line.find_first_not_of(whitespace);
			if (start == std::string::npos)
			{
				return "";
			}
			const size_t end = line.find_last_not_of(whitespace);
			return line.substr(start, end - start + 1);
		}

		/**
		 * if line start wirh '#' remove line
		 * @param line
		 */
		void removeComments(std::string& line)
		{
			size_t commentPosition = line.find('#');
			if (commentPosition != std::string::npos)
			{
				line = line.substr(0, commentPosition);
			}
		}

		struct IsConsecutiveSpace
		{
			bool operator()(char a, char b) const
			{
				return a == ' ' && b == ' ';
			}
		};

		std::string removeSpacesAndTabs(std::string& line)
		{
			line.erase(
				std::unique(line.begin(), line.end(), IsConsecutiveSpace()),
				line.end());
			return line;
		}

		std::string normalizeSpaces(const std::string& line)
		{
			std::stringstream ss(line);
			std::string word;
			std::string result;

			while (ss >> word)
			{
				if (!result.empty())
					result += " ";
				result += word;
			}
			return result;
		}

		/**
		 * returns 0 on success, -1 on msg_errors
		 * F_OK: check for existence
		 * R_OK: check for read permission
		*/
		bool fileExists(const std::string& path)
		{
			return (access(path.c_str(), F_OK | R_OK) == 0);
		}

		std::vector<std::string> split(const std::string& str, char delimiter)
		{
			std::vector<std::string> tokens;
			std::string token;
			std::istringstream tokenStream(str);

			if (delimiter == ' ')
			{
				while (tokenStream >> token)
					tokens.push_back(token);
			}
			else
			{
				while (std::getline(tokenStream, token, delimiter))
				{
					tokens.push_back(token);
				}
			}
			return tokens;
		}

		/**
		 * use find() function to search position of ';'
		 * @return substr of str
		 */
		std::string removeSemicolon(const std::string& str)
		{
			size_t pos = str.find(';');
			if (pos != std::string::npos)
			{
				return str.substr(0, pos);
			}
			return str;
		}

		int stringToInt(const std::string& str)
		{
			char* end;
			long value = std::strtol(str.c_str(), &end, 10);

			if (*end != '\0' || end == str.c_str())
			{
				throw std::runtime_error("Invalid Characters.");
			}
			if (value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min())
			{
				throw std::runtime_error("Number out of range (overflow).");
			}

			return static_cast<int>(value);
		}

		void exportContentToLogFile(const std::string& fileContent, const std::string& pathToExport)
		{
			std::ofstream log(pathToExport.data());
			log << fileContent;
			log.close();
		}

	}

	namespace debug
	{
		/**
		* AUX FUNCTION TO DEBUG
		* export config file '.log'
		* remove empty lines and comment lines.
		*/
		void debugConfigLog(const std::string& config_file_path)
		{
			std::ifstream ifs(config_file_path.c_str());
			if (!ifs.is_open())
			{
				throw ConfigException(
					config::errors::cannot_open_file +
					config_file_path +
					" (in generatePrettyConfigLog)"
				);
			}

			std::ofstream logFile(config::paths::log_file_config.c_str());
			if (!logFile.is_open())
			{
				std::cerr << "Warning: Could not open/create pretty log file: ";
				return;
			}

			logFile << "=== Pretty print of configuration file ===\n";
			logFile << "File: " << config_file_path << "\n";
			logFile << "Generated: " << __DATE__ << " " << __TIME__ << "\n";
			logFile << "----------------------------------------\n\n";

			std::string line;
			size_t lineNum = 0;
			while (std::getline(ifs, line))
			{
				++lineNum;
				config::utils::removeComments(line);
				line = config::utils::trimLine(line);
				if (line.empty())
					continue;
				logFile << lineNum << "|" << line << "\n";
			}
			ifs.close();
		}
	}
}



























