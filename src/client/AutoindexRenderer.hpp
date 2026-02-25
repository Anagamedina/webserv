#ifndef AUTOINDEX_RENDERER_HPP
#define AUTOINDEX_RENDERER_HPP

#include <string>
#include <vector>

std::string escapeHtml(const std::string& s);

std::vector<char> renderAutoindexHtml(const std::string& base,
                                      const std::string& itemsHtml);

#endif  // AUTOINDEX_RENDERER_HPP
