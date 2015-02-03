#ifndef STAK_H
#define STAK_H
#include <string>
#include <vector>

#define LOG_ERRORS 0
#define LOG_INFO 1
#include <stdint.h>
#include <stdio.h>
#define STAK_EXPORT extern "C"

namespace stak {
  using string = std::string;
  template<typename T> using vector = std::vector<T>;
}
#endif
