#include "SessionUtils.hpp"

#include <set>
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>

// creates a unique id for the cookie (timestamp + counter + random)
std::string createSessionId()
{
    static int counter = 0;
    std::ostringstream ss;
    ss << time(NULL) << "_" << counter << "_" << (rand() % 100000);
    counter++;
    return ss.str();
}

// finds the value of our "id" cookie in the Cookie header
// example: Cookie: id=12345_0_999; other=val  -> returns "12345_0_999"
std::string extractIdFromCookie(const std::string& cookieHeader)
{
    std::string key = "id=";
    std::string::size_type pos = cookieHeader.find(key);
    if (pos == std::string::npos)
        return "";

    std::string::size_type startPos = pos + key.length();
    std::string::size_type endPos = cookieHeader.find(';', startPos);
    if (endPos == std::string::npos)
        endPos = cookieHeader.length();

    std::string result = cookieHeader.substr(startPos, endPos - startPos);

    // remove trailing spaces
    while (result.size() > 0 && (result[result.size() - 1] == ' ' || result[result.size() - 1] == '\t'))
    {
        result.erase(result.size() - 1);
    }
    return result;
}

void addSessionCookieIfNeeded(HttpResponse& response, const HttpRequest& request, int statusCode)
{
    // only add cookie for 200-299 responses
    if (statusCode < 200 || statusCode > 299)
        return;

    // list of ids we created that are valid
    static std::set<std::string> validSessions;

    // init rand once
    static bool alreadyInitialized = false;
    if (alreadyInitialized == false)
    {
        srand((unsigned int)time(NULL));
        alreadyInitialized = true;
    }

    // see what cookie the client sends
    std::string headerCookie = request.getHeader("cookie");
    std::string receivedId = extractIdFromCookie(headerCookie);

    // check if the id they send exists in our list
    bool isValid = false;
    if (receivedId != "" && validSessions.find(receivedId) != validSessions.end())
    {
        isValid = true;
    }

    // if no cookie or invalid, give them a new one
    if (isValid == false)
    {
        std::string newId = createSessionId();
        validSessions.insert(newId);
        std::string cookieValue = "id=" + newId + "; Path=/";
        response.setHeader("Set-Cookie", cookieValue);
    }
}
