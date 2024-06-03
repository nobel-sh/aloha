#ifndef SYMBOLTABLE_H_
#define SYMBOLTABLE_H_

#include "type.h"
#include <string>
#include <unordered_map>
#include <vector>

struct VariableInfo {
  AlohaType::Type type;
  bool isDeclared;
};

struct FunctionInfo {
  AlohaType::Type returnType;
  std::vector<AlohaType::Type> parameterTypes;
  bool isDeclared;
};

class SymbolTable {
public:
  bool addVariable(const std::string &name, AlohaType::Type type);
  bool addFunction(const std::string &name, AlohaType::Type returnType,
                   const std::vector<AlohaType::Type> &parameterTypes);
  VariableInfo *getVariable(const std::string &name);
  FunctionInfo *getFunction(const std::string &name);

private:
  std::unordered_map<std::string, VariableInfo> variableTable;
  std::unordered_map<std::string, FunctionInfo> functionTable;
};

#endif // SYMBOLTABLE_H_
