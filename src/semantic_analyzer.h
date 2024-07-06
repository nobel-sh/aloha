#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ASTVisitor.h"
#include "ast.h"
#include "symbolTable.h"
#include "type.h"

class SemanticAnalyzer : public ASTVisitor {
public:
  void analyze(Aloha::Program *program);

  void visit(Aloha::Number *node) override;
  void visit(Aloha::Boolean *node) override;
  void visit(Aloha::String *node) override;
  void visit(Aloha::ExpressionStatement *node) override;
  void visit(Aloha::UnaryExpression *node) override;
  void visit(Aloha::BinaryExpression *node) override;
  void visit(Aloha::Identifier *node) override;
  void visit(Aloha::Declaration *node) override;
  void visit(Aloha::Assignment *node) override;
  void visit(Aloha::FunctionCall *node) override;
  void visit(Aloha::ReturnStatement *node) override;
  void visit(Aloha::IfStatement *node) override;
  void visit(Aloha::WhileLoop *node) override;
  void visit(Aloha::ForLoop *node) override;
  void visit(Aloha::Function *node) override;
  void visit(Aloha::StructDecl *node) override;
  void visit(Aloha::StructInstantiation *node) override;
  void visit(Aloha::StatementList *node) override;
  void visit(Aloha::Program *node) override;

  void dump_symbol_table() { symbol_table.dump(); }

private:
  SymbolTable symbol_table;
  Aloha::Function *current_fn = nullptr;
  TypeError error;
};

#endif // SEMANTIC_ANALYZER_H
