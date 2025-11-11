#ifndef TYPE_H_
#define TYPE_H_

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class TypeError
{
public:
  explicit TypeError() {}
  void print_error() const
  {
    for (const std::string &error : errors)
    {
      std::cerr << "Type error: " << error << std::endl;
    }
  }
  void add_error(const std::string &message) { errors.push_back(message); }
  bool is_empty() { return errors.empty(); }

private:
  std::vector<std::string> errors;
};

namespace AlohaType
{
  enum class Type
  {
    NUMBER,
    STRING,
    BOOL,
    VOID,
    UNKNOWN,                  // for types that might be known later on
    USER_DEFINED_START = 1000 // Reserve space for user-defined types
  };

  static std::string to_string(Type type)
  {
    switch (type)
    {
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
      if (static_cast<int>(type) >= static_cast<int>(Type::USER_DEFINED_START))
      {
        return "struct_" +
               std::to_string(static_cast<int>(type) -
                              static_cast<int>(Type::USER_DEFINED_START));
      }
      throw std::invalid_argument("Invalid type");
    }
  }
  static Type from_string(const std::string &type)
  {
    static std::unordered_map<std::string, Type> type_map = {
        {"number", Type::NUMBER},
        {"string", Type::STRING},
        {"bool", Type::BOOL},
        {"void", Type::VOID},
        {"unknown", Type::UNKNOWN}};

    auto it = type_map.find(type);
    if (it != type_map.end())
    {
      return it->second;
    }

    if (type.substr(0, 7) == "struct_")
    {
      int type_id =
          std::stoi(type.substr(7)) + static_cast<int>(Type::USER_DEFINED_START);
      return static_cast<Type>(type_id);
    }

    throw std::invalid_argument("Unknown type: " + type);
  }

  static bool is_struct_type(Type type)
  {
    return static_cast<int>(type) >= static_cast<int>(Type::USER_DEFINED_START);
  }

  static Type create_struct_type(int id)
  {
    return static_cast<Type>(static_cast<int>(Type::USER_DEFINED_START) + id);
  }

}; // namespace AlohaType

#endif // TYPE_H_
