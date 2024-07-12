#ifndef SYMBOLTABLE_H_
#define SYMBOLTABLE_H_

#include "type.h"
#include <algorithm>
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

struct VariableInfo {
  AlohaType::Type type;
  bool is_assigned;
  bool is_mutable;
};

struct FunctionInfo {
  AlohaType::Type return_type;
  std::vector<AlohaType::Type> param_types;
  bool is_declared;
};

struct StructField {
  std::string name;
  AlohaType::Type type;
};

struct StructInfo {
  AlohaType::Type type;
  std::vector<StructField> fields;
};

class SymbolTable {
public:
  SymbolTable();
  bool add_variable(const std::string &name, AlohaType::Type type,
                    bool is_assigned, bool is_mutable);
  bool add_function(const std::string &name, AlohaType::Type return_type,
                    const std::vector<AlohaType::Type> &param_types);
  AlohaType::Type add_struct(const std::string &name,
                             const std::vector<StructField> &fields);
  VariableInfo *get_variable(const std::string &name);
  FunctionInfo *get_function(const std::string &name);
  StructInfo *get_struct(const std::string &name);
  StructInfo *get_struct_by_type(const AlohaType::Type &type);
  void enter_scope();
  void leave_scope();
  bool is_builtin_function(std::string name) const;
  void dump() const;

private:
  std::vector<std::unordered_map<std::string, VariableInfo>>
      variable_table_stack;
  std::unordered_map<std::string, FunctionInfo> function_table;
  std::unordered_map<std::string, StructInfo> struct_table;
  std::unordered_map<AlohaType::Type, std::string> struct_name;
  int struct_id_counter;
  std::vector<std::string> predefined_functions = {
      "print", "println", "printNum", "printlnNum", "input"};
};

#endif // SYMBOLTABLE_H_
