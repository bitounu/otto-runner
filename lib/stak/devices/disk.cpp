#include <stak/devices/disk.hpp>

// Return used size of user data partition in bytes
STAK_EXPORT uint64_t stakDiskUsage() {
  return 124746531L;
}

// Return total size of user data partition in bytes
STAK_EXPORT uint64_t stakDiskSize() {
  return 4000000000L;
}
