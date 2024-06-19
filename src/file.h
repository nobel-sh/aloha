#ifndef FILE_H_
#define FILE_H_

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class AlohaReader {

public:
  AlohaReader(const std::string &filePath) {
    auto len = filePath.length();
    if (filePath[len - 3] != 'a' && filePath[len - 2] != 'l' &&
        filePath[len - 1] != 'o') {
      throw std::runtime_error("Provided file is not a valid .alo file");
    }
    this->filePath = filePath;
  }

  std::string as_string() const {
    std::ifstream fileStream(filePath);
    if (!fileStream.is_open()) {
      throw std::runtime_error("Could not open file " + filePath);
    }
    std::stringstream stringStream;
    stringStream << fileStream.rdbuf();
    return stringStream.str();
  }

  std::vector<char> as_bytes() const {
    std::ifstream fileStream(filePath, std::ios::binary);
    if (!fileStream.is_open()) {
      throw std::runtime_error("Could not open file " + filePath);
    }

    return std::vector<char>((std::istreambuf_iterator<char>(fileStream)),
                             std::istreambuf_iterator<char>());
  }

private:
  std::string filePath;
};

#endif // FILE_H_
