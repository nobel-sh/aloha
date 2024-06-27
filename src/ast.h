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

class StatementList : public Node {
public:
  std::vector<std::shared_ptr<Statement>> statements;
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  explicit StatementList(const std::vector<StmtPtr> stmts)
      : statements(std::move(stmts)) {}
  bool empty() const { return statements.size() == 0; }
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
  explicit Number(const std::string &val) : value(val) {}
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return AlohaType::Type::NUMBER; }
};

class Boolean : public Expression {
public:
  bool value;
  explicit Boolean(const bool &val) : value(val) {}
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return AlohaType::Type::BOOL; }
};

class AlohaString : public Expression {
public:
  std::string value;
  explicit AlohaString(const std::string &val) : value(std::move(val)) {}
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return AlohaType::Type::STRING; }
};

class UnaryExpression : public Expression {
public:
  std::string op;
  ExprPtr expr;
  AlohaType::Type type;

  UnaryExpression(const std::string &oper, ExprPtr expr)
      : op(oper), expr(std::move(expr)), type(AlohaType::Type::UNKNOWN) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return expr->get_type(); }
};

class BinaryExpression : public Expression {
public:
  ExprPtr left;
  std::string op;
  ExprPtr right;
  AlohaType::Type type;

  BinaryExpression(ExprPtr lhs, const std::string &oper, ExprPtr rhs)
      : left(std::move(lhs)), op(oper), right(std::move(rhs)),
        type(AlohaType::Type::UNKNOWN) {}

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return left->get_type(); }
};

class Identifier : public Expression {
public:
  std::string name;
  AlohaType::Type type;
  explicit Identifier(const std::string &name, AlohaType::Type t)
      : name(name), type(t) {}
  explicit Identifier(const std::string &name)
      : name(name), type(AlohaType::Type::UNKNOWN) {}
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type get_type() const override { return type; }
};

// Statements
class Declaration : public Statement {
public:
  std::string variable_name;
  std::optional<AlohaType::Type> type;
  ExprPtr expression;
  bool is_assigned;
  bool is_mutable;
  explicit Declaration(const std::string &var_name,
                       std::optional<AlohaType::Type> type, ExprPtr expr,
                       bool is_mutable)
      : variable_name(var_name), type(type), expression(std::move(expr)),
        is_assigned(expression != nullptr), is_mutable(is_mutable) {}
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class Assignment : public Statement {
public:
  std::string variable_name;
  AlohaType::Type type;
  ExprPtr expression;
  explicit Assignment(const std::string &var_name, ExprPtr expr)
      : variable_name(var_name), type(AlohaType::Type::UNKNOWN),
        expression(std::move(expr)) {}
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class FunctionCall : public Expression {
public:
  std::shared_ptr<Identifier> funcName;
  std::vector<ExprPtr> arguments;
  AlohaType::Type type;
  FunctionCall(std::shared_ptr<Identifier> &func_name,
               std::vector<ExprPtr> args)
      : funcName(func_name), arguments(std::move(args)) {}
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
      : name(ident), type(type) {}
};

class Function : public Statement {
public:
  std::shared_ptr<Identifier> name;
  std::vector<Parameter> parameters;
  AlohaType::Type return_type;
  std::shared_ptr<StatementList> body;
  Function(const std::shared_ptr<Identifier> &func_name,
           std::vector<Parameter> params, AlohaType::Type return_type,
           std::shared_ptr<StatementList> b)
      : name(func_name), parameters(std::move(params)),
        return_type(return_type), body(std::move(b)) {}
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

// Program
class Program : public Node {
public:
  std::vector<std::shared_ptr<Node>> nodes;
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

#endif // AST_H_
