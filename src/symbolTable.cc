#include "symbolTable.h"
#include "type.h"
#include <utility>

SymbolTable::SymbolTable() : struct_id_counter(0) { enterScope(); }

bool SymbolTable::addVariable(const std::string &name, AlohaType::Type type,
                              bool is_assigned, bool is_mutable) {
  auto &currentScope = variable_table_stack.back();
  if (currentScope.find(name) != currentScope.end()) {
    return false;
  }
  // variableTableStack.back()[name] = {type, true};
  currentScope[name] = {type, is_assigned, is_mutable};
  return true;
}

bool SymbolTable::addFunction(
    const std::string &name, AlohaType::Type returnType,
    const std::vector<AlohaType::Type> &parameterTypes) {
  if (function_table.find(name) != function_table.end()) {
    return false;
  }
  function_table[name] = {returnType, parameterTypes, true};
  return true;
}

AlohaType::Type SymbolTable::addStruct(const std::string &name,
                                       const std::vector<StructField> &fields) {
  if (struct_table.find(name) != struct_table.end()) {
    return AlohaType::Type::UNKNOWN;
  }
  AlohaType::Type new_type = AlohaType::create_struct_type(struct_id_counter++);
  struct_table[name] = {new_type, fields};
  struct_name[new_type] = name;
  return new_type;
}

VariableInfo *SymbolTable::getVariable(const std::string &name) {
  for (auto it = variable_table_stack.rbegin();
       it != variable_table_stack.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

FunctionInfo *SymbolTable::getFunction(const std::string &name) {
  auto it = function_table.find(name);
  if (it != function_table.end()) {
    return &it->second;
  }
  return nullptr;
}

StructInfo *SymbolTable::getStruct(const std::string &name) {
  auto it = struct_table.find(name);
  return it != struct_table.end() ? &it->second : nullptr;
}

StructInfo *SymbolTable::getStructByType(const AlohaType::Type &type) {
  if (!AlohaType::is_struct_type(type)) {
    throw std::invalid_argument("Not a struct type");
  }
  auto it = struct_name.find(type);
  if (it != struct_name.end()) {
    return getStruct(it->second);
  }
  return nullptr;
}

void SymbolTable::enterScope() { variable_table_stack.push_back({}); }

void SymbolTable::leaveScope() {
  if (!variable_table_stack.empty()) {
    variable_table_stack.pop_back();
  }
}

bool SymbolTable::isBuiltinFunction(std::string name) const {
  auto it =
      std::find(predefined_functions.begin(), predefined_functions.end(), name);
  return it != predefined_functions.end();
}

void SymbolTable::dump() const {
  std::cout << "Symbol Table Dump:" << std::endl;
  std::cout << "Functions:" << std::endl;
  for (const auto &[name, info] : function_table) {
    std::cout << "  Function: " << name
              << " -> Return Type: " << AlohaType::to_string(info.return_type)
              << ", Parameters: ";
    for (const auto &paramType : info.param_types) {
      std::cout << AlohaType::to_string(paramType) << " ";
    }
    std::cout << std::endl;
  }

  std::cout << "Variables:" << std::endl;
  int scopeLevel = 0;
  for (const auto &scope : variable_table_stack) {
    std::cout << "  Scope Level " << scopeLevel++ << ":" << std::endl;
    for (const auto &[name, info] : scope) {
      std::cout << "    Variable: " << name
                << " -> Type: " << AlohaType::to_string(info.type) << std::endl;
    }
  }

  std::cout << "Structs:" << std::endl;
  for (const auto &struct_info : struct_table) {
    std::cout << "  " << struct_info.first << ": "
              << AlohaType::to_string(struct_info.second.type) << " {";
    for (size_t i = 0; i < struct_info.second.fields.size(); ++i) {
      if (i > 0)
        std::cout << ", ";
      std::cout << struct_info.second.fields[i].name << ": "
                << AlohaType::to_string(struct_info.second.fields[i].type);
    }
    std::cout << "}" << std::endl;
  }
}
