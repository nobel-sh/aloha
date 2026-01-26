#ifndef AIR_DECL_H_
#define AIR_DECL_H_

namespace aloha
{
  namespace air
  {

    // forward declarations for all air nodes
    //
    // expression nodes
    class IntegerLiteral;
    class FloatLiteral;
    class StringLiteral;
    class BoolLiteral;
    class VarRef;
    class BinaryOp;
    class UnaryOp;
    class Call;
    class StructInstantiation;
    class FieldAccess;
    class ArrayExpr;
    class ArrayAccess;

    // statement nodes
    class VarDecl;
    class Assignment;
    class FieldAssignment;
    class Return;
    class If;
    class ExprStmt;

    // declaration nodes
    class Function;
    class StructDecl;
    class Module;

  } // namespace air
} // namespace aloha

#endif // AIR_DECL_H_
