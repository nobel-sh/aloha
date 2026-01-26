#ifndef AIR_VISITOR_H_
#define AIR_VISITOR_H_

#include "decl.h"

namespace AIR
{
  class AIRVisitor
  {
  public:
    virtual ~AIRVisitor() = default;

    // expression visitors
    virtual void visit(IntegerLiteral *node) = 0;
    virtual void visit(FloatLiteral *node) = 0;
    virtual void visit(StringLiteral *node) = 0;
    virtual void visit(BoolLiteral *node) = 0;
    virtual void visit(VarRef *node) = 0;
    virtual void visit(BinaryOp *node) = 0;
    virtual void visit(UnaryOp *node) = 0;
    virtual void visit(Call *node) = 0;
    virtual void visit(StructInstantiation *node) = 0;
    virtual void visit(FieldAccess *node) = 0;
    virtual void visit(ArrayExpr *node) = 0;
    virtual void visit(ArrayAccess *node) = 0;

    // statement visitors
    virtual void visit(VarDecl *node) = 0;
    virtual void visit(Assignment *node) = 0;
    virtual void visit(FieldAssignment *node) = 0;
    virtual void visit(Return *node) = 0;
    virtual void visit(If *node) = 0;
    virtual void visit(ExprStmt *node) = 0;

    // declaration visitors
    virtual void visit(Function *node) = 0;
    virtual void visit(StructDecl *node) = 0;
    virtual void visit(Module *node) = 0;
  };

} // namespace AIR

#endif // AIR_VISITOR_H_
