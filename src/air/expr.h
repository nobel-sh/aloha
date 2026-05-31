#ifndef AIR_EXPR_H_
#define AIR_EXPR_H_

#include "air.h"
#include "../ty/ty.h"
#include "../frontend/location.h"
#include <memory>
#include <string>
#include <vector>

namespace aloha
{
  namespace air
  {
    class IntegerLiteral : public Expr
    {
    public:
      int64_t m_value;

      IntegerLiteral(const Location &loc, int64_t value)
          : Expr(loc, TyIds::INTEGER), m_value(value) {}
      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class FloatLiteral : public Expr
    {
    public:
      double m_value;

      FloatLiteral(const Location &loc, double value)
          : Expr(loc, TyIds::FLOAT), m_value(value) {}
      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class StringLiteral : public Expr
    {
    public:
      std::string m_value;

      StringLiteral(const Location &loc, const std::string &value)
          : Expr(loc, TyIds::STRING), m_value(value) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class BoolLiteral : public Expr
    {
    public:
      bool m_value;

      BoolLiteral(const Location &loc, bool value)
          : Expr(loc, TyIds::BOOL), m_value(value) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    using VarId = uint32_t;

    class VarRef : public Expr
    {
    public:
      std::string m_name;
      VarId m_var_id; // resolved variable reference

      VarRef(const Location &loc, const std::string &name, VarId var_id, TyId ty)
          : Expr(loc, ty), m_name(name), m_var_id(var_id) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class EnumValue : public Expr
    {
    public:
      std::string m_enum_name;
      std::string m_variant_name;
      uint32_t m_value;

      EnumValue(const Location &loc, const std::string &enum_name,
                const std::string &variant_name, uint32_t value, TyId ty)
          : Expr(loc, ty), m_enum_name(enum_name), m_variant_name(variant_name),
            m_value(value) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    struct MatchExprArm
    {
      bool m_is_wildcard;
      std::string m_enum_name;
      std::string m_variant_name;
      uint32_t m_variant_value;
      ExprPtr m_value;
      Location m_loc;

      MatchExprArm(const std::string &enum_name, const std::string &variant_name,
                   uint32_t variant_value, ExprPtr value, const Location &loc)
          : m_is_wildcard(false), m_enum_name(enum_name),
            m_variant_name(variant_name), m_variant_value(variant_value),
            m_value(std::move(value)), m_loc(loc) {}

      MatchExprArm(ExprPtr value, const Location &loc)
          : m_is_wildcard(true), m_variant_value(0),
            m_value(std::move(value)), m_loc(loc) {}
    };

    class MatchExpr : public Expr
    {
    public:
      ExprPtr m_scrutinee;
      std::vector<MatchExprArm> m_arms;

      MatchExpr(const Location &loc, ExprPtr scrutinee,
                std::vector<MatchExprArm> arms, TyId result_ty)
          : Expr(loc, result_ty), m_scrutinee(std::move(scrutinee)),
            m_arms(std::move(arms)) {}

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
      LOGICAL_AND,
      LOGICAL_OR,
    };

    class BinaryOp : public Expr
    {
    public:
      BinaryOpKind m_op;
      ExprPtr m_left;
      ExprPtr m_right;

      BinaryOp(const Location &loc, BinaryOpKind op, ExprPtr left, ExprPtr right, TyId result_ty)
          : Expr(loc, result_ty), m_op(op), m_left(std::move(left)), m_right(std::move(right)) {}

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
        case BinaryOpKind::LOGICAL_AND:
          return "&&";
        case BinaryOpKind::LOGICAL_OR:
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
        case BinaryOpKind::LOGICAL_AND:
        case BinaryOpKind::LOGICAL_OR:
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
      UnaryOpKind m_op;
      ExprPtr m_operand;

      UnaryOp(const Location &loc, UnaryOpKind op, ExprPtr operand, TyId result_ty)
          : Expr(loc, result_ty), m_op(op), m_operand(std::move(operand)) {}

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
      std::string m_function_name;
      FunctionId m_func_id; // resolved function reference
      std::vector<ExprPtr> m_arguments;

      Call(const Location &loc, const std::string &function_name, FunctionId func_id,
           std::vector<ExprPtr> arguments, TyId return_ty)
          : Expr(loc, return_ty), m_function_name(function_name), m_func_id(func_id),
            m_arguments(std::move(arguments)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class StructInstantiation : public Expr
    {
    public:
      std::string m_struct_name;
      StructId m_struct_id; // resolved struct reference
      std::vector<ExprPtr> m_field_values;

      StructInstantiation(const Location &loc, const std::string &struct_name,
                          StructId struct_id, std::vector<ExprPtr> field_values, TyId struct_ty)
          : Expr(loc, struct_ty), m_struct_name(struct_name), m_struct_id(struct_id),
            m_field_values(std::move(field_values)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class NewObject : public Expr
    {
    public:
      std::string m_struct_name;
      StructId m_struct_id;
      ExprPtr m_arena;
      std::vector<ExprPtr> m_field_values;

      NewObject(const Location &loc, const std::string &struct_name,
                StructId struct_id, ExprPtr arena,
                std::vector<ExprPtr> field_values, TyId ref_ty)
          : Expr(loc, ref_ty), m_struct_name(struct_name), m_struct_id(struct_id),
            m_arena(std::move(arena)), m_field_values(std::move(field_values)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class FieldAccess : public Expr
    {
    public:
      ExprPtr m_object;
      std::string m_field_name;
      uint32_t m_field_index; // resolved field index for codegen

      FieldAccess(const Location &loc, ExprPtr object, const std::string &field_name,
                  uint32_t field_index, TyId field_ty)
          : Expr(loc, field_ty), m_object(std::move(object)), m_field_name(field_name),
            m_field_index(field_index) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class ArrayExpr : public Expr
    {
    public:
      std::vector<ExprPtr> m_elements;
      ArrayExpr(const Location &loc, std::vector<ExprPtr> elements, TyId array_ty)
          : Expr(loc, array_ty), m_elements(std::move(elements)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class ArrayAccess : public Expr
    {
    public:
      ExprPtr m_array_expr;
      ExprPtr m_index_expr;

      ArrayAccess(const Location &loc, ExprPtr array_expr, ExprPtr index_expr, TyId element_ty)
          : Expr(loc, element_ty), m_array_expr(std::move(array_expr)), m_index_expr(std::move(index_expr)) {}
      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

  } // namespace air
} // namespace aloha

#endif // AIR_EXPR_H_
