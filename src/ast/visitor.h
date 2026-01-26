#ifndef ASTVISITOR_H_
#define ASTVISITOR_H_

#include "decl.h"

namespace aloha
{
  class ASTVisitor
  {
  public:
    virtual void visit(ast::Integer *node) = 0;
    virtual void visit(ast::Float *node) = 0;
    virtual void visit(ast::Boolean *node) = 0;
    virtual void visit(ast::String *node) = 0;
    virtual void visit(ast::UnaryExpression *node) = 0;
    virtual void visit(ast::BinaryExpression *node) = 0;
    virtual void visit(ast::Identifier *node) = 0;
    virtual void visit(ast::Declaration *node) = 0;
    virtual void visit(ast::Assignment *node) = 0;
    virtual void visit(ast::FunctionCall *node) = 0;
    virtual void visit(ast::ReturnStatement *node) = 0;
    virtual void visit(ast::IfStatement *node) = 0;
    virtual void visit(ast::WhileLoop *node) = 0;
    virtual void visit(ast::ForLoop *node) = 0;
    virtual void visit(ast::Function *node) = 0;
    virtual void visit(ast::StructDecl *node) = 0;
    virtual void visit(ast::StructInstantiation *node) = 0;
    virtual void visit(ast::StructFieldAccess *node) = 0;
    virtual void visit(ast::StructFieldAssignment *node) = 0;
    virtual void visit(ast::Array *node) = 0;
    virtual void visit(ast::ArrayAccess *node) = 0;
    virtual void visit(ast::ExpressionStatement *node) = 0;
    virtual void visit(ast::StatementBlock *node) = 0;
    virtual void visit(ast::Program *node) = 0;
    virtual void visit(ast::Import *node) = 0;
  };
} // namespace aloha

#endif // ASTVISITOR_H_
