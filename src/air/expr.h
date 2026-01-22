#ifndef AIR_EXPR_H_
#define AIR_EXPR_H_

#include "air.h"
#include "../ty/ty.h"
#include "../frontend/location.h"
#include <memory>
#include <string>
#include <vector>

namespace AIR
{

  class NumberLiteral : public Expr
  {
  public:
    double value;

    NumberLiteral(const Location &loc, double value)
        : Expr(loc, TyIds::NUMBER), value(value) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  class StringLiteral : public Expr
  {
  public:
    std::string value;

    StringLiteral(const Location &loc, const std::string &value)
        : Expr(loc, TyIds::STRING), value(value) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  class BoolLiteral : public Expr
  {
  public:
    bool value;

    BoolLiteral(const Location &loc, bool value)
        : Expr(loc, TyIds::BOOL), value(value) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  using VarId = uint32_t;

  class VarRef : public Expr
  {
  public:
    std::string name;
    VarId var_id; // resolved variable reference

    VarRef(const Location &loc, const std::string &name, VarId var_id, TyId ty)
        : Expr(loc, ty), name(name), var_id(var_id) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  enum class BinaryOpKind
  {
    // arithmetic
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,

    // comparison
    EQ,
    NE,
    LT,
    LE,
    GT,
    GE,

    // logical
    AND,
    OR,
  };

  class BinaryOp : public Expr
  {
  public:
    BinaryOpKind op;
    ExprPtr left;
    ExprPtr right;

    BinaryOp(const Location &loc, BinaryOpKind op, ExprPtr left, ExprPtr right, TyId result_ty)
        : Expr(loc, result_ty), op(op), left(std::move(left)), right(std::move(right)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }

    static const char *op_to_string(BinaryOpKind op)
    {
      switch (op)
      {
      case BinaryOpKind::ADD:
        return "+";
      case BinaryOpKind::SUB:
        return "-";
      case BinaryOpKind::MUL:
        return "*";
      case BinaryOpKind::DIV:
        return "/";
      case BinaryOpKind::MOD:
        return "%";
      case BinaryOpKind::EQ:
        return "==";
      case BinaryOpKind::NE:
        return "!=";
      case BinaryOpKind::LT:
        return "<";
      case BinaryOpKind::LE:
        return "<=";
      case BinaryOpKind::GT:
        return ">";
      case BinaryOpKind::GE:
        return ">=";
      case BinaryOpKind::AND:
        return "&&";
      case BinaryOpKind::OR:
        return "||";
      default:
        return "<unknown>";
      }
    }

    static TyId result_type(BinaryOpKind op, TyId operand_ty)
    {
      switch (op)
      {
      case BinaryOpKind::ADD:
      case BinaryOpKind::SUB:
      case BinaryOpKind::MUL:
      case BinaryOpKind::DIV:
      case BinaryOpKind::MOD:
        return operand_ty;

      case BinaryOpKind::EQ:
      case BinaryOpKind::NE:
      case BinaryOpKind::LT:
      case BinaryOpKind::LE:
      case BinaryOpKind::GT:
      case BinaryOpKind::GE:
      case BinaryOpKind::AND:
      case BinaryOpKind::OR:
        return TyIds::BOOL;

      default:
        return TyIds::ERROR;
      }
    }
  };

  enum class UnaryOpKind
  {
    NEG, // -x
    NOT, // !x
  };

  class UnaryOp : public Expr
  {
  public:
    UnaryOpKind op;
    ExprPtr operand;

    UnaryOp(const Location &loc, UnaryOpKind op, ExprPtr operand, TyId result_ty)
        : Expr(loc, result_ty), op(op), operand(std::move(operand)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }

    static const char *op_to_string(UnaryOpKind op)
    {
      switch (op)
      {
      case UnaryOpKind::NEG:
        return "-";
      case UnaryOpKind::NOT:
        return "!";
      default:
        return "<unknown>";
      }
    }
  };

  using FunctionId = uint32_t;

  class Call : public Expr
  {
  public:
    std::string function_name;
    FunctionId func_id; // resolved function reference
    std::vector<ExprPtr> arguments;

    Call(const Location &loc, const std::string &function_name, FunctionId func_id,
         std::vector<ExprPtr> arguments, TyId return_ty)
        : Expr(loc, return_ty), function_name(function_name), func_id(func_id),
          arguments(std::move(arguments)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  class StructInstantiation : public Expr
  {
  public:
    std::string struct_name;
    StructId struct_id; // resolved struct reference
    std::vector<ExprPtr> field_values;

    StructInstantiation(const Location &loc, const std::string &struct_name,
                        StructId struct_id, std::vector<ExprPtr> field_values, TyId struct_ty)
        : Expr(loc, struct_ty), struct_name(struct_name), struct_id(struct_id),
          field_values(std::move(field_values)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  class FieldAccess : public Expr
  {
  public:
    ExprPtr object;
    std::string field_name;
    uint32_t field_index; // resolved field index for codegen

    FieldAccess(const Location &loc, ExprPtr object, const std::string &field_name,
                uint32_t field_index, TyId field_ty)
        : Expr(loc, field_ty), object(std::move(object)), field_name(field_name),
          field_index(field_index) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

} // namespace AIR

#endif // AIR_EXPR_H_
