#include "symbolTable.h"
#include "type.h"
#include <utility>

SymbolTable::SymbolTable() { enterScope(); }

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
}
