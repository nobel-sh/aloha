#ifndef AST_H_
#define AST_H_

#include "ASTVisitor.h"
#include "type.h"
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace Aloha {

// Base Node
class Node {
public:
  virtual ~Node() = default;
  virtual void write(std::ostream &os, unsigned long indent = 0) const = 0;
  virtual void accept(ASTVisitor &visitor) = 0;
};

// Expressions
class Expression : public Node {
public:
  virtual AlohaType::Type get_type() const = 0;
};

class Statement : public Node {};

using ExprPtr = std::shared_ptr<Expression>;
using StmtPtr = std::shared_ptr<Statement>;

class StatementList : public Statement {
public:
  std::vector<StmtPtr> statements;

  StatementList(std::vector<StmtPtr> stmts) : statements(std::move(stmts)) {}
  StatementList() = default;

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }

  bool empty() const { return statements.empty(); }
};

class ExpressionStatement : public Statement {
public:
  ExprPtr expr;

  explicit ExpressionStatement(ExprPtr expr) : expr(std::move(expr)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class Number : public Expression {
public:
  std::string value;

  explicit Number(std::string val) : value(std::move(val)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return AlohaType::Type::NUMBER; }
};

class Boolean : public Expression {
public:
  bool value;

  explicit Boolean(bool val) : value(val) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return AlohaType::Type::BOOL; }
};

class String : public Expression {
public:
  std::string value;

  explicit String(std::string val) : value(std::move(val)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return AlohaType::Type::STRING; }
};

class UnaryExpression : public Expression {
public:
  std::string op;
  ExprPtr expr;

  UnaryExpression(std::string oper, ExprPtr expr)
      : op(std::move(oper)), expr(std::move(expr)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return expr->get_type(); }
};

class BinaryExpression : public Expression {
public:
  ExprPtr left;
  std::string op;
  ExprPtr right;

  BinaryExpression(ExprPtr lhs, std::string oper, ExprPtr rhs)
      : left(std::move(lhs)), op(std::move(oper)), right(std::move(rhs)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return left->get_type(); }
};

class Identifier : public Expression {
public:
  std::string name;
  AlohaType::Type type;

  explicit Identifier(std::string name, AlohaType::Type t)
      : name(std::move(name)), type(t) {}
  explicit Identifier(std::string name)
      : name(std::move(name)), type(AlohaType::Type::UNKNOWN) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return type; }
};

class StructFieldAccess : public Expression {
public:
  ExprPtr m_struct_expr;
  std::string m_field_name;
  AlohaType::Type m_type;

  StructFieldAccess(ExprPtr t_struct_expr, std::string t_field_name)
      : m_struct_expr(std::move(t_struct_expr)),
        m_field_name(std::move(t_field_name)),
        m_type(AlohaType::Type::UNKNOWN) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return m_type; }
  void set_type(AlohaType::Type t_type) { m_type = t_type; }
};

class StructFieldAssignment : public Statement {
public:
  ExprPtr m_struct_expr;
  std::string m_field_name;
  ExprPtr m_value;
  AlohaType::Type m_type;

  StructFieldAssignment(ExprPtr t_struct_expr, std::string t_field_name,
                        ExprPtr t_value)
      : m_struct_expr(std::move(t_struct_expr)),
        m_field_name(std::move(t_field_name)), m_value(std::move(t_value)),
        m_type(AlohaType::Type::UNKNOWN) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const { return m_type; }
};

// Statements
class Declaration : public Statement {
public:
  std::string variable_name;
  std::optional<AlohaType::Type> type;
  ExprPtr expression;
  bool is_assigned;
  bool is_mutable;

