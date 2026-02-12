#include "TcpListener.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "common/StringUtils.hpp"

TcpListener::TcpListener(const std::string& host, int port)
    : socket_fd_(-1), port_(port), host_(host) {
  createSocket();
  setSocketOptions();
  bindSocket();
}

TcpListener::~TcpListener() {
  if (socket_fd_ != -1) {
    close(socket_fd_);
  }
}

/// Creates a TCP socket for accepting client connections.
///
/// Uses AF_INET (IPv4) with SOCK_STREAM (TCP) to establish a reliable,
/// connection-oriented communication channel. When SOCK_STREAM is
/// specified with AF_INET, the kernel automatically selects IPPROTO_TCP.
///
/// Socket characteristics:
/// - Ordered delivery: bytes arrive in send order
/// - No duplication: duplicate TCP segments are discarded by the kernel
/// - Flow control: sliding window prevents receiver saturation
/// - Congestion control: TCP Cubic dynamically adjusts the send window
///
/// Alternatives not used:
/// - AF_INET6: IPv6 not required by the 42 subject
/// - AF_UNIX: local IPC only; not applicable for a web server
/// - SOCK_DGRAM: UDP is unreliable and incompatible with HTTP semantics
///
/// Returns a file descriptor (typically ≥ 3) representing this socket
/// in the kernel file descriptor table. The initial state is blocking;
/// O_NONBLOCK must be enabled separately via fcntl().
///
/// @throws std::runtime_error if socket creation fails
///         (EMFILE, ENFILE, ENOMEM, etc.)
void TcpListener::createSocket() {
  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ == -1) {
    throw std::runtime_error("Failed to create socket");
  }
  std::cout << "Socket created (fd: " << socket_fd_ << ")" << std::endl;
}

/// Configures socket options for optimal server operation.
///
/// Sets two critical flags:
///
/// 1. SO_REUSEADDR (socket-level option):
///    Allows immediate rebinding to the same port after server restart.
///
///    Problem it solves: TIME_WAIT state
///    - When server closes, TCP enters TIME_WAIT for 2×MSL (~60s on Linux)
///    - Without SO_REUSEADDR: bind() fails with EADDRINUSE
///    - With SO_REUSEADDR: kernel marks TIME_WAIT entry as reusable
///
///    Safety considerations:
///    - Safe for servers: same port, different client ephemeral ports
///    - Collision probability of (IP_local, port_local, IP_remote, port_remote)
///    ≈ 0
///
///    Level: SOL_SOCKET (generic, not TCP-specific)
///
/// 2. O_NONBLOCK (file status flag):
///    Enables non-blocking I/O on the socket.
///
///    Behavior change:
///    - Blocking (default): read/write/accept suspend execution until ready
///    - Non-blocking: return immediately with EAGAIN/EWOULDBLOCK
///
///    Why this is critical:
///    - Single-threaded event loop: one slow client cannot block others
///    - Epoll integration: readiness checked before actual I/O
///    - No process suspension: preserves responsiveness under load
///
///    Implementation: fcntl() read–modify–write
///    - F_GETFL: read current flags (preserves O_LARGEFILE, etc.)
///    - OR with O_NONBLOCK: add flag without clearing others
///    - F_SETFL: write modified flags back
///
///    Alternative not used:
///    - socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)  // Linux ≥ 2.6.27
///    - Reason: portability; read–modify–write is safer
///
/// @throws std::runtime_error if setsockopt() fails (EBADF, ENOPROTOOPT, …)
/// @throws std::runtime_error if fcntl(F_GETFL) fails
/// @throws std::runtime_error if fcntl(F_SETFL) fails
void TcpListener::setSocketOptions() {
  int opt = 1;
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    close(socket_fd_);
    throw std::runtime_error("Failed to set socket options");
  }

  // INFO: El subject prohibe usar otros flags que no sean F_SETFL
  // O_NONBLOCK, FD_CLOEXEC, sin embargo la buena practica para c++ es recuperar
  // los flags antes de añadir uno nuevo por que si solapa alguno de los flags
  // se sobreescribira. aplicando F_GETFL -> | <NEW_FLAG> se evita el problema
  int flags = fcntl(socket_fd_, F_GETFL, 0);
  if (flags == -1) {
    close(socket_fd_);
    throw std::runtime_error("Failed to get socket flags");
  }
  if (fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
    close(socket_fd_);
    throw std::runtime_error("Failed to set socket to non-blocking");
  }
  // if (fcntl(socket_fd_, F_SETFL, O_NONBLOCK) == -1) {
  //	close(socket_fd_);
  //	throw std::runtime_error("Failed to set socket to non-blocking");
  // }
}

