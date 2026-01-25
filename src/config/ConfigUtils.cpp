#include "ConfigUtils.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

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

			std::ofstream logFile(config::paths::log_file.c_str());
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
				utils::removeComments(line);
				line = utils::trimLine(line);
				if (line.empty())
					continue;
				logFile << lineNum << "|" << line << "\n";
			}
			ifs.close();
		}
	}
}
