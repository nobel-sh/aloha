#include "semantic_analyzer.h"

void SemanticAnalyzer::analyze(Program *program) { program->accept(*this); }

void SemanticAnalyzer::visit(Number *node) {}

void SemanticAnalyzer::visit(UnaryExpression *node) {
  node->expr->accept(*this);
}

void SemanticAnalyzer::visit(BinaryExpression *node) {
  node->left->accept(*this);
  node->right->accept(*this);
}

void SemanticAnalyzer::visit(Identifier *node) {
  if (!symbolTable.getVariable(node->name)) {
    throw TypeError("Undeclared variable: " + node->name);
  }
}

void SemanticAnalyzer::visit(Declaration *node) {
  if (!symbolTable.addVariable(node->variableName,
                               node->expression->getType())) {
    throw TypeError("Variable redeclaration: " + node->variableName);
  }
  node->expression->accept(*this);
}

void SemanticAnalyzer::visit(FunctionCall *node) {
  FunctionInfo *funcInfo = symbolTable.getFunction(node->functionName);
  if (!funcInfo) {
    throw TypeError("Undeclared function: " + node->functionName);
  }
  if (node->arguments.size() != funcInfo->parameterTypes.size()) {
    throw TypeError("Argument count mismatch in function call: " +
                    node->functionName);
  }
  for (size_t i = 0; i < node->arguments.size(); ++i) {
    node->arguments[i]->accept(*this);
    if (node->arguments[i]->getType() != funcInfo->parameterTypes[i]) {
      throw TypeError("Argument type mismatch in function call: " +
                      node->functionName);
    }
  }
}

void SemanticAnalyzer::visit(ReturnStatement *node) {
  node->expression->accept(*this);
}

void SemanticAnalyzer::visit(IfStatement *node) {
  node->condition->accept(*this);
  node->thenBranch->accept(*this);
  if (node->has_else_branch()) {
    node->elseBranch->accept(*this);
  }
}

void SemanticAnalyzer::visit(WhileLoop *node) {
  node->condition->accept(*this);
  for (auto &stmt : node->body) {
    stmt->accept(*this);
  }
}

void SemanticAnalyzer::visit(ForLoop *node) {
  node->initializer->accept(*this);
  node->condition->accept(*this);
  node->increment->accept(*this);
  for (auto &stmt : node->body) {
    stmt->accept(*this);
  }
}

void SemanticAnalyzer::visit(Function *node) {
  if (!symbolTable.addFunction(node->name->name, node->returnType, {})) {
    throw TypeError("Function redeclaration: " + node->name->name);
  }
  node->body->accept(*this);
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