/// Binds the socket to a specific port on all network interfaces.
///
/// Associates the socket with a local address (IP + port) so the kernel
/// knows where to route incoming connections. Uses sockaddr_in structure
/// for IPv4 addressing.
///
/// Address configuration:
///
/// 1. sin_family = AF_INET:
///    Specifies IPv4 address family (matches socket() domain)
///
/// 2. sin_addr.s_addr = INADDR_ANY (0.0.0.0):
///    Listens on ALL network interfaces simultaneously:
///    - lo (127.0.0.1): localhost connections
///    - eth0 (192.168.1.X): LAN connections
///    - wlan0 (10.0.0.X): WiFi connections
///
///    Alternative not used:
///    - Specific IP (e.g., inet_addr("127.0.0.1")): restricts to one interface
///    - Reason: server must be externally accessible
///
/// 3. sin_port = htons(port_):
///    Port number in network byte order (big-endian).
///
///    htons() necessity:
///    - Little-endian (x86): 8080 = 0x1F90 → memory [0x90, 0x1F]
///    - Big-endian (network): 8080 = 0x1F90 → memory [0x1F, 0x90]
///    - TCP/IP standard: network byte order = big-endian (RFC 1700)
///    - htons(): converts host order → network order (no-op on big-endian)
///
///    Without conversion: server would listen on port 36895 (0x901F) instead of
///    8080
///
/// Kernel operations on bind():
/// 1. Verifies port is not in use (unless SO_REUSEADDR)
/// 2. Registers socket in global port hash table
/// 3. Socket gains identity: (0.0.0.0:port_)
/// 4. Subsequent packets to this port route to this socket
///
/// Error conditions:
/// - EADDRINUSE: port already bound (check SO_REUSEADDR, kill other process)
/// - EACCES: port < 1024 requires root (use port >= 1024)
/// - EINVAL: socket already bound or invalid address
///
/// @throws std::runtime_error if bind() fails
void TcpListener::bindSocket() {
  struct addrinfo hints;
  struct addrinfo*
      result;  // will point to a linked list of addrinfo structures
  struct addrinfo* rp;
  int s;

  std::memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;  // Allow IPv4 only (compatible with socket())
  hints.ai_socktype = SOCK_STREAM;  // TCP socket
  hints.ai_flags = AI_PASSIVE;  // For wildcard IP address if host is null/empty

  const char* host_ptr = host_.empty() ? NULL : host_.c_str();
  std::string port_str = string_utils::toString(port_);

  s = getaddrinfo(host_ptr, port_str.c_str(), &hints, &result);
  if (s != 0) {
    close(socket_fd_);
    throw std::runtime_error("getaddrinfo failed: " +
                             std::string(gai_strerror(s)));
  }

  // getaddrinfo() returns a list of address structures.
  // Try each address until we successfully bind.
  // Since we forced AF_INET, we typically get one IPv4 result.

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if (bind(socket_fd_, rp->ai_addr, rp->ai_addrlen) == 0) {
      break;  // Success :)
    }
  }

  if (rp == NULL) {  // No address succeeded
    freeaddrinfo(result);
    close(socket_fd_);
    throw std::runtime_error("Failed to bind socket to " +
                             (host_.empty() ? "*" : host_) + ":" + port_str);
  }

  freeaddrinfo(result);

  std::cout << "Socket bound to " << (host_.empty() ? "0.0.0.0" : host_) << ":"
            << port_ << std::endl;
}

