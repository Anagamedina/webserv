#include "SessionManager.hpp"

#include <cstdlib>
#include <sstream>

SessionManager& SessionManager::getInstance() {
  static SessionManager instance;
  return instance;
}

SessionManager::SessionManager() { std::srand(std::time(NULL)); }

SessionManager::~SessionManager() {}

SessionData& SessionManager::getOrCreateSession(const std::string& sessionId) {
  cleanupSessions();  // Auto-cleanup on access

  std::map<std::string, SessionData>::iterator it = _sessions.find(sessionId);
  if (it != _sessions.end()) {
    it->second.lastAccess = std::time(NULL);
    it->second.visitCount++;
    return it->second;
  }

  return createSession();
}

SessionData& SessionManager::createSession() {
  std::string newId = generateSessionId();

  SessionData data;
  data.sessionId = newId;
  data.visitCount = 1;
  data.lastAccess = std::time(NULL);

  _sessions[newId] = data;
  return _sessions[newId];
}

std::string SessionManager::generateSessionId() const {
  const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::string sessionId;
  for (int i = 0; i < 32; ++i) {
    sessionId += alphanum[std::rand() % (sizeof(alphanum) - 1)];
  }

  std::ostringstream timeComponent;
  timeComponent << std::time(NULL);

  return sessionId + "_" + timeComponent.str();
}

void SessionManager::cleanupSessions(int timeoutSeconds) {
  std::time_t now = std::time(NULL);
  std::map<std::string, SessionData>::iterator it = _sessions.begin();

  while (it != _sessions.end()) {
    if (now - it->second.lastAccess > timeoutSeconds) {
      std::map<std::string, SessionData>::iterator toErase = it;
      ++it;
      _sessions.erase(toErase);
    } else {
      ++it;
    }
  }
}
