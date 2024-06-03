#include "symbolTable.h"
#include "type.h"

bool SymbolTable::addVariable(const std::string &name, AlohaType::Type type) {
  if (variableTable.find(name) != variableTable.end()) {
    return false;
  }
  variableTable[name] = {type, true};
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
  auto it = variableTable.find(name);
  if (it != variableTable.end()) {
    return &it->second;
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
