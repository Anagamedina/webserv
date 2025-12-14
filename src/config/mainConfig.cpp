#include <iostream>
#include <fcntl.h>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

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
	size_t positionOfBackslash = path.find_last_of("/\\");

    if (positionOfBackslash != std::string::npos)
    {
        return (path.substr(0, positionOfBackslash));
    }
	return ("");
}

//	function to get the root project using system call variable __FILE__
std::string getProjectRoot(const std::string& currentFilePath)
{
	std::string path = currentFilePath;

    path = getParentDirectory(path);
    // path = getParentDirectory(path);
    // path = getParentDirectory(path);

    return (path);
}
    /*
    std::string path = __FILE__;
    std::string projectRoot = getProjectRoot(__FILE__);
    std::string projectRoot = __FILE__;
    std::cout << "Project root: [" << projectRoot << "]\n";
    */

int  main(int argc, char const *argv[])
{
	std::string configPath = "config/default.conf";

    if (argc == 1)
    {
        std::cout << "Use default config file store in [config/default.conf]\n";
    }
	else if (argc == 2)
	{
		configPath = argv[1];
	}
	else
	{
		std::cerr << "Use: ./webserv [default.conf]" << "\n";
        return (1);
	}

    return (0);
}

/*
int main(int argc, char const *argv[])
{
    if (argc == 1)
    {
        std::string path = __FILE__;
        std::cout << "Use default config file store in [config/default.conf]\n";
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));  // get actual dir
        std::string start_dir = cwd;
        std::string filename = "default.conf";

        std::string path = findFile(start_dir, filename);
        if (!path.empty()) {
            std::cout << "Ruta encontrada: " << path << std::endl;
        } else {
            std::cout << "Fichero no encontrado." << std::endl;
        }
    }
    else if (argc == 2)
    {
		std::string fileName = argv[1];
		std::cout << "filename: [" << fileName << "]" << std::endl;
        std::string extensionFile = ".conf";

        size_t extLen = extensionFile.size();
        if (fileName.size() >= extLen && fileName.compare(fileName.size() - extLen, extLen, extensionFile) == 0)
        {
		    std::cout << "valid extension file config." << std::endl;
        }
    }
    return 0;
}
*/
