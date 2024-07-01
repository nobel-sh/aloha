#include "reader.h"

namespace Aloha {

FileReader::FileReader(const std::string &t_filePath) : m_filePath(t_filePath) {
  if (!is_valid_alo_file(m_filePath)) {
    throw std::runtime_error("Provided file is not a valid .alo file");
  }
}

std::string FileReader::as_string() const {
  std::ifstream fileStream(m_filePath);
  if (!fileStream.is_open()) {
    throw std::runtime_error("Could not open file " + m_filePath);
  }
  std::stringstream stringStream;
  stringStream << fileStream.rdbuf();
  return stringStream.str();
}

std::vector<char> FileReader::as_bytes() const {
  std::ifstream fileStream(m_filePath, std::ios::binary);
  if (!fileStream.is_open()) {
    throw std::runtime_error("Could not open file " + m_filePath);
  }
  return std::vector<char>((std::istreambuf_iterator<char>(fileStream)),
                           std::istreambuf_iterator<char>());
}

bool FileReader::is_valid_alo_file(const std::string &filePath) const {
  auto len = filePath.length();
  return len >= 3 && filePath[len - 3] == 'a' && filePath[len - 2] == 'l' &&
         filePath[len - 1] == 'o';
}

StringReader::StringReader(const std::string &t_input) : m_input(t_input) {}

std::string StringReader::as_string() const { return m_input; }

std::vector<char> StringReader::as_bytes() const {
  return std::vector<char>(m_input.begin(), m_input.end());
}

} // namespace Aloha
