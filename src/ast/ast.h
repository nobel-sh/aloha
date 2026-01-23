#ifndef ALOHA_AST_H_
#define ALOHA_AST_H_

#include "decl.h"
#include "visitor.h"
#include "ty_spec.h"
#include "../frontend/location.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace aloha
{

  // Type alias for ast type annotations
  using Type = TySpecId;

  class Node
  {
  public:
    explicit Node(Location loc) : m_loc(loc) {}
    virtual ~Node() = default;
    virtual void write(std::ostream &os, unsigned long indent = 0) const = 0;
    virtual void accept(ASTVisitor &visitor) = 0;
    virtual Location get_location() const { return m_loc; }

  public:
    Location m_loc;
  };

  class Expression : public Node
  {
  public:
    explicit Expression(Location loc) : Node(loc) {}
  };

  class Statement : public Node
  {
  public:
    explicit Statement(Location loc) : Node(loc) {}
  };

  using NodePtr = std::unique_ptr<Node>;
  using ExprPtr = std::unique_ptr<Expression>;
  using StmtPtr = std::unique_ptr<Statement>;

  class StatementBlock : public Statement
  {
  public:
    std::vector<StmtPtr> m_statements;

    explicit StatementBlock(Location loc, std::vector<StmtPtr> stmts = {});
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
    bool is_empty() const;
  };

  class ExpressionStatement : public Statement
  {
  public:
    ExprPtr m_expr;

    explicit ExpressionStatement(Location loc, ExprPtr expr);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class Integer : public Expression
  {
  public:
    int64_t m_value;

    explicit Integer(Location loc, int64_t val);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class Float : public Expression
  {
  public:
    double m_value;

    explicit Float(Location loc, double val);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class Boolean : public Expression
  {
  public:
    bool m_value;

    explicit Boolean(Location loc, bool val);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class String : public Expression
  {
  public:
    std::string m_value;

    explicit String(Location loc, std::string val);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class UnaryExpression : public Expression
  {
  public:
    std::string m_op;
    ExprPtr m_expr;

    UnaryExpression(Location loc, std::string oper, ExprPtr expr);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class BinaryExpression : public Expression
  {
  public:
    ExprPtr m_left;
    std::string m_op;
    ExprPtr m_right;

    BinaryExpression(Location loc, ExprPtr lhs, std::string oper, ExprPtr rhs);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class Identifier : public Expression
  {
  public:
    std::string m_name;

    explicit Identifier(Location loc, std::string name);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class StructFieldAccess : public Expression
  {
  public:
    ExprPtr m_struct_expr;
    std::string m_field_name;

    StructFieldAccess(Location loc, ExprPtr struct_expr, std::string field_name);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class StructFieldAssignment : public Statement
  {
  public:
    ExprPtr m_struct_expr;
    std::string m_field_name;
    ExprPtr m_value;

    StructFieldAssignment(Location loc, ExprPtr struct_expr,
                          std::string field_name, ExprPtr value);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class Declaration : public Statement
  {
  public:
    std::string m_variable_name;
    std::optional<Type> m_type;
    ExprPtr m_expression;
    bool m_is_assigned;
    bool m_is_mutable;

    Declaration(Location loc, std::string var_name, std::optional<Type> type,
                ExprPtr expr, bool is_mutable);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class Assignment : public Statement
  {
  public:
    std::string m_variable_name;
    ExprPtr m_expression;

    Assignment(Location loc, std::string var_name, ExprPtr expr);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class FunctionCall : public Expression
  {
  public:
    std::unique_ptr<Identifier> m_func_name;
    std::vector<ExprPtr> m_arguments;

    FunctionCall(Location loc, std::unique_ptr<Identifier> func_name,
                 std::vector<ExprPtr> args);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class ReturnStatement : public Statement
  {
  public:
    ExprPtr m_expression;

    explicit ReturnStatement(Location loc, ExprPtr expr);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class IfStatement : public Statement
  {
  public:
    ExprPtr m_condition;
    std::unique_ptr<StatementBlock> m_then_branch;
    std::unique_ptr<StatementBlock> m_else_branch;

    IfStatement(Location loc, ExprPtr cond,
                std::unique_ptr<StatementBlock> then_branch,
                std::unique_ptr<StatementBlock> else_branch);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
    bool has_else_branch() const;
  };

  class WhileLoop : public Statement
  {
  public:
    ExprPtr m_condition;
    std::unique_ptr<StatementBlock> m_body;

    WhileLoop(Location loc, ExprPtr cond, std::unique_ptr<StatementBlock> body);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class ForLoop : public Statement
  {
  public:
    std::unique_ptr<Declaration> m_initializer;
    ExprPtr m_condition;
    std::unique_ptr<Declaration> m_increment;
    std::vector<StmtPtr> m_body;

    ForLoop(Location loc, std::unique_ptr<Declaration> init, ExprPtr cond,
            std::unique_ptr<Declaration> inc, std::vector<StmtPtr> body);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class Parameter
  {
  public:
    std::string m_name;
    Type m_type;

    Parameter(std::string name, Type type);
    Parameter(std::string name, Type type, std::string type_name);
  };

  class Function : public Statement
  {
  public:
    std::unique_ptr<Identifier> m_name;
    std::vector<Parameter> m_parameters;
    Type m_return_type;
    std::unique_ptr<StatementBlock> m_body;
    bool m_is_extern;

    Function(Location loc, std::unique_ptr<Identifier> func_name,
             std::vector<Parameter> params, Type return_type,
             std::unique_ptr<StatementBlock> body, bool is_extern = false);
    Function(Location loc, std::unique_ptr<Identifier> func_name,
             std::vector<Parameter> params, Type return_type, std::string return_type_name,
             std::unique_ptr<StatementBlock> body, bool is_extern = false);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class StructField
  {
  public:
    std::string m_name;
    Type m_type;

    StructField(std::string name, Type type);
    StructField(std::string name, Type type, std::string type_name);
  };

  class StructDecl : public Statement
  {
  public:
    std::string m_name;
    std::vector<StructField> m_fields;

    StructDecl(Location loc, std::string name, std::vector<StructField> fields);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class StructInstantiation : public Expression
  {
  public:
    std::string m_struct_name;
    std::vector<ExprPtr> m_field_values;

    StructInstantiation(Location loc, std::string name,
                        std::vector<ExprPtr> values);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class Array : public Expression
  {
  public:
    std::vector<ExprPtr> m_members;
    uint64_t m_size;

    Array(Location loc, std::vector<ExprPtr> members);
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

  class Program : public Node
  {
  public:
    std::vector<NodePtr> m_nodes;

    explicit Program(Location loc) : Node(loc) {}
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void write(std::ostream &os, const TySpecArena &arena, unsigned long indent = 0) const;
    void accept(ASTVisitor &visitor) override;
  };

  class Import : public Node
  {
  public:
    std::string m_path;

    explicit Import(Location loc, std::string path)
        : Node(loc), m_path(path) {}
    void write(std::ostream &os, unsigned long indent = 0) const override;
    void accept(ASTVisitor &visitor) override;
  };

} // namespace aloha

#endif // ALOHA_AST_H_
