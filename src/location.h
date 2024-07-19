#ifndef LOCATION_H_
#define LOCATION_H_

#include <cstdint>
#include <string>

struct Location {
  uint32_t line;
  uint32_t col;
  constexpr Location(uint32_t line, uint32_t col) : line(line), col(col) {}
  std::string to_string() const {
    return std::to_string(line) + ":" + std::to_string(col);
  }
};

#endif // LOCATION_H_
