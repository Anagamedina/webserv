#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>

void printFlags(int flags, const std::string& label) {
    std::cout << "\n" << label << ":\n";
    std::cout << "  Raw value: 0x" << std::hex << flags << std::dec << " (" << flags << ")\n";
    std::cout << "  Flags breakdown:\n";
    
    // Access mode (bits 0-1)
    int access = flags & O_ACCMODE;
    if (access == O_RDONLY) std::cout << "    O_RDONLY (read-only)\n";
    if (access == O_WRONLY) std::cout << "    O_WRONLY (write-only)\n";
    if (access == O_RDWR)   std::cout << "    O_RDWR (read-write)\n";
    
    // File status flags
    if (flags & O_APPEND)    std::cout << "    O_APPEND (0x" << std::hex << O_APPEND << std::dec << ")\n";
    if (flags & O_NONBLOCK)  std::cout << "    O_NONBLOCK (0x" << std::hex << O_NONBLOCK << std::dec << ")\n";
    if (flags & O_SYNC)      std::cout << "    O_SYNC (0x" << std::hex << O_SYNC << std::dec << ")\n";
    if (flags & O_ASYNC)     std::cout << "    O_ASYNC (0x" << std::hex << O_ASYNC << std::dec << ")\n";
    
#ifdef O_LARGEFILE
    if (flags & O_LARGEFILE) std::cout << "    O_LARGEFILE (0x" << std::hex << O_LARGEFILE << std::dec << ")\n";
#endif
    
    if (flags == 0) std::cout << "    (no flags set)\n";
}

int main() {
    std::cout << "=== Testing F_GETFL with sockets ===\n";
    
    // 1. Create a regular socket
    std::cout << "\n--- Test 1: Regular socket() ---\n";
    int sock1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock1 < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }
    
    int flags1 = fcntl(sock1, F_GETFL, 0);
    printFlags(flags1, "Flags after socket()");
    
    // 2. Set O_NONBLOCK using F_SETFL WITHOUT F_GETFL (incorrect)
    std::cout << "\n--- Test 2: F_SETFL with O_NONBLOCK only (INCORRECT) ---\n";
    int sock2 = socket(AF_INET, SOCK_STREAM, 0);
    
    int flags_before = fcntl(sock2, F_GETFL, 0);
    printFlags(flags_before, "Before F_SETFL (O_NONBLOCK only)");
    
    // Wrong way: overwrite all flags
    if (fcntl(sock2, F_SETFL, O_NONBLOCK) == -1) {
        std::cerr << "F_SETFL failed\n";
    }
    
    int flags_after_wrong = fcntl(sock2, F_GETFL, 0);
    printFlags(flags_after_wrong, "After F_SETFL (O_NONBLOCK only)");
    
    std::cout << "\n  ⚠️  Notice: O_LARGEFILE was LOST!\n";
    
    // 3. Set O_NONBLOCK using F_GETFL + F_SETFL (correct)
    std::cout << "\n--- Test 3: F_GETFL + F_SETFL (CORRECT) ---\n";
    int sock3 = socket(AF_INET, SOCK_STREAM, 0);
    
    int flags_before_correct = fcntl(sock3, F_GETFL, 0);
    printFlags(flags_before_correct, "Before F_GETFL + F_SETFL");
    
    // Correct way: read-modify-write
    int current_flags = fcntl(sock3, F_GETFL, 0);
    if (fcntl(sock3, F_SETFL, current_flags | O_NONBLOCK) == -1) {
        std::cerr << "F_SETFL failed\n";
    }
    
    int flags_after_correct = fcntl(sock3, F_GETFL, 0);
    printFlags(flags_after_correct, "After F_GETFL + F_SETFL");
    
    std::cout << "\n  ✅ O_LARGEFILE was PRESERVED!\n";
    
    // 4. Test with SOCK_NONBLOCK (modern alternative)
#ifdef SOCK_NONBLOCK
    std::cout << "\n--- Test 4: socket() with SOCK_NONBLOCK ---\n";
    int sock4 = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock4 >= 0) {
        int flags4 = fcntl(sock4, F_GETFL, 0);
        printFlags(flags4, "Flags after socket(SOCK_STREAM | SOCK_NONBLOCK)");
        close(sock4);
    } else {
        std::cout << "  SOCK_NONBLOCK not supported on this system\n";
    }
#else
    std::cout << "\n--- Test 4: SOCK_NONBLOCK not available (old system) ---\n";
#endif
    
    // 5. Comparison table
    std::cout << "\n=== Comparison Summary ===\n";
    std::cout << std::setw(30) << "Method" 
              << std::setw(15) << "O_NONBLOCK" 
              << std::setw(15) << "O_LARGEFILE\n";
    std::cout << std::string(60, '-') << "\n";
    
    std::cout << std::setw(30) << "socket() default" 
              << std::setw(15) << ((flags1 & O_NONBLOCK) ? "YES" : "NO")
              << std::setw(15) << ((flags1 & O_LARGEFILE) ? "YES" : "NO") << "\n";
    
    std::cout << std::setw(30) << "F_SETFL O_NONBLOCK only" 
              << std::setw(15) << ((flags_after_wrong & O_NONBLOCK) ? "YES" : "NO")
              << std::setw(15) << ((flags_after_wrong & O_LARGEFILE) ? "YES" : "NO") 
              << "  ❌ LOST\n";
    
    std::cout << std::setw(30) << "F_GETFL + F_SETFL" 
              << std::setw(15) << ((flags_after_correct & O_NONBLOCK) ? "YES" : "NO")
              << std::setw(15) << ((flags_after_correct & O_LARGEFILE) ? "YES" : "NO") 
              << "  ✅ OK\n";
    
    // Cleanup
    close(sock1);
    close(sock2);
    close(sock3);
    
    std::cout << "\n=== Conclusion ===\n";
    std::cout << "F_GETFL is REQUIRED to preserve existing flags when using F_SETFL.\n";
    std::cout << "Without F_GETFL, O_LARGEFILE (and potentially other flags) are lost.\n";
    
    return 0;
}
