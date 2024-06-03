#ifndef TYPE_H_
#define TYPE_H_

#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <stdexcept>

class TypeError : public std::runtime_error {
public:
    explicit TypeError(const std::string& message) : std::runtime_error(message) {}
};


class AlohaType {
public:
  enum class Type {
    INT,
    FLOAT,
    STRING,
    VOID,
    UNKNOWN, // for types that might be known later on
  };
  static std::string to_string(Type type) {
    switch (type) {
    case Type::INT:
      return "INT";
    case Type::FLOAT:
      return "FLOAT";
    case Type::STRING:
      return "STRING";
    case Type::VOID:
      return "VOID";
    case Type::UNKNOWN:
      return "UNKNOWN";
    default:
      throw std::invalid_argument("Invalid type");
    }
  }
  static Type from_string(const std::string &type) {
    if (type == "INT")
      return Type::INT;
    else if (type == "FLOAT")
      return Type::FLOAT;
    else if (type == "STRING")
      return Type::STRING;
    else if (type == "VOID")
      return Type::VOID;
    else if (type == "UNKNOWN")
        return Type::UNKNOWN;
    else
      throw std::invalid_argument("Unknown type");
  }
};

#endif // TYPE_H_
