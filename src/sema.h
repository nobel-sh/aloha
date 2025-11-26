#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast/ast.h"
#include "ast/visitor.h"
#include "symbol_table.h"
#include "type.h"

class SemanticAnalyzer : public ASTVisitor
{
public:
  void analyze(aloha::Program *program);

  void visit(aloha::Number *node) override;
  void visit(aloha::Boolean *node) override;
  void visit(aloha::String *node) override;
  void visit(aloha::ExpressionStatement *node) override;
  void visit(aloha::UnaryExpression *node) override;
  void visit(aloha::BinaryExpression *node) override;
  void visit(aloha::Identifier *node) override;
  void visit(aloha::Declaration *node) override;
  void visit(aloha::Assignment *node) override;
  void visit(aloha::FunctionCall *node) override;
  void visit(aloha::ReturnStatement *node) override;
  void visit(aloha::IfStatement *node) override;
  void visit(aloha::WhileLoop *node) override;
  void visit(aloha::ForLoop *node) override;
  void visit(aloha::Function *node) override;
  void visit(aloha::StructDecl *node) override;
  void visit(aloha::StructInstantiation *node) override;
  void visit(aloha::StructFieldAccess *node) override;
  void visit(aloha::StructFieldAssignment *node) override;
  void visit(aloha::Array *node) override;
  void visit(aloha::StatementBlock *node) override;
  void visit(aloha::Program *node) override;
  void visit(aloha::Import *node) override;

  void dump_symbol_table() { symbol_table.dump(); }

private:
  SymbolTable symbol_table;
  aloha::Function *current_fn = nullptr;
  TypeError error;
};

#endif // SEMANTIC_ANALYZER_H
