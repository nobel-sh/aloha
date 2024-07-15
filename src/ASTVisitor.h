#ifndef ASTVISITOR_H_
#define ASTVISITOR_H_

namespace aloha {
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
class Array;
class StatementList;
class Program;
}; // namespace aloha

class ASTVisitor {
public:
  virtual void visit(aloha::Number *node) = 0;
  virtual void visit(aloha::Boolean *node) = 0;
  virtual void visit(aloha::String *node) = 0;
  virtual void visit(aloha::UnaryExpression *node) = 0;
  virtual void visit(aloha::BinaryExpression *node) = 0;
  virtual void visit(aloha::Identifier *node) = 0;
  virtual void visit(aloha::Declaration *node) = 0;
  virtual void visit(aloha::Assignment *node) = 0;
  virtual void visit(aloha::FunctionCall *node) = 0;
  virtual void visit(aloha::ReturnStatement *node) = 0;
  virtual void visit(aloha::IfStatement *node) = 0;
  virtual void visit(aloha::WhileLoop *node) = 0;
  virtual void visit(aloha::ForLoop *node) = 0;
  virtual void visit(aloha::Function *node) = 0;
  virtual void visit(aloha::StructDecl *node) = 0;
  virtual void visit(aloha::StructInstantiation *node) = 0;
  virtual void visit(aloha::StructFieldAccess *node) = 0;
  virtual void visit(aloha::StructFieldAssignment *node) = 0;
  virtual void visit(aloha::Array *node) = 0;
  virtual void visit(aloha::ExpressionStatement *node) = 0;
  virtual void visit(aloha::StatementList *node) = 0;
  virtual void visit(aloha::Program *node) = 0;
};

#endif // ASTVISITOR_H_
