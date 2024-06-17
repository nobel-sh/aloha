#ifndef TYPE_H_
#define TYPE_H_

#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>

class TypeError : public std::runtime_error {
public:
  explicit TypeError(const std::string &message)
      : std::runtime_error(message) {}
};

namespace AlohaType {
enum class Type {
  NUMBER,
  STRING,
  BOOL,
  VOID,
  UNKNOWN, // for types that might be known later on
};
static std::string to_string(Type type) {
  switch (type) {
  case Type::NUMBER:
    return "number";
  case Type::STRING:
    return "string";
  case Type::BOOL:
    return "bool";
  case Type::VOID:
    return "void";
  case Type::UNKNOWN:
    return "unknown";
  default:
    throw std::invalid_argument("Invalid type");
  }
}
static Type from_string(const std::string &type) {
  if (type == "number")
    return Type::NUMBER;
  else if (type == "string")
    return Type::STRING;
  else if (type == "bool")
    return Type::BOOL;
  else if (type == "void")
    return Type::VOID;
  else if (type == "unknown")
    return Type::UNKNOWN;
  else
    throw std::invalid_argument("Unknown type");
}
}; // namespace AlohaType

#endif // TYPE_H_