/// Activates listening mode on the socket, enabling it to accept connections.
///
/// Marks the socket as passive (listening for incoming connections) rather
/// than active (initiating connections). Transitions the socket state from
/// UNCONNECTED to LISTEN.
///
/// Backlog parameter: SOMAXCONN
///
/// Definition:
/// Maximum size of the accept queue (completed connections).
/// - Typical Linux value: 128 (see /proc/sys/net/core/somaxconn)
/// - Can be increased system-wide via sysctl
///
/// Purpose:
/// Buffers completed connections that have not yet been accept()'ed,
/// protecting the server from bursts of simultaneous connection attempts.
///
/// Linux maintains two internal queues:
///
/// 1. SYN queue (incomplete connections):
///    - Connections in TCP handshake (SYN_RECV state)
///    - Size: tcp_max_syn_backlog (default: 256)
///    - Client sent SYN, server sent SYN-ACK, awaiting final ACK
///
/// 2. Accept queue (completed connections):
///    - Connections in ESTABLISHED state
///    - Size: min(backlog, somaxconn)
///    - Handshake complete; waiting for accept()
///    - Controlled by SOMAXCONN
///
/// Queue overflow behavior:
/// - Accept queue full and a new connection completes the handshake:
///   → Connection is dropped silently (no RST sent)
///   → Client retransmits SYN (interpreting loss)
///   → Provides basic protection against SYN flood attacks
///
/// Why SOMAXCONN instead of a fixed value:
///
/// - Small value (e.g., 10):
///   - Issue: burst of 50 connections → 40 dropped
///   - Suitable only for low-traffic internal services
///
/// - Large value (e.g., 100000):
///   - Issue: unnecessary memory consumption; facilitates DoS
///   - Suitable use case: none (kernel limits exist for safety)
///
/// - SOMAXCONN (128):
///   - Balanced default for typical workloads
///   - Tunable by system administrators via sysctl
///   - Kernel-enforced upper bound limits abuse
///
/// After listen(), the socket can:
/// - Receive SYN packets from clients
/// - Complete TCP three-way handshakes automatically
/// - Queue established connections for accept()
///
/// State transition:
/// - Before: bind()  → socket bound to 0.0.0.0:8080
/// - After:  listen() → socket in TCP_LISTEN state
///
/// Kernel actions:
/// 1. Transitions sk->sk_state to TCP_LISTEN
/// 2. Allocates the SYN and accept queues
/// 3. Registers the socket in the listening hash table
/// 4. Begins processing SYN packets for this port
///
/// Error conditions:
/// - EADDRINUSE: another socket is already listening on this port
/// - EOPNOTSUPP: socket type does not support listen() (e.g., SOCK_DGRAM)
///
/// @throws std::runtime_error if listen() fails
void TcpListener::listen() {
  if (::listen(socket_fd_, SOMAXCONN) < 0) {
    throw std::runtime_error("Failed to listen on socket");
  }
  std::cout << "Listening for connections..." << std::endl;
}

