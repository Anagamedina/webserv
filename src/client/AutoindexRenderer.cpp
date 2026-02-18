#include "AutoindexRenderer.hpp"
#include <sstream>

static std::string escapeHtml(const std::string& s) {
  std::string out;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '&') out += "&amp;";
    else if (s[i] == '<') out += "&lt;";
    else if (s[i] == '>') out += "&gt;";
    else if (s[i] == '"') out += "&quot;";
    else out += s[i];
  }
  return out;
}

std::vector<char> renderAutoindexHtml(const std::string& base,
                                      const std::string& itemsHtml) {
  std::ostringstream html;
  std::string safeBase = base.empty() ? "/" : base;

  std::string title = "Index of " + safeBase;

  html << "<!DOCTYPE html>\n"
       << "<html lang=\"en\">\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <title>" << escapeHtml(title) << "</title>\n"
       << "</head>\n"
       << "<body>\n"
       << "  <h1>" << escapeHtml(title) << "</h1>\n"
       << "  <ul>\n"
       << itemsHtml
       << "  </ul>\n"
       << "</body>\n"
       << "</html>\n";

  std::string content = html.str();
  return std::vector<char>(content.begin(), content.end());
}
