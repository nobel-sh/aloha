#include "symbolTable.h"
#include "type.h"
#include <utility>

SymbolTable::SymbolTable() {
  variableTableStack.push_back({});
}

bool SymbolTable::addVariable(const std::string &name, AlohaType::Type type) {
  if (variableTableStack.back().find(name) != variableTableStack.back().end()) {
    return false;
  }
  variableTableStack.back()[name] = {type, true};
  return true;
}

bool SymbolTable::addFunction(const std::string &name, AlohaType::Type returnType,
                              const std::vector<AlohaType::Type> &parameterTypes) {
  if (functionTable.find(name) != functionTable.end()) {
    return false;
  }
  functionTable[name] = {returnType, parameterTypes, true};
  return true;
}

VariableInfo *SymbolTable::getVariable(const std::string &name) {
  for (auto it = variableTableStack.rbegin(); it != variableTableStack.rend(); ++it) {
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

void SymbolTable::enterScope() {
  variableTableStack.push_back({});
}

void SymbolTable::leaveScope() {
  if (!variableTableStack.empty()) {
    variableTableStack.pop_back();
  }
}
