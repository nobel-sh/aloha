#ifndef ASTVISITOR_H_
#define ASTVISITOR_H_

// forward decls avoiding dependency cycle
class Number;
class Boolean;
class AlohaString;
class UnaryExpression;
class BinaryExpression;
class Identifier;
class Declaration;
class Assignment;
class FunctionCall;
class ReturnStatement;
class IfStatement;
class WhileLoop;
class ForLoop;
class Function;
class ExpressionStatement;
class StatementList;
class Program;

class ASTVisitor {
public:
  virtual void visit(Number *node) = 0;
  virtual void visit(Boolean *node) = 0;
  virtual void visit(AlohaString *node) = 0;
  virtual void visit(UnaryExpression *node) = 0;
  virtual void visit(BinaryExpression *node) = 0;
  virtual void visit(Identifier *node) = 0;
  virtual void visit(Declaration *node) = 0;
  virtual void visit(Assignment *node) = 0;
  virtual void visit(FunctionCall *node) = 0;
  virtual void visit(ReturnStatement *node) = 0;
  virtual void visit(IfStatement *node) = 0;
  virtual void visit(WhileLoop *node) = 0;
  virtual void visit(ForLoop *node) = 0;
  virtual void visit(Function *node) = 0;
  virtual void visit(ExpressionStatement *node) = 0;
  virtual void visit(StatementList *node) = 0;
  virtual void visit(Program *node) = 0;
};

#endif // ASTVISITOR_H_
