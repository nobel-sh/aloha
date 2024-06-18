#include "semantic_analyzer.h"
#include "type.h"
#include <vector>

void SemanticAnalyzer::analyze(Program *program) {
  program->accept(*this);
  if (!error.isEmpty()) {
    throw error;
  }
}

void SemanticAnalyzer::visit(Number *node) {}

void SemanticAnalyzer::visit(Boolean *node) {}

void SemanticAnalyzer::visit(UnaryExpression *node) {
  node->expr->accept(*this);
  node->type = node->expr->getType();
}

void SemanticAnalyzer::visit(BinaryExpression *node) {
  node->left->accept(*this);
  node->right->accept(*this);

  if (node->left->getType() == node->right->getType()) {
    node->type = node->left->getType();
  } else {
    error.addError("Type mismatch in binary expression");
  }
}

void SemanticAnalyzer::visit(Identifier *node) {
  VariableInfo *varInfo = symbolTable.getVariable(node->name);
  if (!varInfo) {
    error.addError("Undeclared variable: " + node->name);
  }
  node->type = varInfo->type;
}

void SemanticAnalyzer::visit(Declaration *node) {
  node->expression->accept(*this);

  if (!node->type) {
    node->type = node->expression->getType();
  }

  if (!symbolTable.addVariable(node->variableName, node->type.value())) {
    error.addError("Variable redeclaration: " + node->variableName);
    // throw TypeError("Variable redeclaration: " + node->variableName);
  }
}

void SemanticAnalyzer::visit(FunctionCall *node) {
  FunctionInfo *funcInfo = symbolTable.getFunction(node->funcName->name);
  if (!funcInfo) {
    error.addError("Undeclared function: " + node->funcName->name);
  }
  if (node->arguments.size() != funcInfo->parameterTypes.size()) {
    error.addError("Argument count mismatch in function call: " +
                   node->funcName->name);
  }
  for (size_t i = 0; i < node->arguments.size(); ++i) {
    node->arguments[i]->accept(*this);
    if (node->arguments[i]->getType() != funcInfo->parameterTypes[i]) {
      error.addError("Argument type mismatch in function call: " +
                     node->funcName->name);
    }
  }
}

void SemanticAnalyzer::visit(ReturnStatement *node) {
  node->expression->accept(*this);

  if (currentFunction &&
      node->expression->getType() != currentFunction->returnType) {
    std::cout << AlohaType::to_string(node->expression->getType()) << std::endl;
    std::cout << AlohaType::to_string(currentFunction->returnType) << std::endl;

    symbolTable.dump();
    error.addError("Return type mismatch in function: " +
                   currentFunction->name->name);
  }
}

void SemanticAnalyzer::visit(IfStatement *node) {
  node->condition->accept(*this);
  symbolTable.enterScope();
  node->thenBranch->accept(*this);
  symbolTable.leaveScope();
  if (node->has_else_branch()) {
    symbolTable.enterScope();
    node->elseBranch->accept(*this);
    symbolTable.leaveScope();
  }
}

void SemanticAnalyzer::visit(WhileLoop *node) {
  node->condition->accept(*this);
  symbolTable.enterScope();
  node->body->accept(*this);
  symbolTable.leaveScope();
}

void SemanticAnalyzer::visit(ForLoop *node) {
  symbolTable.enterScope();
  node->initializer->accept(*this);
  node->condition->accept(*this);
  node->increment->accept(*this);
  for (auto &stmt : node->body) {
    stmt->accept(*this);
  }
  symbolTable.leaveScope();
}

void SemanticAnalyzer::visit(Function *node) {
  std::vector<AlohaType::Type> parameterType;
  for (const auto &param : node->parameters) {
    parameterType.push_back(param.type);
  }
  if (!symbolTable.addFunction(node->name->name, node->returnType,
                               parameterType)) {
    error.addError("Function redeclaration: " + node->name->name);
  }

  symbolTable.enterScope();
  currentFunction = node;

  for (const auto &param : node->parameters) {
    if (!symbolTable.addVariable(param.name, param.type)) {
      error.addError("Parameter redeclaration: " + param.name);
    }
  }
  node->body->accept(*this);
  currentFunction = nullptr;
  symbolTable.leaveScope();
}

void SemanticAnalyzer::visit(StatementList *node) {
  for (auto &stmt : node->statements) {
    stmt->accept(*this);
  }
}

void SemanticAnalyzer::visit(Program *node) {
  for (auto &n : node->nodes) {
    n->accept(*this);
  }
}
