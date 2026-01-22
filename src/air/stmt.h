#ifndef AIR_STMT_H_
#define AIR_STMT_H_

#include "air.h"
#include "expr.h"
#include "../ty/ty.h"
#include "../frontend/location.h"
#include <memory>
#include <string>
#include <vector>

namespace AIR
{

  class VarDecl : public Stmt
  {
  public:
    std::string name;
    VarId var_id;
    bool is_mutable;
    TyId var_ty;
    ExprPtr initializer;

    VarDecl(const Location &loc, const std::string &name, VarId var_id,
            bool is_mutable, TyId var_ty, ExprPtr initializer)
        : Stmt(loc), name(name), var_id(var_id), is_mutable(is_mutable),
          var_ty(var_ty), initializer(std::move(initializer)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  class Assignment : public Stmt
  {
  public:
    std::string var_name;
    VarId var_id;
    ExprPtr value;

    Assignment(const Location &loc, const std::string &var_name, VarId var_id, ExprPtr value)
        : Stmt(loc), var_name(var_name), var_id(var_id), value(std::move(value)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  class FieldAssignment : public Stmt
  {
  public:
    ExprPtr object;
    std::string field_name;
    uint32_t field_index;
    ExprPtr value;

    FieldAssignment(const Location &loc, ExprPtr object, const std::string &field_name,
                    uint32_t field_index, ExprPtr value)
        : Stmt(loc), object(std::move(object)), field_name(field_name),
          field_index(field_index), value(std::move(value)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  class Return : public Stmt
  {
  public:
    ExprPtr value;

    Return(const Location &loc, ExprPtr value)
        : Stmt(loc), value(std::move(value)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  class If : public Stmt
  {
  public:
    ExprPtr condition; // must be TyIds::BOOL
    std::vector<StmtPtr> then_branch;
    std::vector<StmtPtr> else_branch;

    If(const Location &loc, ExprPtr condition,
       std::vector<StmtPtr> then_branch, std::vector<StmtPtr> else_branch)
        : Stmt(loc), condition(std::move(condition)),
          then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  class ExprStmt : public Stmt
  {
  public:
    ExprPtr expression;

    ExprStmt(const Location &loc, ExprPtr expression)
        : Stmt(loc), expression(std::move(expression)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  struct Param
  {
    std::string name;
    VarId var_id;
    TyId ty;
    bool is_mutable;
    Location loc;

    Param(const std::string &name, VarId var_id, TyId ty, bool is_mutable, const Location &loc)
        : name(name), var_id(var_id), ty(ty), is_mutable(is_mutable), loc(loc) {}
  };

  class Function : public Node
  {
  public:
    std::string name;
    FunctionId func_id;
    std::vector<Param> params;
    TyId return_ty;
    std::vector<StmtPtr> body;
    bool is_extern;

    Function(const Location &loc, const std::string &name, FunctionId func_id,
             std::vector<Param> params, TyId return_ty,
             std::vector<StmtPtr> body, bool is_extern)
        : Node(loc), name(name), func_id(func_id), params(std::move(params)),
          return_ty(return_ty), body(std::move(body)), is_extern(is_extern) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }
  };

  struct Field
  {
    std::string name;
    TyId ty;
    uint32_t index; // field index for codegen
    Location loc;

    Field(const std::string &name, TyId ty, uint32_t index, const Location &loc)
        : name(name), ty(ty), index(index), loc(loc) {}
  };

  class StructDecl : public Node
  {
  public:
    std::string name;
    StructId struct_id;
    TyId ty_id; // type id for this struct
    std::vector<Field> fields;

    StructDecl(const Location &loc, const std::string &name, StructId struct_id,
               TyId ty_id, std::vector<Field> fields)
        : Node(loc), name(name), struct_id(struct_id), ty_id(ty_id),
          fields(std::move(fields)) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }

    int find_field_index(const std::string &field_name) const
    {
      for (size_t i = 0; i < fields.size(); ++i)
      {
        if (fields[i].name == field_name)
        {
          return static_cast<int>(i);
        }
      }
      return -1; // not found
    }

    const Field *get_field(const std::string &field_name) const
    {
      for (const auto &field : fields)
      {
        if (field.name == field_name)
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
    std::string name;
    std::vector<StructDeclPtr> structs;
    std::vector<FunctionPtr> functions;
    std::vector<std::string> imports;

    Module(const Location &loc, const std::string &name)
        : Node(loc), name(name) {}

    void accept(AIRVisitor &visitor) override { visitor.visit(this); }

    StructDecl *find_struct(const std::string &struct_name)
    {
      for (auto &s : structs)
      {
        if (s->name == struct_name)
        {
          return s.get();
        }
      }
      return nullptr;
    }

    Function *find_function(const std::string &func_name)
    {
      for (auto &f : functions)
      {
        if (f->name == func_name)
        {
          return f.get();
        }
      }
      return nullptr;
    }
  };

} // namespace AIR

#endif // AIR_STMT_H_
