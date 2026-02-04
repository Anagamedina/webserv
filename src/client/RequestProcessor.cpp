#include "RequestProcessor.hpp"
#include "../utils/StringUtils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>
#include <algorithm>

RequestProcessor::RequestProcessor() : cgi_process_(NULL) {}

RequestProcessor::~RequestProcessor() {}

HttpResponse RequestProcessor::process(const HttpRequest& request, const ServerBlock* config) {
    std::string uri = request.getUri();
    
    // 0. Security Check
    // Decode URI first to catch encoded traversal (e.g. %2e%2e)
    // Simple decoder
    std::string decoded_uri = "";
    for (size_t i = 0; i < uri.length(); ++i) {
        if (uri[i] == '%' && i + 2 < uri.length()) {
            std::string hex = uri.substr(i + 1, 2);
            char c = (char)strtol(hex.c_str(), NULL, 16);
            decoded_uri += c;
            i += 2;
        } else {
            decoded_uri += uri[i];
        }
    }
    
    if (decoded_uri.find("..") != std::string::npos) {
        return handleError(403, config);
    }
    
    // Update URI to use decoded version for processing?
    // Usually we resolve path using decoded version.
    uri = decoded_uri;

    // 1. Find Location
    const LocationConfig* location = findLocation(uri, config);
    if (!location) {
        // Should rely on default location or root if not found, but usually there's a "/" location.
        // If no location matches, use server root.
        // For now, let's assume we always have a location or default to 404/403.
        // Better: create a default temporary location if none found? 
        // Or strictly strictly follow Nginx: returns 404 if no match.
        return handleError(404, config);
    }

    // 2. Check Redirect
    if (!location->redirect_url.empty()) {
        return handleRedirect(location->redirect_url);
    }

    // 3. Check Method
    if (!methodAllowed(request.getMethod(), *location)) {
        return handleError(405, config);
    }

    // 4. Resolve Path
    std::string path = resolvePath(uri, *location, config);
    
    // 5. Check URL length? (Already parsed)
    // 6. Check Body Size? (Should be done during receive if possible, or here)
    // 6. Check Body Size
    size_t limit = config->client_max_body_size;
    if (location->client_max_body_size > 0) {
        limit = location->client_max_body_size;
    }
    
    if (request.getBody().size() > limit) {
        return handleError(413, config);
    }

    // 7. Check File Type (Directory or File)
    struct stat s;
    if (stat(path.c_str(), &s) == 0) {
        if (s.st_mode & S_IFDIR) {
            // Check if it's a POST to a directory (meaning upload to this dir? or CGI?)
            if (request.getMethod() == "POST") {
                 return handlePost(request, path, *location, config);
            }
            return handleDirectory(path, *location);
        } else if (s.st_mode & S_IFREG) {
            // Check for CGI match
            std::string ext = "";
            size_t dot = path.find_last_of(".");
            if (dot != std::string::npos) {
                ext = path.substr(dot);
            }
            
            if (!ext.empty() && location->cgi_handlers.count(ext)) {
                 std::string interpreter = location->cgi_handlers.at(ext);
                 // Start async CGI execution
                 CgiProcess* proc = cgi_executor_.executeAsync(request, path, interpreter);
                 if (proc == NULL) {
                     return handleError(500, config);
                 }
                 // Store for later (Client will pick it up)
                 cgi_process_ = proc;
                 // Return placeholder response - Client will handle from here
                 return HttpResponse(200);
            }

            if (request.getMethod() == "DELETE") {
                return handleDelete(path, config);
            }
             // POST to existing file? Overwrite?
            if (request.getMethod() == "POST") {
                 return handlePost(request, path, *location, config); // Or 409 Conflict?
            }
            return handleStaticFile(path);
        } else {
             return handleError(403, config); // Sockets etc
        }
    } else {
        // File doesn't exist.
        // If method is POST, we might be creating it?
        // But resolvePath resolves to a file path.
        // If URI is /uploads/new.txt, resolvePath gives .../uploads/new.txt
        // stat fails. 
        // We should allow POST if parent dir exists?
        
        if (request.getMethod() == "POST") {
             return handlePost(request, path, *location, config);
        }
        
        return handleError(404, config);
    }
}

const LocationConfig* RequestProcessor::findLocation(const std::string& uri, const ServerBlock* config) {
    const LocationConfig* best_match = NULL;
    size_t longuest_match = 0;

    for (size_t i = 0; i < config->locations.size(); ++i) {
        const std::string& path = config->locations[i].path;
        
        // Check if URI starts with path
        if (uri.compare(0, path.length(), path) == 0) {
            // Exact match or prefix match
            // Needs to be robust: "/img" matches "/img/logo.png" but not "/img_backup"
            // So needs to match full segment or be equal
            
            if (path.length() > longuest_match) {
                 // Verify segment boundary if not root
                 if (path == "/" || uri.length() == path.length() || uri[path.length()] == '/') {
                    longuest_match = path.length();
                    best_match = &config->locations[i];
                 }
            }
        }
    }
    return best_match;
}

std::string RequestProcessor::resolvePath(const std::string& uri, const LocationConfig& location, const ServerBlock* config) {
    // Standard Nginx 'root' directive behavior:
    // root /var/www
    // location /img { root /var/www; }
    // request GET /img/logo.png
    // result: /var/www/img/logo.png (full URI is appended)
    
    std::string root = location.root;
    std::string uri_suffix = uri;

    if (root.empty()) {
        // Fallback to server root
        if (config && !config->root.empty()) {
            root = config->root;
        } else {
            root = "./www"; // Safe fallback
        }
    }
    
    // Standard root: append full URI to root (NOT stripping location path)
    // This is more standard Nginx behavior
    std::string path = root + uri_suffix;
    
    // Clean double slashes  
    while (path.find("//") != std::string::npos) {
        path.replace(path.find("//"), 2, "/");
    }
    
    return path;
}

