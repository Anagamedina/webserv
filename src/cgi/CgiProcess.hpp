/**
 * CgiProcess.hpp
 * 
 * Represents an active CGI process and its communication pipes
 * Tracks execution state, handles timeouts, buffers output
 */

#pragma once

#include <string>
#include <map>
#include <ctime>
#include <sys/types.h>

class CgiProcess {
public:
    enum State {
        RUNNING,      // Child process executing
        DATA_READY,   // Output available to read
        COMPLETED,    // Child finished, waiting for response send
        FAILED,       // Child crashed or timeout
        SENT          // Response fully sent to client
    };

    /**
     * Create a new CGI process tracker
     * 
     * @param script_path: Absolute path to CGI script
     * @param interpreter: Interpreter path (empty for executable scripts)
     * @param pipe_in_write: Write end of stdin pipe (parent writes request body)
     * @param pipe_out_read: Read end of stdout pipe (parent reads response)
     * @param pid: Child process ID
     * @param timeout: Timeout in seconds
     */
    CgiProcess(const std::string& script_path,
               const std::string& interpreter,
               int pipe_in_write,
               int pipe_out_read,
               pid_t pid,
               int timeout_secs);
    
    ~CgiProcess();

    // ========== State Management ==========
    State getState() const { return state_; }
    void setState(State new_state) { state_ = new_state; }
    
    // ========== Process Info ==========
    pid_t getPid() const { return pid_; }
    int getPipeOut() const { return pipe_out_read_; }
    int getPipeIn() const { return pipe_in_write_; }
    
    // ========== Data Management ==========
    /**
     * Append data read from CGI output
     * @return true if complete (headers received), false if still reading
     */
    bool appendResponseData(const char* data, size_t len);
    
    const std::string& getResponseHeaders() const { return response_headers_; }
    const std::string& getResponseBody() const { return response_body_; }
    const std::string& getCompleteResponse() const { return complete_response_; }
    
    int getStatusCode() const { return status_code_; }
    void setStatusCode(int code) { status_code_ = code; }
    
    // ========== Timeout Management ==========
    bool isTimedOut() const;
    time_t getStartTime() const { return start_time_; }
    int getTimeoutSeconds() const { return timeout_secs_; }
    
    // ========== Utility ==========
    std::string getScriptPath() const { return script_path_; }
    std::string getInterpreter() const { return interpreter_; }
    
private:
    // ========== Process Info ==========
    pid_t pid_;
    std::string script_path_;
    std::string interpreter_;
    
    // ========== Communication Pipes ==========
    int pipe_in_write_;   // Write request body to child stdin
    int pipe_out_read_;   // Read CGI output from child stdout
    
    // ========== Response Data ==========
    std::string complete_response_;  // Raw CGI output (headers + body)
    std::string response_headers_;   // Parsed headers section
    std::string response_body_;      // Parsed body section
    bool headers_complete_;          // True once we've found header/body separator
    int status_code_;
    
    // ========== State ==========
    State state_;
    time_t start_time_;
    int timeout_secs_;
    
    // ========== Helper Methods ==========
    /**
     * Try to parse response into headers and body
     * Looks for "\r\n\r\n" or "\n\n" separator
     * @return true if headers are complete, false if still waiting
     */
    bool tryParseHeaders();
};
