#ifndef LOCATION_H_
#define LOCATION_H_

#include <cstdint>
#include <string>
#include <optional>

struct Location
{
  uint32_t line;
  uint32_t col;
  std::optional<std::string> file_path;

  Location() : line(1), col(1), file_path(std::nullopt) {}

  Location(uint32_t line, uint32_t col, std::string file)
      : line(line), col(col), file_path(std::move(file)) {}

  std::string to_string() const
  {
    if (file_path)
    {
      return *file_path + ":" + std::to_string(line) + ":" + std::to_string(col);
    }
    return std::to_string(line) + ":" + std::to_string(col);
  }
};

#endif // LOCATION_H_