  Declaration(std::string var_name, std::optional<AlohaType::Type> type,
              ExprPtr expr, bool is_mutable)
      : variable_name(std::move(var_name)), type(type),
        expression(std::move(expr)), is_assigned(expression != nullptr),
        is_mutable(is_mutable) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class Assignment : public Statement {
public:
  std::string variable_name;
  ExprPtr expression;
  AlohaType::Type type;

  Assignment(std::string var_name, ExprPtr expr)
      : variable_name(std::move(var_name)), expression(std::move(expr)),
        type(AlohaType::Type::UNKNOWN) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class FunctionCall : public Expression {
public:
  std::shared_ptr<Identifier> funcName;
  std::vector<ExprPtr> arguments;
  AlohaType::Type type;

  FunctionCall(std::shared_ptr<Identifier> func_name, std::vector<ExprPtr> args)
      : funcName(std::move(func_name)), arguments(std::move(args)),
        type(AlohaType::Type::UNKNOWN) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return type; }
};

class ReturnStatement : public Statement {
public:
  ExprPtr expression;

  explicit ReturnStatement(ExprPtr expr) : expression(std::move(expr)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class IfStatement : public Statement {
public:
  ExprPtr condition;
  std::shared_ptr<StatementList> then_branch;
  std::shared_ptr<StatementList> else_branch;

  IfStatement(ExprPtr cond, std::shared_ptr<StatementList> then_branch,
              std::shared_ptr<StatementList> elseBr)
      : condition(std::move(cond)), then_branch(std::move(then_branch)),
        else_branch(std::move(elseBr)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  bool has_else_branch() const { return else_branch != nullptr; }
};

class WhileLoop : public Statement {
public:
  ExprPtr condition;
  std::shared_ptr<StatementList> body;

  WhileLoop(ExprPtr cond, std::shared_ptr<StatementList> b)
      : condition(std::move(cond)), body(std::move(b)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class ForLoop : public Statement {
public:
  std::shared_ptr<Declaration> initializer;
  ExprPtr condition;
  std::shared_ptr<Declaration> increment;
  std::vector<StmtPtr> body;

  ForLoop(std::shared_ptr<Declaration> init, ExprPtr cond,
          std::shared_ptr<Declaration> inc, std::vector<StmtPtr> b)
      : initializer(std::move(init)), condition(std::move(cond)),
        increment(std::move(inc)), body(std::move(b)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

// Function
class Parameter {
public:
  std::string name;
  AlohaType::Type type;

  Parameter(std::string ident, AlohaType::Type type)
      : name(std::move(ident)), type(type) {}
};

class Function : public Statement {
public:
  std::shared_ptr<Identifier> name;
  std::vector<Parameter> parameters;
  AlohaType::Type return_type;
  std::shared_ptr<StatementList> body;

  Function(std::shared_ptr<Identifier> func_name, std::vector<Parameter> params,
           AlohaType::Type return_type, std::shared_ptr<StatementList> b)
      : name(std::move(func_name)), parameters(std::move(params)),
        return_type(return_type), body(std::move(b)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class StructField {
public:
  std::string m_name;
  AlohaType::Type m_type;

  StructField(std::string t_name, AlohaType::Type t_type)
      : m_name(std::move(t_name)), m_type(t_type) {}
};

class StructDecl : public Statement {
public:
  // std::shared_ptr<Identifier> m_ident;
  std::string m_name;
  std::vector<StructField> m_fields;

  StructDecl(std::string t_name, std::vector<StructField> t_fields)
      : m_name(std::move(t_name)), m_fields(std::move(t_fields)) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class StructInstantiation : public Expression {
public:
  std::string m_struct_name;
  std::vector<ExprPtr> m_field_values;
  AlohaType::Type m_type;

  StructInstantiation(std::string t_name, std::vector<ExprPtr> t_values)
      : m_struct_name(t_name), m_field_values(std::move(t_values)),
        m_type(AlohaType::Type::UNKNOWN) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }

  AlohaType::Type get_type() const override { return m_type; }

  void set_type(AlohaType::Type t_type) { m_type = t_type; }
};

// Program
class Program : public Node {
public:
  std::vector<std::shared_ptr<Node>> nodes;

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

} // namespace Aloha

#endif // AST_H_
