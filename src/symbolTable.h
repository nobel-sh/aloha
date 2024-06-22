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
  bool isDeclared;
};

struct FunctionInfo {
  AlohaType::Type returnType;
  std::vector<AlohaType::Type> parameterTypes;
  bool isDeclared;
};

class SymbolTable {
public:
  SymbolTable();
  bool addVariable(const std::string &name, AlohaType::Type type);
  bool addFunction(const std::string &name, AlohaType::Type returnType,
                   const std::vector<AlohaType::Type> &parameterTypes);
  VariableInfo *getVariable(const std::string &name);
  FunctionInfo *getFunction(const std::string &name);

  void enterScope();
  void leaveScope();

  bool isBuiltinFunction(std::string name) const;
  void dump() const;

private:
  std::vector<std::unordered_map<std::string, VariableInfo>> variableTableStack;
  std::unordered_map<std::string, FunctionInfo> functionTable;
  std::vector<std::string> predefined_functions = {
      "print", "println", "printNum", "printlnNum", "input"};
};

#endif // SYMBOLTABLE_H_
