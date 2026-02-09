#include "../cgi/CgiExecutor.hpp"
#include "../cgi/CgiProcess.hpp"
#include "../http/HttpHeaderUtils.hpp"
#include "../network/ServerManager.hpp"
#include "Client.hpp"
#include "ErrorUtils.hpp"
#include "RequestProcessorUtils.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unistd.h>

void Client::setServerManager(ServerManager* serverManager) {
    _serverManager = serverManager;
}

static void parseCgiHeaders(const std::string& headers, HttpResponse& response) {
    std::istringstream iss(headers);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);
        if (line.empty())
            continue;

        std::string key;
        std::string value;
        if (!http_header_utils::splitHeaderLine(line, key, value))
            continue;

        std::string keyLower = key;
        std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
        if (keyLower == "status")
            continue;
        response.setHeader(key, value);
    }
}

bool Client::startCgiIfNeeded(const HttpRequest& request) {
    if (_configs == 0 || _serverManager == 0)
        return false;

    const ServerConfig* server = selectServerByPort(_listenPort, _configs);
    if (server == 0)
        return false;
    const LocationConfig* location = matchLocation(*server, request.getPath());
    if (location == 0)
        return false;

    std::string scriptPath = resolvePath(*server, location, request.getPath());
    if (!isCgiRequest(scriptPath))
        return false;

    std::string interpreterPath; // TODO: obtener desde config si aplica
    CgiExecutor exec;
    _cgiProcess = exec.executeAsync(request, scriptPath, interpreterPath);
    if (_cgiProcess == 0) {
        buildErrorResponse(_response, request, 500, true, server);
        return true;
    }

    _serverManager->registerCgiPipe(_cgiProcess->getPipeOut(), EPOLLIN | EPOLLRDHUP, this);
    _serverManager->registerCgiPipe(_cgiProcess->getPipeIn(), EPOLLOUT | EPOLLRDHUP, this);
    _state = STATE_READING_BODY;
    return true;
}

void Client::finalizeCgiResponse() {
    const HttpRequest& request = _parser.getRequest();
    bool shouldClose = request.shouldCloseConnection();

    _response.setStatusCode(_cgiProcess->getStatusCode());
    if (request.getVersion() == HTTP_VERSION_1_0)
        _response.setVersion("HTTP/1.0");
    else
        _response.setVersion("HTTP/1.1");
    _response.setHeader("Connection", shouldClose ? "close" : "keep-alive");

    parseCgiHeaders(_cgiProcess->getResponseHeaders(), _response);
    _response.setBody(_cgiProcess->getResponseBody());

    std::vector< char > serialized = _response.serialize();
    enqueueResponse(serialized, shouldClose);
}

void Client::handleCgiPipe(int pipe_fd, size_t events) {
    if (_cgiProcess == 0)
        return;

    if (pipe_fd == _cgiProcess->getPipeIn() && (events & EPOLLOUT)) {
        const std::string& body = _cgiProcess->getRequestBody();
        size_t offset = _cgiProcess->getBodyBytesWritten();
        if (offset < body.size()) {
            ssize_t written = write(pipe_fd, body.c_str() + offset, body.size() - offset);
            if (written > 0) {
                _cgiProcess->advanceBodyBytesWritten(static_cast< size_t >(written));
                _lastActivity = std::time(0);
            }
        }
        if (_cgiProcess->isRequestBodySent()) {
            _cgiProcess->closePipeIn();
            _serverManager->unregisterCgiPipe(pipe_fd);
        }
        return;
    }

    if (pipe_fd == _cgiProcess->getPipeOut() && (events & EPOLLIN)) {
        char buffer[4096];
        ssize_t bytes = read(pipe_fd, buffer, sizeof(buffer));
        if (bytes > 0) {
            _cgiProcess->appendResponseData(buffer, static_cast< size_t >(bytes));
            _lastActivity = std::time(0);
            return;
        }
        if (bytes == 0) {
            _cgiProcess->closePipeOut();
            _serverManager->unregisterCgiPipe(pipe_fd);
            finalizeCgiResponse();
            delete _cgiProcess;
            _cgiProcess = 0;
            return;
        }
    }
}
