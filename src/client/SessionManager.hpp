#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <ctime>
#include <map>
#include <string>

struct SessionData {
  std::string sessionId;
  int visitCount;
  std::time_t lastAccess;
};

class SessionManager {
 public:
  static SessionManager& getInstance();

  // Retrieves existing session or creates a new one
  SessionData& getOrCreateSession(const std::string& sessionId);

  // Creates a strictly new session with a unique ID
  SessionData& createSession();

  // Helper to generate a random session ID
  std::string generateSessionId() const;

  // Clean up old sessions
  void cleanupSessions(int timeoutSeconds = 3600);

 private:
  SessionManager();
  ~SessionManager();
  SessionManager(const SessionManager&);
  SessionManager& operator=(const SessionManager&);

  std::map<std::string, SessionData> _sessions;
};

#endif  // SESSION_MANAGER_HPP
