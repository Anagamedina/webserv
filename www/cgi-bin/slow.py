#!/usr/bin/env python3
import time
import sys

# Output headers
print("Content-Type: text/plain")
print()

# Simulate slow processing
for i in range(5):
    print(f"Line {i+1}: Processing... (sleep for 1 second)")
    sys.stdout.flush()
    time.sleep(1)

print("Done!")
