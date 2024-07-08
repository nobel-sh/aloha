#ifndef ASTVISITOR_H_
#define ASTVISITOR_H_

namespace Aloha {

// forward decls avoiding dependency cycle
class Number;
class Boolean;
class String;
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
class StructDecl;
class StructInstantiation;
class StructFieldAccess;
class StructFieldAssignment;
class ExpressionStatement;
class StatementList;
class Program;

}; // namespace Aloha
class ASTVisitor {
public:
  virtual void visit(Aloha::Number *node) = 0;
  virtual void visit(Aloha::Boolean *node) = 0;
  virtual void visit(Aloha::String *node) = 0;
  virtual void visit(Aloha::UnaryExpression *node) = 0;
  virtual void visit(Aloha::BinaryExpression *node) = 0;
  virtual void visit(Aloha::Identifier *node) = 0;
  virtual void visit(Aloha::Declaration *node) = 0;
  virtual void visit(Aloha::Assignment *node) = 0;
  virtual void visit(Aloha::FunctionCall *node) = 0;
  virtual void visit(Aloha::ReturnStatement *node) = 0;
  virtual void visit(Aloha::IfStatement *node) = 0;
  virtual void visit(Aloha::WhileLoop *node) = 0;
  virtual void visit(Aloha::ForLoop *node) = 0;
  virtual void visit(Aloha::Function *node) = 0;
  virtual void visit(Aloha::StructDecl *node) = 0;
  virtual void visit(Aloha::StructInstantiation *node) = 0;
  virtual void visit(Aloha::StructFieldAccess *node) = 0;
  virtual void visit(Aloha::StructFieldAssignment *node) = 0;
  virtual void visit(Aloha::ExpressionStatement *node) = 0;
  virtual void visit(Aloha::StatementList *node) = 0;
  virtual void visit(Aloha::Program *node) = 0;
};

#endif // ASTVISITOR_H_
