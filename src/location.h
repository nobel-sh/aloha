#ifndef LOCATION_H_
#define LOCATION_H_
#include <iostream>
#include <sstream>
#include <string>

class Location {
public:
  unsigned int line;
  unsigned int col;
  Location(unsigned int line, unsigned int col) : line(line), col(col) {}

  std::string to_string() const {
    std::stringstream ss;
    ss << line << ":" << col;
    return ss.str();
  }

  std::string line_as_str() { return std::to_string(line); }
  std::string col_as_str() { return std::to_string(col); }
};

#endif // LOCATION_H_
