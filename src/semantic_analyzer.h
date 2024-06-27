#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ASTVisitor.h"
#include "ast.h"
#include "symbolTable.h"
#include "type.h"

class SemanticAnalyzer : public ASTVisitor {
public:
  void analyze(Program *program);

  void visit(Number *node) override;
  void visit(Boolean *node) override;
  void visit(AlohaString *node) override;
  void visit(ExpressionStatement *node) override;
  void visit(UnaryExpression *node) override;
  void visit(BinaryExpression *node) override;
  void visit(Identifier *node) override;
  void visit(Declaration *node) override;
  void visit(FunctionCall *node) override;
  void visit(ReturnStatement *node) override;
  void visit(IfStatement *node) override;
  void visit(WhileLoop *node) override;
  void visit(ForLoop *node) override;
  void visit(Function *node) override;
  void visit(StatementList *node) override;
  void visit(Program *node) override;

  void dump_symbol_table() { symbol_table.dump(); }

private:
  SymbolTable symbol_table;
  Function *current_fn = nullptr;
  TypeError error;
};

#endif // SEMANTIC_ANALYZER_H
