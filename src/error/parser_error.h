// parser_error.h
#ifndef PARSER_ERROR_H_
#define PARSER_ERROR_H_

#include <stdexcept>
#include <string>
#include <vector>

class ParserError : public std::runtime_error {
public:
  explicit ParserError(const std::string& message) : std::runtime_error(message) {}
};

class ErrorCollector {
public:
  void add_error(const std::string& message) {
    errors.emplace_back(message);
  }

  const std::vector<std::string>& get_errors() const {
    return errors;
  }

  bool has_errors() const {
    return !errors.empty();
  }

private:
  std::vector<std::string> errors;
};

#endif // PARSER_ERROR_H_