bool RequestProcessor::methodAllowed(const std::string& method, const LocationConfig& location) {
    if (location.accepted_methods.empty()) {
        return true; // Default allow all? Or default allow GET? usually GET is allowed.
    }
    return location.accepted_methods.count(method);
}

HttpResponse RequestProcessor::handleStaticFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        // Maybe permissions?
        // We already checked stat existing.
        // Or it's a folder? (Covered)
        return HttpResponse(403); // Forbidden reading
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string body = ss.str();
    
    HttpResponse response(200);
    response.setBody(body);
    response.setHeader("Content-Type", getMimeType(path));
    response.setHeader("Content-Length", StringUtils::toString(body.length()));
    
    return response;
}

HttpResponse RequestProcessor::handleDirectory(const std::string& path, const LocationConfig& location) {
    // 1. Check Index
    if (!location.index.empty()) {
        std::string index_path = path;
        if (index_path[index_path.length()-1] != '/') index_path += "/";
        index_path += location.index;
        
        struct stat s;
        if (stat(index_path.c_str(), &s) == 0 && (s.st_mode & S_IFREG)) {
            return handleStaticFile(index_path);
        }
    }
    
    // 2. Autoindex
    if (location.autoindex) {
        // Generate listing
        std::string body = "<html><body><h1>Directory Listing</h1><ul>";
        // Iterate directory using opendir/readdir (allowed functions)
        // ... (TODO: Implement listing)
        body += "<li>TODO: Directory Listing</li>";
        body += "</ul></body></html>";
        
        HttpResponse response(200);
        response.setBody(body);
        response.setHeader("Content-Type", "text/html");
        return response;
    }
    
    return HttpResponse(403);
}

HttpResponse RequestProcessor::handleRedirect(const std::string& location_url) {
    HttpResponse response(301); // Moved Permanently
    response.setHeader("Location", location_url);
    return response;
}

HttpResponse RequestProcessor::handleDelete(const std::string& path, const ServerBlock* config) {
    if (access(path.c_str(), F_OK) == -1) {
        return handleError(404, config);
    }
    
    // Check write permissions?
    // remove() deletes a name from the filesystem.
    if (remove(path.c_str()) == 0) {
        HttpResponse response(204); // No Content
        return response;
    } else {
        // Checking errno would be better
        if (errno == EACCES || errno == EPERM) {
             return handleError(403, config);
        }
        return handleError(500, config);
    }
}

HttpResponse RequestProcessor::handlePost(const HttpRequest& request, const std::string& path, const LocationConfig& location, const ServerBlock* config) {
    (void)path; // Unused if we use upload_path
    
    // 1. Check if upload is enabled
    if (!location.upload_enabled) {
        // If upload not enabled, maybe it's CGI? (Handled before/after?)
        // Or 405 Method Not Allowed? But we checked methodAllowed before.
        // If POST is allowed but upload is disabled, maybe it means we need CGI.
        // For now, if no CGI, return 403 or 404.
        return handleError(403, config);
    }
    
    // 2. Determine Filename
    // Use the URI's basename
    std::string uri = request.getUri();
    size_t last_slash = uri.find_last_of('/');
    std::string filename;
    if (last_slash != std::string::npos && last_slash < uri.length() - 1) {
        filename = uri.substr(last_slash + 1);
    } else {
        filename = "uploaded_file_" + StringUtils::toString(time(NULL));
    }

    // 3. Determine Output Path
    std::string upload_dir = location.upload_path;
    if (upload_dir.empty()) {
        upload_dir = "./www/uploads"; // Default?
    }
    
    // Ensure dir ends with slash
    if (upload_dir[upload_dir.length() - 1] != '/') {
         upload_dir += "/";
    }
    
    std::string final_path = upload_dir + filename;
    
    // 4. Write to file
    std::ofstream outfile(final_path.c_str(), std::ios::binary);
    if (!outfile.is_open()) {
        return handleError(500, config);
    }
    
    outfile << request.getBody();
    outfile.close();
    
    HttpResponse response(201); // Created
    response.setHeader("Location", uri); // Or full URL
    return response;
}

HttpResponse RequestProcessor::handleError(int code, const ServerBlock* config) {
    // Check custom error pages
    if (config && config->error_pages.count(code)) {
        std::string path = config->error_pages.at(code);
        // Try to serve that file
        // Be careful of recursion!
        std::ifstream file(path.c_str());
        if (file.is_open()) {
             std::ostringstream ss;
             ss << file.rdbuf();
             HttpResponse response(code);
             response.setBody(ss.str());
             response.setHeader("Content-Type", "text/html");
             return response;
        }
    }
    
    // Default error page
    HttpResponse response(code);
    std::string body = "<html><body><h1>Error " + StringUtils::toString(code) + "</h1></body></html>";
    response.setBody(body);
    response.setHeader("Content-Type", "text/html");
    return response;
}

std::string RequestProcessor::getMimeType(const std::string& path) {
    size_t dot = path.find_last_of(".");
    if (dot != std::string::npos) {
        std::string ext = path.substr(dot + 1);
        if (ext == "html") return "text/html";
        if (ext == "css") return "text/css";
        if (ext == "js") return "application/javascript";
        if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
        if (ext == "png") return "image/png";
        if (ext == "gif") return "image/gif";
        if (ext == "txt") return "text/plain";
    }
    return "application/octet-stream";
}

CgiProcess* RequestProcessor::getCgiProcess() const {
    return cgi_process_;
}

void RequestProcessor::clearCgiProcess() {
    cgi_process_ = NULL;
}
