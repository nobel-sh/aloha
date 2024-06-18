#include "symbolTable.h"
#include "type.h"
#include <utility>

SymbolTable::SymbolTable() { enterScope(); }

bool SymbolTable::addVariable(const std::string &name, AlohaType::Type type) {
  auto &currentScope = variableTableStack.back();
  if (currentScope.find(name) != currentScope.end()) {
    return false;
  }
  // variableTableStack.back()[name] = {type, true};
  currentScope[name] = {type, true};
  return true;
}

bool SymbolTable::addFunction(
    const std::string &name, AlohaType::Type returnType,
    const std::vector<AlohaType::Type> &parameterTypes) {
  if (functionTable.find(name) != functionTable.end()) {
    return false;
  }
  functionTable[name] = {returnType, parameterTypes, true};
  return true;
}

VariableInfo *SymbolTable::getVariable(const std::string &name) {
  for (auto it = variableTableStack.rbegin(); it != variableTableStack.rend();
       ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

FunctionInfo *SymbolTable::getFunction(const std::string &name) {
  auto it = functionTable.find(name);
  if (it != functionTable.end()) {
    return &it->second;
  }
  return nullptr;
}

void SymbolTable::enterScope() { variableTableStack.push_back({}); }

void SymbolTable::leaveScope() {
  if (!variableTableStack.empty()) {
    variableTableStack.pop_back();
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
  for (const auto &[name, info] : functionTable) {
    std::cout << "  Function: " << name
              << " -> Return Type: " << AlohaType::to_string(info.returnType)
              << ", Parameters: ";
    for (const auto &paramType : info.parameterTypes) {
      std::cout << AlohaType::to_string(paramType) << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "Variables:" << std::endl;
  int scopeLevel = 0;
  for (const auto &scope : variableTableStack) {
    std::cout << "  Scope Level " << scopeLevel++ << ":" << std::endl;
    for (const auto &[name, info] : scope) {
      std::cout << "    Variable: " << name
                << " -> Type: " << AlohaType::to_string(info.type) << std::endl;
    }
  }
}
