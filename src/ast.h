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
  virtual void write(std::ostream &os, int indent = 0) const = 0;
  virtual void accept(ASTVisitor &visitor) = 0;
};

// Expressions
class Expression : public Node {
public:
  virtual AlohaType::Type getType() const = 0;
};

class Statement : public Node {};

using ExprPtr = std::shared_ptr<Expression>;
using StmtPtr = std::shared_ptr<Statement>;

class StatementList : public Node {
public:
  std::vector<std::shared_ptr<Statement>> statements;
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  explicit StatementList(const std::vector<StmtPtr> stmts)
      : statements(std::move(stmts)) {}
  bool empty() const { return statements.size() == 0; }
};

class ExpressionStatement : public Statement {
public:
  ExprPtr expr;
  explicit ExpressionStatement(ExprPtr expr) : expr(std::move(expr)) {}
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class Number : public Expression {
public:
  std::string value;
  explicit Number(const std::string &val) : value(val) {}
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type getType() const override { return AlohaType::Type::NUMBER; }
};

class Boolean : public Expression {
public:
  bool value;
  explicit Boolean(const bool &val) : value(val) {}
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type getType() const override { return AlohaType::Type::BOOL; }
};
class UnaryExpression : public Expression {
public:
  std::string op;
  ExprPtr expr;
  AlohaType::Type type;

  UnaryExpression(const std::string &oper, ExprPtr expr)
      : op(oper), expr(std::move(expr)), type(AlohaType::Type::UNKNOWN) {}

  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type getType() const override { return expr->getType(); }
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

  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type getType() const override { return left->getType(); }
};

class Identifier : public Expression {
public:
  std::string name;
  AlohaType::Type type;
  explicit Identifier(const std::string &name, AlohaType::Type t)
      : name(name), type(t) {}
  explicit Identifier(const std::string &name)
      : name(name), type(AlohaType::Type::UNKNOWN) {}
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type getType() const override { return type; }
};

// Statements
class Declaration : public Statement {
public:
  std::string variableName;
  std::optional<AlohaType::Type> type;
  ExprPtr expression;
  explicit Declaration(const std::string &varName,
                       std::optional<AlohaType::Type> type, ExprPtr expr)
      : variableName(varName), type(type), expression(std::move(expr)) {}
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class FunctionCall : public Expression {
public:
  std::shared_ptr<Identifier> funcName;
  std::vector<ExprPtr> arguments;
  AlohaType::Type type;
  FunctionCall(std::shared_ptr<Identifier> &funcName, std::vector<ExprPtr> args)
      : funcName(funcName), arguments(std::move(args)) {}
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  AlohaType::Type getType() const override { return type; }
};

class ReturnStatement : public Statement {
public:
  ExprPtr expression;
  explicit ReturnStatement(ExprPtr expr) : expression(std::move(expr)) {}
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

class IfStatement : public Statement {
public:
  ExprPtr condition;
  std::shared_ptr<StatementList> thenBranch;
  std::shared_ptr<StatementList> elseBranch;
  IfStatement(ExprPtr cond, std::shared_ptr<StatementList> thenBr,
              std::shared_ptr<StatementList> elseBr)
      : condition(std::move(cond)), thenBranch(std::move(thenBr)),
        elseBranch(std::move(elseBr)) {}
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
  bool has_else_branch() const { return elseBranch != nullptr; }
};

class WhileLoop : public Statement {
public:
  ExprPtr condition;
  std::shared_ptr<StatementList> body;
  WhileLoop(ExprPtr cond, std::shared_ptr<StatementList> b)
      : condition(std::move(cond)), body(std::move(b)) {}
  void write(std::ostream &os, int indent = 0) const override;
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
  void write(std::ostream &os, int indent = 0) const override;
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
  AlohaType::Type returnType;
  std::shared_ptr<StatementList> body;
  Function(const std::shared_ptr<Identifier> &funcName,
           std::vector<Parameter> params, AlohaType::Type retType,
           std::shared_ptr<StatementList> b)
      : name(funcName), parameters(std::move(params)), returnType(retType),
        body(std::move(b)) {}
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

// Program
class Program : public Node {
public:
  std::vector<std::shared_ptr<Node>> nodes;
  void write(std::ostream &os, int indent = 0) const override;
  void accept(ASTVisitor &visitor) override { visitor.visit(this); }
};

#endif // AST_H_
