#include "StaticPathHandler.hpp"

#include "ErrorUtils.hpp"

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

static bool readFileToBody(const std::string& path, std::vector<char>& out)
{
    //Binary : los guarda tal cual. Esto permite que el servidor envíe fotos, PDFs o vídeos sin romperlos
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
        return false;

    out.clear();
    //char mide exactamente 1 byte. Es la unidad perfecta para medir datos binarios.
    char c;
    while (file.get(c))
        out.push_back(c);
    return true;
}

static std::vector<char> generateAutoIndexBody(const std::string& dirPath,
                                               const std::string& requestPath)
{
    std::ostringstream html;
    std::string base = requestPath;
    if (base.empty())
        base = "/";
    if (base[base.size() - 1] != '/')
        base += "/";

    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "  <head>\n"
         << "    <meta charset=\"utf-8\">\n"
         << "    <title>Index of " << base << "</title>\n"
         << "  </head>\n"
         << "  <body>\n"
         << "    <h1>Index of " << base << "</h1>\n"
         << "    <ul>\n";

    DIR* dir = opendir(dirPath.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == "." || name == "..")
                continue;

            std::string fullPath = dirPath;
            if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/')
                fullPath += "/";
            fullPath += name;

            struct stat st;
            bool isDir = (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

            html << "      <li><a href=\"" << base << name;
            if (isDir)
                html << "/";
            html << "\">" << name;
            if (isDir)
                html << "/";
            html << "</a></li>\n";
        }
        closedir(dir);
    }

    html << "    </ul>\n"
         << "  </body>\n"
         << "</html>\n";

    std::string content = html.str();
    return std::vector<char>(content.begin(), content.end());
}

bool handleStaticPath(const HttpRequest& request,
                      const ServerConfig* server,
                      const LocationConfig* location,
                      const std::string& path,
                      std::vector<char>& body,
                      HttpResponse& response)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        //si es distinto a cero es que la ruta no existe -> 404 error 
        buildErrorResponse(response, request, 404, false, server);
        return true;
    }

    //Sin barra: El sistema busca un archivo con un nombre largo y extraño.
    //Con barra: El sistema entiende que lo que sigue es el contenido de esa carpeta.
    //ES UNA CARPETA? si es asi no se puede leer como un archivo debemos buscar
    //un index 
    if (S_ISDIR(st.st_mode))
    {
        std::vector<std::string> indexes;
        if (location)
            indexes = location->getIndexes();
        if (indexes.empty() && server && !server->getIndex().empty())
            indexes.push_back(server->getIndex());
        if (indexes.empty())
            indexes.push_back("index.html");

        bool foundIndex = false;
        std::string indexPath;
        for (size_t i = 0; i < indexes.size(); ++i)
        {
            indexPath = path;
            //distinción entre el contenedor y el contenido.
            //Si el usuario pidió localhost:8080/perfil/ (con barra), el path ya termina en /. No queremos añadir otra y que quede perfil//index.html.
            //Si el usuario pidió localhost:8080/perfil (sin barra), tenemos que añadirla nosotros para poder "entrar" en la carpeta.
            if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
                indexPath += "/";
            //Construye la ruta: Pega la carpeta con el nombre del archivo (ej: ./www/perfil/ + index.html).
            //La carpeta (path): ./www/perfil
            //El archivo (index): index.html
            //El sistema operativo buscaría un archivo llamado perfilindex.html dentro de ./www/.
            indexPath += indexes[i];

            struct stat idx;

            if (stat(indexPath.c_str(), &idx) == 0 && S_ISREG(idx.st_mode)) //es un archivo normal, no otra carpeta".
            {
                foundIndex = true;
                break;
            }
        }

        if (foundIndex)
        {
            if (!readFileToBody(indexPath, body))
            {
                buildErrorResponse(response, request, 403, false, server);
                return true;
            }
            return false;
        }

        //encontro algun index? NO -> revisamos si el autoindex esta encendido
        //si esta ON deberia mostrar la lista de archivos 
        //si esta OFF 403 forbidden
        //El autoindex en Nginx se utiliza para generar automáticamente un listado de archivos y carpetas en el navegador cuando se accede a un director        //io que no contiene un archivo de índice (como index.html o index.php). Habilita la visualización de archivos, 
        //facilitando la descarga o exploración de directorios. 
        if (location && location->getAutoIndex())
        {
            body = generateAutoIndexBody(path, request.getPath());
            response.setHeader("Content-Type", "text/html");
            return false;
        }

        buildErrorResponse(response, request, 403, false, server);
        return true;
    }

    //es algo "especial" de Linux algo raro ni archivo ni directorio
    if (!S_ISREG(st.st_mode))
    {
        buildErrorResponse(response, request, 403, false, server);
        return true;
    }
    


    //En una Carpeta (S_ISDIR):
    //El método casi siempre va a ser GET (queremos ver el índice o la lista de archivos)
    // Archivos regulares: diferenciar metodo
    if (request.getMethod() == HTTP_METHOD_POST)
    {
        //Los POST solo deberían ir a los CGIs
        buildErrorResponse(response, request, 405, false, server);
        return true;
    }
    if (request.getMethod() == HTTP_METHOD_DELETE)
    {
        //Linux para borrar un archivo
        if (unlink(path.c_str()) == 0)
        {
            body.clear();
            return false;
        }
        buildErrorResponse(response, request, 500, true, server);
        return true;
    }

    //METHOD GET!! 

    if (!readFileToBody(path, body))
    {
        buildErrorResponse(response, request, 403, false, server);
        return true;
    }

    return false;
}
