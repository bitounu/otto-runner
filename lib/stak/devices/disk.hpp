#ifndef STAK_DEVICES_DISK_HPP
#define STAK_DEVICES_DISK_HPP
#include <stak/stak.hpp>
#include <stak/types.hpp>

// Return used size of user data partition in bytes
STAK_EXPORT uint64_t stakDiskUsage();

// Return total size of user data partition in bytes
STAK_EXPORT uint64_t stakDiskSize();

#endif//STAK_DEVICES_DISK_HPP
