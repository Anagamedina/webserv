#include "AutoindexRenderer.hpp"

#include <sstream>
#include <string>

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
  std::string safeBase = base;
  if (safeBase.empty()) safeBase = "/";

  std::string pageTitle = "Index of " + safeBase;
  std::string mainTitle = "Index of " + safeBase;
  bool isTacticalView = (safeBase.find("/images") != std::string::npos);
  if (isTacticalView) {
    pageTitle = "LaserWeb — Tactical Scanner";
    mainTitle = "TACTICAL SCANNER — Rayos lanzados";
  }

  html << "<!DOCTYPE html>\n"
       << "<html lang=\"es\">\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <title>" << escapeHtml(pageTitle) << "</title>\n"
       << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
       << "  <link rel=\"stylesheet\" href=\"/css/laserweb.css\">\n"
       << "  <style>\n"
       << "    .scanner-grid{display:grid;grid-template-columns:minmax(0,1.2fr) minmax(0,1fr);gap:1.5rem;align-items:start}\n"
       << "    .radar-wrap{display:flex;justify-content:center}\n"
       << "    .radar{position:relative;width:260px;height:260px;border-radius:50%;background:radial-gradient(circle,rgba(0,255,0,0.25) 0,transparent 60%),radial-gradient(circle,transparent 58%,rgba(0,255,0,0.4) 60%,transparent 62%);border:2px solid #39ff14;box-shadow:0 0 25px rgba(57,255,20,0.6);overflow:hidden}\n"
       << "    .radar::before{content:'';position:absolute;width:100%;height:100%;background:conic-gradient(from 0deg,rgba(57,255,20,0) 0deg,rgba(57,255,20,0.5) 40deg,rgba(57,255,20,0) 80deg);animation:sweep 4s linear infinite}\n"
       << "    .radar-grid-line{position:absolute;top:50%;left:0;right:0;height:1px;background:rgba(57,255,20,0.3)}\n"
       << "    .radar-grid-line.vert{width:1px;height:100%;left:50%;top:0}\n"
       << "    @keyframes sweep{from{transform:rotate(0deg)}to{transform:rotate(360deg)}}\n"
       << "    .scanner-table{margin-top:0.8rem}\n"
       << "    .status-ok{color:#39ff14}\n"
       << "    .autoindex-list{list-style:none;margin:0;padding:0}\n"
       << "    .autoindex-list li{padding:0.35rem 0;border-bottom:1px solid #1a1a2e}\n"
       << "    .autoindex-list a{color:#00f5ff}\n"
       << "    .autoindex-list a:hover{color:#39ff14;text-shadow:0 0 8px rgba(0,245,255,0.5)}\n"
       << "    .rays-gallery{display:grid;grid-template-columns:repeat(auto-fill,minmax(120px,1fr));gap:1rem;margin-top:0.8rem}\n"
       << "    .rays-gallery .ray-img{display:flex;flex-direction:column;align-items:center;padding:0.5rem;border:1px solid #39ff14;border-radius:4px;background:rgba(57,255,20,0.05);border-bottom:none}\n"
       << "    .rays-gallery .ray-img a{display:block}\n"
       << "    .rays-gallery .ray-img img{width:100px;height:100px;object-fit:cover;border:1px solid #00f5ff}\n"
       << "    .rays-gallery .ray-img img:hover{box-shadow:0 0 12px rgba(0,245,255,0.6)}\n"
       << "    .rays-gallery .ray-img span{font-size:0.7rem;color:#808090;margin-top:0.3rem;word-break:break-all;text-align:center}\n"
       << "    .rays-gallery li:not(.ray-img){padding:0.5rem;border:1px solid #1a1a2e;border-radius:4px}\n"
       << "  </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "  <div class=\"page\">\n"
       << "    <header>\n"
       << "      <h1 class=\"logo\">L<span>Λ</span>S<span>Ξ</span>RW<span>Ξ</span>B</h1>\n"
       << "    </header>\n"
       << "    <nav class=\"nav-hud\">\n"
       << "      <a href=\"/\">THE HUD</a>\n"
       << "      <a href=\"/upload.html\">LOAD LASER</a>\n"
       << "      <a href=\"/about.html\">THE SQUAD</a>\n"
       << "      <a href=\"/target-list.html\">TARGET LIST</a>\n"
       << "      <a href=\"/images/\">TACTICAL SCANNER</a>\n"
       << "    </nav>\n"
       << "    <main class=\"panel\">\n"
       << "      <h2>" << escapeHtml(mainTitle) << "</h2>\n"
       << "      <p class=\"back-row\"><a href=\"/\" class=\"btn btn-back\">&larr; Volver al HUD</a></p>\n";

  if (isTacticalView) {
    html << "      <div class=\"scanner-grid\">\n"
         << "        <div class=\"radar-wrap\">\n"
         << "          <div class=\"radar\">\n"
         << "            <div class=\"radar-grid-line\"></div>\n"
         << "            <div class=\"radar-grid-line vert\"></div>\n"
         << "          </div>\n"
         << "        </div>\n"
         << "        <div>\n"
         << "          <h3 style=\"font-size:0.9rem;text-transform:uppercase;letter-spacing:0.15em;color:#00f5ff;\">Rayos lanzados</h3>\n"
         << "          <ul class=\"autoindex-list rays-gallery\">\n"
         << itemsHtml
         << "          </ul>\n"
         << "        </div>\n"
         << "      </div>\n";
  } else {
    html << "      <ul class=\"autoindex-list\">\n"
         << itemsHtml
         << "      </ul>\n";
  }

  html << "    </main>\n"
       << "  </div>\n"
       << "</body>\n"
       << "</html>\n";

  std::string content = html.str();
  return std::vector<char>(content.begin(), content.end());
}
