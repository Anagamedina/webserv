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
    
    // Access mode (critical - bits 0-1)
    int access = flags & O_ACCMODE;
    std::cout << "    Access mode: ";
    if (access == O_RDONLY) std::cout << "O_RDONLY (0)\n";
    else if (access == O_WRONLY) std::cout << "O_WRONLY (1)\n";
    else if (access == O_RDWR) std::cout << "O_RDWR (2) ✅\n";
    
    // Status flags
    if (flags & O_NONBLOCK)  std::cout << "    O_NONBLOCK (0x" << std::hex << O_NONBLOCK << std::dec << ") ✅\n";
    if (flags & O_APPEND)    std::cout << "    O_APPEND (0x" << std::hex << O_APPEND << std::dec << ") ✅\n";
    if (flags & O_ASYNC)     std::cout << "    O_ASYNC (0x" << std::hex << O_ASYNC << std::dec << ") ✅\n";
    
    if (flags == O_RDWR) std::cout << "    (only access mode, no status flags)\n";
}

int main() {
    std::cout << "=== System Information ===\n";
    std::cout << "Architecture: x86_64 (64-bit)\n";
    std::cout << "O_LARGEFILE value: " << O_LARGEFILE << " (0 = not needed in 64-bit)\n";
    std::cout << "O_NONBLOCK value: 0x" << std::hex << O_NONBLOCK << std::dec << "\n";
    std::cout << "O_RDWR value: " << O_RDWR << "\n";
    
    // Test 1: Default socket
    std::cout << "\n=== Test 1: Default socket flags ===\n";
    int sock1 = socket(AF_INET, SOCK_STREAM, 0);
    int flags1 = fcntl(sock1, F_GETFL, 0);
    printFlags(flags1, "socket() default");
    
    // Test 2: WRONG - Set O_NONBLOCK without F_GETFL
    std::cout << "\n=== Test 2: F_SETFL O_NONBLOCK only (WRONG) ===\n";
    int sock2 = socket(AF_INET, SOCK_STREAM, 0);
    
    int before_wrong = fcntl(sock2, F_GETFL, 0);
    printFlags(before_wrong, "Before");
    
    fcntl(sock2, F_SETFL, O_NONBLOCK);  // ❌ Overwrites everything
    
    int after_wrong = fcntl(sock2, F_GETFL, 0);
    printFlags(after_wrong, "After (lost O_RDWR!)");
    
    int access_before = before_wrong & O_ACCMODE;
    int access_after = after_wrong & O_ACCMODE;
    std::cout << "\n  Access mode changed: " << access_before << " → " << access_after;
    if (access_before != access_after) {
        std::cout << " ❌ LOST O_RDWR!\n";
    }
    
    // Test 3: CORRECT - F_GETFL + F_SETFL
    std::cout << "\n=== Test 3: F_GETFL + F_SETFL (CORRECT) ===\n";
    int sock3 = socket(AF_INET, SOCK_STREAM, 0);
    
    int before_correct = fcntl(sock3, F_GETFL, 0);
    printFlags(before_correct, "Before");
    
    int current = fcntl(sock3, F_GETFL, 0);
    fcntl(sock3, F_SETFL, current | O_NONBLOCK);  // ✅ Preserves existing
    
    int after_correct = fcntl(sock3, F_GETFL, 0);
    printFlags(after_correct, "After (O_RDWR preserved!)");
    
    int access_before2 = before_correct & O_ACCMODE;
    int access_after2 = after_correct & O_ACCMODE;
    std::cout << "\n  Access mode: " << access_before2 << " → " << access_after2;
    if (access_before2 == access_after2) {
        std::cout << " ✅ PRESERVED!\n";
    }
    
    // Test 4: What if there's O_ASYNC set?
    std::cout << "\n=== Test 4: Preserving O_ASYNC (hypothetical) ===\n";
    int sock4 = socket(AF_INET, SOCK_STREAM, 0);
    
    // Artificially set O_ASYNC first
    fcntl(sock4, F_SETFL, O_RDWR | O_ASYNC);
    
    int with_async = fcntl(sock4, F_GETFL, 0);
    printFlags(with_async, "After setting O_ASYNC");
    
    // Now add O_NONBLOCK the WRONG way
    std::cout << "\n  Adding O_NONBLOCK WITHOUT F_GETFL:\n";
    fcntl(sock4, F_SETFL, O_NONBLOCK);
    int lost_async = fcntl(sock4, F_GETFL, 0);
    printFlags(lost_async, "Result (lost O_ASYNC!)");
    
    // Reset and do it RIGHT
    int sock5 = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock5, F_SETFL, O_RDWR | O_ASYNC);
    
    std::cout << "\n  Adding O_NONBLOCK WITH F_GETFL:\n";
    int flags_with_async = fcntl(sock5, F_GETFL, 0);
    fcntl(sock5, F_SETFL, flags_with_async | O_NONBLOCK);
    int preserved_async = fcntl(sock5, F_GETFL, 0);
    printFlags(preserved_async, "Result (O_ASYNC preserved!)");
    
    // Comparison table
    std::cout << "\n=== Comparison Summary ===\n";
    std::cout << std::left << std::setw(35) << "Method" 
              << std::setw(15) << "O_RDWR" 
              << std::setw(15) << "O_NONBLOCK"
              << std::setw(15) << "O_ASYNC\n";
    std::cout << std::string(80, '-') << "\n";
    
    std::cout << std::setw(35) << "socket() default" 
              << std::setw(15) << ((flags1 & O_ACCMODE) == O_RDWR ? "YES" : "NO")
              << std::setw(15) << ((flags1 & O_NONBLOCK) ? "YES" : "NO")
              << std::setw(15) << ((flags1 & O_ASYNC) ? "YES" : "NO") << "\n";
    
    std::cout << std::setw(35) << "F_SETFL O_NONBLOCK only" 
              << std::setw(15) << ((after_wrong & O_ACCMODE) == O_RDWR ? "YES" : "NO")
              << std::setw(15) << ((after_wrong & O_NONBLOCK) ? "YES" : "NO")
              << std::setw(15) << "N/A"
              << "  ❌\n";
    
    std::cout << std::setw(35) << "F_GETFL + F_SETFL" 
              << std::setw(15) << ((after_correct & O_ACCMODE) == O_RDWR ? "YES" : "NO")
              << std::setw(15) << ((after_correct & O_NONBLOCK) ? "YES" : "NO")
              << std::setw(15) << "N/A"
              << "  ✅\n";
    
    std::cout << std::setw(35) << "F_SETFL O_NONBLOCK (had O_ASYNC)" 
              << std::setw(15) << ((lost_async & O_ACCMODE) == O_RDWR ? "YES" : "NO")
              << std::setw(15) << ((lost_async & O_NONBLOCK) ? "YES" : "NO")
              << std::setw(15) << ((lost_async & O_ASYNC) ? "YES" : "NO")
              << "  ❌\n";
    
    std::cout << std::setw(35) << "F_GETFL + F_SETFL (had O_ASYNC)" 
              << std::setw(15) << ((preserved_async & O_ACCMODE) == O_RDWR ? "YES" : "NO")
              << std::setw(15) << ((preserved_async & O_NONBLOCK) ? "YES" : "NO")
              << std::setw(15) << ((preserved_async & O_ASYNC) ? "YES" : "NO")
              << "  ✅\n";
    
    // Cleanup
    close(sock1);
    close(sock2);
    close(sock3);
    close(sock4);
    close(sock5);
    
    std::cout << "\n=== Conclusion ===\n";
    std::cout << "On 64-bit systems, O_LARGEFILE = 0 (not needed).\n";
    std::cout << "However, F_GETFL is STILL REQUIRED to preserve:\n";
    std::cout << "  1. Access mode (O_RDWR) - bits 0-1\n";
    std::cout << "  2. Other status flags (O_ASYNC, O_APPEND, etc.)\n";
    std::cout << "  3. Portability to systems where other flags exist\n";
    std::cout << "\nWithout F_GETFL, you LOSE the access mode!\n";
    
    return 0;
}
