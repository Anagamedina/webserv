#include "SessionUtils.hpp"

#include <set>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cstdio>

namespace {

std::string generateSessionId() {
    static unsigned int counter = 0;
    std::ostringstream oss;
    oss << std::time(NULL) << "_" << (counter++) << "_" << (rand() % 1000000);
    return oss.str();
}

// Parse Cookie header: "session_id=value" or "name1=v1; session_id=value"
std::string extractSessionIdSimple(const std::string& cookieHeader) {
    const std::string key = "session_id=";
    size_t pos = cookieHeader.find(key);
    if (pos == std::string::npos)
        return "";

    size_t valueStart = pos + key.size();
    size_t valueEnd = cookieHeader.find(';', valueStart);
    if (valueEnd == std::string::npos)
        valueEnd = cookieHeader.size();

    std::string value = cookieHeader.substr(valueStart, valueEnd - valueStart);
    // Trim trailing space
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
        value.pop_back();
    return value;
}

} // namespace

void addSessionCookieIfNeeded(HttpResponse& response, const HttpRequest& request, int statusCode) {
    if (statusCode < 200 || statusCode >= 300)
        return;

    static std::set<std::string> validSessions;
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned>(time(NULL)));
        seeded = true;
    }

    std::string cookieHeader = request.getHeader("cookie");
    std::string sessionId = extractSessionIdSimple(cookieHeader);

    bool hasValidSession = false;
    if (!sessionId.empty() && validSessions.find(sessionId) != validSessions.end()) {
        hasValidSession = true;
    }

    if (!hasValidSession) {
        sessionId = generateSessionId();
        validSessions.insert(sessionId);
        response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/");
    }
}
