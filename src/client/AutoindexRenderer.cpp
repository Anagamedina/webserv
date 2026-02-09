#include "AutoindexRenderer.hpp"

#include <sstream>

std::vector< char > renderAutoindexHtml(const std::string& base, const std::string& itemsHtml) {
    std::ostringstream html;
    std::string safeBase = base;
    if (safeBase.empty())
        safeBase = "/";

    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "  <head>\n"
         << "    <meta charset=\"utf-8\">\n"
         << "    <title>Index of " << safeBase << "</title>\n"
         << "  </head>\n"
         << "  <body>\n"
         << "    <h1>Index of " << safeBase << "</h1>\n"
         << "    <ul>\n"
         << itemsHtml << "    </ul>\n"
         << "  </body>\n"
         << "</html>\n";

    std::string content = html.str();
    return std::vector< char >(content.begin(), content.end());
}
