#include <iostream>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>

#include "ConfigParser.hpp"
#include "ConfigException.hpp"

std::string findFile(const std::string& startDirectory, const std::string& fileName)
{
    DIR* dir = opendir(startDirectory.c_str());
    if (!dir)
    {
        return ("");
    }
    struct dirent* entry;
    while ( (entry = readdir(dir)))
    {
        std::string name = entry->d_name;
        std::string path = startDirectory + "/" + name;
        struct stat st;

        if (stat(path.c_str(), &st) == 0)
        {
            if (S_ISREG(st.st_mode) && name == fileName)
            {
                closedir(dir);
                return (path);
            }
            else if (S_ISDIR(st.st_mode))
            {
                std::string found = findFile(path, fileName);
                if (!found.empty())
                {
                    closedir(dir);
                    return (found);
                }
            }
        }
    }
    closedir(dir);
    return ("");
}

//	get directory parent of current directory 
std::string getParentDirectory(const std::string& path)
{
	const size_t positionOfBackslash = path.find_last_of("/\\");

    if (positionOfBackslash != std::string::npos)
    {
        return (path.substr(0, positionOfBackslash));
    }
	return ("");
}

//	function to get the root project using system call variable __FILE__
/*
std::string& getProjectRoot(const std::string& currentFilePath)
{
	std::string path = currentFilePath;

    path = getParentDirectory(path);

}
    */
    /*
    std::string path = __FILE__;
    std::string projectRoot = getProjectRoot(__FILE__);
    std::string projectRoot = __FILE__;
    std::cout << "Project root: [" << projectRoot << "]\n";
    */


int main(int argc, char *argv[])
{
	try
	{
		const std::string configPath = (argc == 2) ? argv[1] : "config/default.conf";

		ConfigParser parsingFile(configPath);

		parsingFile.parse();

		// std::vector<ServerConfig> vectorServers= parsingFile.getServers();
		// std::cout << "number of servers in file: " << vectorServers.size() << " server(s)" << std::endl;
	}
	catch (const ConfigException& e)
	{
		std::cerr << "Configuration error: " << e.what() << std::endl;
		return 1;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