/// Accepts a pending client connection from the accept queue.
///
/// Extracts the first completed connection from the kernel's accept queue
/// (populated by listen()) and creates a new socket for bidirectional
/// communication with the client. The original listening socket remains
/// active to accept additional connections.
///
/// Accept mechanics:
///
/// 1. Kernel state before accept():
///    - Listening socket: fd 3, state TCP_LISTEN, queue: [conn1, conn2, ...]
///    - Each queued connection has completed the TCP three-way handshake
///    - Client connections waiting to be extracted by the application
///
/// 2. accept() operation:
///    - Dequeues the oldest connection from the accept queue (FIFO)
///    - Allocates a NEW socket (distinct file descriptor)
///    - Copies peer address into client_addr (IP + ephemeral port)
///    - Returns new socket fd in ESTABLISHED state
///
/// 3. After accept():
///    - Listening socket: fd 3, remains in TCP_LISTEN (unchanged)
///    - New socket: fd N, state TCP_ESTABLISHED, connected to specific client
///    - Two independent sockets: one listens, one communicates
///
/// Non-blocking behavior:
///
/// - Accept queue empty:
///   → accept() returns -1 immediately
///   → errno set to EAGAIN or EWOULDBLOCK
///   → Caller must retry after epoll_wait() indicates readiness
///
/// - Accept queue has connections:
///   → accept() returns instantly with client_fd
///   → No blocking, even if new connections arrive concurrently
///
/// Why O_NONBLOCK on the new socket is CRITICAL:
///
/// - accept() does NOT inherit flags from the listening socket
/// - New client_fd starts in blocking mode (kernel default)
/// - Without fcntl(O_NONBLOCK): read/write on client_fd would block
/// - Consequence: one slow client stalls the entire server
///
/// Implementation: fcntl() read–modify–write pattern
/// - F_GETFL: read current flags (preserves O_LARGEFILE, etc.)
/// - OR with O_NONBLOCK: atomically add flag
/// - F_SETFL: write modified flags back
///
/// Alternative (Linux ≥ 2.6.28):
/// - accept4(fd, addr, len, SOCK_NONBLOCK | SOCK_CLOEXEC)
/// - Reason not used: portability; POSIX compliance
///
/// sockaddr_in extraction:
///
/// 1. sin_addr.s_addr:
///    Client IP address in network byte order (big-endian).
///    - Example: 192.168.1.100 → 0xC0A80164 in memory
///    - inet_ntop() converts to human-readable "192.168.1.100"
///
/// 2. sin_port:
///    Client ephemeral port in network byte order.
///    - Range: 32768–60999 (Linux default
///    /proc/sys/net/ipv4/ip_local_port_range)
///    - ntohs() converts to host order for logging
///
/// 3. addr_len (value-result parameter):
///    - Input: size of client_addr buffer
///    - Output: actual size written (unchanged for IPv4)
///    - Needed for IPv6 (larger sockaddr_in6) or protocol flexibility
///
/// Security considerations:
///
/// - bzero() before use: ensures uninitialized memory doesn't leak
/// - inet_ntop() with INET_ADDRSTRLEN: buffer overflow protection
/// - Failed fcntl() doesn't abort: graceful degradation
///
/// Why client IP/port logging is useful:
/// - Debugging: correlate requests with specific clients
/// - Security: identify attack sources (rate limiting, ban lists)
/// - Analytics: geographic distribution, connection patterns
///
/// Error conditions and handling:
///
/// - accept() returns -1:
///   → EAGAIN/EWOULDBLOCK: no connections available (non-blocking)
///   → EMFILE/ENFILE: process/system file descriptor limit reached
///   → ENOMEM: kernel out of memory
///   → EINTR: interrupted by signal (handled by retry)
///   → Function returns -1; caller must check and handle
///
/// - fcntl() failure:
///   → Client socket remains blocking (degraded, but functional)
///   → Server can still communicate; performance impact only
///   → Silent failure (no exception); logged implicitly
///
/// Resource management:
///
/// - Caller MUST close(client_fd) when done
/// - Failure to close causes file descriptor leak
/// - Leak consequence: eventually hits EMFILE → server stops accepting
///
/// State after successful accept():
/// - Listening socket: UNCHANGED, can accept more connections
/// - New socket: fd = client_fd, state = ESTABLISHED, peer = client_addr
/// - TCP connection: fully established, ready for send/recv
///
/// @return New socket file descriptor (≥ 0) on success
///         -1 on failure (check errno: EAGAIN, EMFILE, ENOMEM, etc.)
int TcpListener::acceptConnection() {
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  std::memset(&client_addr, 0, size_t(addr_len));
  int client_fd = accept(socket_fd_, (struct sockaddr*)&client_addr, &addr_len);

  if (client_fd < 0) return -1;

  int flags = fcntl(client_fd, F_GETFL, 0);
  if (flags != -1) {
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
  }

  // Manual IP formatting to replace inet_ntop (forbidden)
  unsigned char* ip_bytes = (unsigned char*)&client_addr.sin_addr;
  std::ostringstream oss;
  oss << (int)ip_bytes[0] << "." << (int)ip_bytes[1] << "." << (int)ip_bytes[2]
      << "." << (int)ip_bytes[3];
  std::string client_ip = oss.str();
  std::cout << "New connection from " << client_ip << ":"
            << ntohs(client_addr.sin_port) << " (fd: " << client_fd << ")"
            << std::endl;

  return client_fd;
}

int TcpListener::getFd() const { return socket_fd_; }

int TcpListener::getPort() const { return port_; }
