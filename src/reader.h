#ifndef FILE_H_
#define FILE_H_

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Aloha {

class Reader {
public:
  virtual std::string as_string() const = 0;
  virtual std::vector<char> as_bytes() const = 0;
  virtual ~Reader() = default;
};

class FileReader : public Reader {
public:
  explicit FileReader(const std::string &t_filePath);
  std::string as_string() const override;
  std::vector<char> as_bytes() const override;

private:
  std::string m_filePath;
  bool is_valid_alo_file(const std::string &filePath) const;
};

class StringReader : public Reader {
public:
  explicit StringReader(const std::string &t_input);
  std::string as_string() const override;
  std::vector<char> as_bytes() const override;

private:
  std::string m_input;
};

} // namespace Aloha

#endif // FILE_H_
