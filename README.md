*This project has been created as part of the 42 curriculum by anamedin, dasalaza, cpujades.*

# Webserv

## Description
Webserv is a custom-built HTTP server written in C++98 entirely from scratch, designed to emulate the core behavior of industry-standard servers like NGINX. 

The primary goal of this project is to understand the underlying mechanics of the HTTP protocol by implementing a fully functional, non-blocking multiplexed server capable of handling multiple concurrent clients. It parses custom configuration files, serves static content, handles file uploads, executes Common Gateway Interface (CGI) scripts (such as Python, PHP, or Bash), and responds with precise HTTP status codes and custom error pages.

This project reinforces modern network programming paradigms, including strict adherence to I/O multiplexing (`epoll`), resilient socket management, and robust request/response parsing according to the RFC semantical definitions.

### Tools & Environment
The development and testing of Webserv were supported by the following tools and environments:
- **Operating Systems:** Linux, macOS
- **IDEs/Editors:** CLion, Vim, Neovim
- **Code Quality:** SonarQube
- **AI Tools:** Gemini, Grok

## Instructions

### Prerequisites
- A UNIX-like operating system (Linux).
- `c++` compiler (or `clang++`/`g++`) supporting the C++98 standard.
- Have installed `make`.

### Compilation
To compile the project and generate the `webserver` executable, run the following command at the root of the repository:
```bash
make
```
Other available `make` rules:
- `make all`: Default rule, compiles the executable.
- `make clean`: Removes all compiled object files and dependencies (`build` directory).
- `make fclean`: Executes `clean` and removes the `webserver` executable.
- `make re`: Executes `fclean` followed by `all` to completely recompile the project.
- `make debug`: Compiles the project with debugging symbols enabled.
- `make leaks`: Compiles the project with memory leak instrumentation (`-fsanitize=leak`).
- `make optimized`: Compiles the project with maximum optimization flags (`-O3`).

### Execution
The server requires a configuration file to run. A default configuration is provided in the `config/` directory. 
Execute the server by passing the path to a configuration file as an argument:
```bash
./webserver config/default.conf
```
If no file is provided, it will fallback to `.config/default.conf` if configured as default in the source.

Once running, you can access the configured servers via your web browser or command-line tools like `curl`:
```bash
curl -v http://localhost:8080
```

## Resources

During the development of Webserv, the following resources were instrumental in understanding network programming and the HTTP protocol:

- **[RFC 9110: HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110.html):** The primary reference for HTTP/1.1 syntax, status codes, and methodology.
- **[RFC 3875: The Common Gateway Interface (CGI) Version 1.1](https://tools.ietf.org/html/rfc3875):** Essential for understanding how to pass environment variables and execute scripts dynamically.
- **[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/):** The classic, indispensable tutorial for understanding sockets, `bind`, `listen`, `accept`, and multiplexing functions.

### Additional Reading
- **[NGINX Core Configuration Source](https://github.com/nginx/nginx/blob/master/conf/nginx.conf):** Reference material used to understand the structure and parameters of a real-world server configuration file.
- **[How To Install Nginx on Ubuntu 20.04](https://www.digitalocean.com/community/tutorials/how-to-install-nginx-on-ubuntu-20-04):** A practical guide that helped conceptualize server environments and standard installation practices.
- **[Understanding Nginx Server and Location Block Selection Algorithms](https://www.digitalocean.com/community/tutorials/understanding-nginx-server-and-location-block-selection-algorithms):** Essential reading for implementing the logic that matches incoming requests to the correct server and location blocks.
- **[Understanding the Nginx Location Directive](https://www.digitalocean.com/community/tutorials/nginx-location-directive):** Specific insights into how the `location` directive behaves and how modifiers affect routing.

### Recommended Books
- **[The Linux Programming Interface](https://amzn.eu/d/03wBhaY5):** A comprehensive guide to Linux system programming, essential for understanding `epoll`, sockets, and process management.
- **[NGINX Cookbook](https://amzn.eu/d/09z4Czpt):** Advanced recipes for high-performance load balancing and web serving, useful for understanding edge-case configurations.
- **[UNIX Network Programming](https://amzn.eu/d/0j1AM1rb):** The definitive text by W. Richard Stevens on network communications and socket-level programming in C.
- **[HTTP: The Definitive Guide](https://amzn.eu/d/06xE70wG):** An in-depth exploration of the HTTP protocol, its history, and its mechanics, by David Gourley.

### AI Usage
Artificial Intelligence tools (such as conversational LLMs like Gemini and Grok) were utilized during this project as a supplemental aid, specifically for:
- **Documentation & Comments:** Helping to formalize and understand concepts of diverse parts of the project.
- **RFC Clarification:** Summarizing complex concepts from the RFC documentation into more digestible explanations.
