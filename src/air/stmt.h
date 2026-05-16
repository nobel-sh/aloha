#ifndef AIR_STMT_H_
#define AIR_STMT_H_

#include "air.h"
#include "expr.h"
#include "../ty/ty.h"
#include "../frontend/location.h"
#include <memory>
#include <string>
#include <vector>

namespace aloha
{
  namespace air
  {

    class VarDecl : public Stmt
    {
    public:
      std::string m_name;
      VarId m_var_id;
      bool m_is_mutable;
      TyId m_var_ty;
      ExprPtr m_initializer;

      VarDecl(const Location &loc, const std::string &name, VarId var_id,
              bool is_mutable, TyId var_ty, ExprPtr initializer)
          : Stmt(loc), m_name(name), m_var_id(var_id), m_is_mutable(is_mutable),
            m_var_ty(var_ty), m_initializer(std::move(initializer)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class Assignment : public Stmt
    {
    public:
      std::string m_var_name;
      VarId m_var_id;
      ExprPtr m_value;

      Assignment(const Location &loc, const std::string &var_name, VarId var_id, ExprPtr value)
          : Stmt(loc), m_var_name(var_name), m_var_id(var_id), m_value(std::move(value)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class ArrayAssignment : public Stmt
    {
    public:
      std::string m_array_name;
      VarId m_array_var_id;
      TyId m_element_ty;
      ExprPtr m_index;
      ExprPtr m_value;

      ArrayAssignment(const Location &loc, const std::string &array_name,
                      VarId array_var_id, TyId element_ty, ExprPtr index, ExprPtr value)
          : Stmt(loc), m_array_name(array_name), m_array_var_id(array_var_id),
            m_element_ty(element_ty), m_index(std::move(index)), m_value(std::move(value)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class FieldAssignment : public Stmt
    {
    public:
      ExprPtr m_object;
      std::string m_field_name;
      uint32_t m_field_index;
      ExprPtr m_value;

      FieldAssignment(const Location &loc, ExprPtr object, const std::string &field_name,
                      uint32_t field_index, ExprPtr value)
          : Stmt(loc), m_object(std::move(object)), m_field_name(field_name),
            m_field_index(field_index), m_value(std::move(value)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class Return : public Stmt
    {
    public:
      ExprPtr m_value;

      Return(const Location &loc, ExprPtr value)
          : Stmt(loc), m_value(std::move(value)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class Break : public Stmt
    {
    public:
      explicit Break(const Location &loc) : Stmt(loc) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class Continue : public Stmt
    {
    public:
      explicit Continue(const Location &loc) : Stmt(loc) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class If : public Stmt
    {
    public:
      ExprPtr m_condition; // must be TyIds::BOOL
      std::vector<StmtPtr> m_then_branch;
      std::vector<StmtPtr> m_else_branch;

      If(const Location &loc, ExprPtr condition,
         std::vector<StmtPtr> then_branch, std::vector<StmtPtr> else_branch)
          : Stmt(loc), m_condition(std::move(condition)),
            m_then_branch(std::move(then_branch)), m_else_branch(std::move(else_branch)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    struct MatchArm
    {
      bool m_is_wildcard;
      std::string m_enum_name;
      std::string m_variant_name;
      uint32_t m_variant_value;
      std::vector<StmtPtr> m_body;
      Location m_loc;

      MatchArm(const std::string &enum_name, const std::string &variant_name,
               uint32_t variant_value, std::vector<StmtPtr> body,
               const Location &loc)
          : m_is_wildcard(false), m_enum_name(enum_name),
            m_variant_name(variant_name), m_variant_value(variant_value),
            m_body(std::move(body)), m_loc(loc) {}

      MatchArm(std::vector<StmtPtr> body, const Location &loc)
          : m_is_wildcard(true), m_variant_value(0),
            m_body(std::move(body)), m_loc(loc) {}
    };

    class Match : public Stmt
    {
    public:
      ExprPtr m_scrutinee;
      std::vector<MatchArm> m_arms;

      Match(const Location &loc, ExprPtr scrutinee, std::vector<MatchArm> arms)
          : Stmt(loc), m_scrutinee(std::move(scrutinee)),
            m_arms(std::move(arms)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class While : public Stmt
    {
    public:
      ExprPtr m_condition;
      std::vector<StmtPtr> m_body;

      While(const Location &loc, ExprPtr condition, std::vector<StmtPtr> body)
          : Stmt(loc), m_condition(std::move(condition)), m_body(std::move(body)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    class ExprStmt : public Stmt
    {
    public:
      ExprPtr m_expression;

      ExprStmt(const Location &loc, ExprPtr expression)
          : Stmt(loc), m_expression(std::move(expression)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    struct Param
    {
      std::string m_name;
      VarId m_var_id;
      TyId m_ty;
      bool m_is_mutable;
      Location m_loc;

      Param(const std::string &name, VarId var_id, TyId ty, bool is_mutable, const Location &loc)
          : m_name(name), m_var_id(var_id), m_ty(ty), m_is_mutable(is_mutable), m_loc(loc) {}
    };

    class Function : public Node
    {
    public:
      std::string m_name;
      FunctionId m_func_id;
      std::vector<Param> m_params;
      TyId m_return_ty;
      std::vector<StmtPtr> m_body;
      bool m_is_extern;

      Function(const Location &loc, const std::string &name, FunctionId func_id,
               std::vector<Param> params, TyId return_ty,
               std::vector<StmtPtr> body, bool is_extern)
          : Node(loc), m_name(name), m_func_id(func_id), m_params(std::move(params)),
            m_return_ty(return_ty), m_body(std::move(body)), m_is_extern(is_extern) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }
    };

    struct Field
    {
      std::string m_name;
      TyId m_ty;
      uint32_t m_index; // field index for codegen
      Location m_loc;

      Field(const std::string &name, TyId ty, uint32_t index, const Location &loc)
          : m_name(name), m_ty(ty), m_index(index), m_loc(loc) {}
    };

    class StructDecl : public Node
    {
    public:
      std::string m_name;
      StructId m_struct_id;
      TyId m_ty_id; // type id for this struct
      std::vector<Field> m_fields;

      StructDecl(const Location &loc, const std::string &name, StructId struct_id,
                 TyId ty_id, std::vector<Field> fields)
          : Node(loc), m_name(name), m_struct_id(struct_id), m_ty_id(ty_id),
            m_fields(std::move(fields)) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }

      int find_field_index(const std::string &field_name) const
      {
        for (size_t i = 0; i < m_fields.size(); ++i)
        {
          if (m_fields[i].m_name == field_name)
          {
            return static_cast<int>(i);
          }
        }
        return -1; // not found
      }

      const Field *get_field(const std::string &field_name) const
      {
        for (const auto &field : m_fields)
        {
          if (field.m_name == field_name)
          {
            return &field;
          }
        }
        return nullptr;
      }
    };

    class Module : public Node
    {
    public:
      std::string m_name;
      std::vector<StructDeclPtr> m_structs;
      std::vector<FunctionPtr> m_functions;
      std::vector<std::string> m_imports;

      Module(const Location &loc, const std::string &name)
          : Node(loc), m_name(name) {}

      void accept(AIRVisitor &visitor) override { visitor.visit(this); }

      StructDecl *find_struct(const std::string &struct_name)
      {
        for (auto &s : m_structs)
        {
          if (s->m_name == struct_name)
          {
            return s.get();
          }
        }
        return nullptr;
      }

      Function *find_function(const std::string &func_name)
      {
        for (auto &f : m_functions)
        {
          if (f->m_name == func_name)
          {
            return f.get();
          }
        }
        return nullptr;
      }
    };

  } // namespace air
} // namespace aloha

#endif // AIR_STMT_H_
