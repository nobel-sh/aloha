#ifndef AST_H_
#define AST_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

// Base Node
class Node {
public:
  virtual ~Node() = default;
  virtual void write(std::ostream &os, int indent = 0) const = 0;
};

// Expressions
class Expression : public Node {};
class Statement : public Node {};

using ExprPtr = std::shared_ptr<Expression>;
using StmtPtr = std::shared_ptr<Statement>;

class StatementList : public Node {
public:
  std::vector<std::shared_ptr<Statement>> statements;
  void write(std::ostream &os, int indent = 0) const override;
  explicit StatementList(const std::vector<StmtPtr> stmts)
      : statements(std::move(stmts)) {}
  bool empty() const { return statements.size() == 0; }
};

class Number : public Expression {
public:
  std::string value;
  explicit Number(const std::string &val) : value(val) {}
  void write(std::ostream &os, int indent = 0) const override;
};

class UnaryExpression : public Expression {
public:
  std::string op;
  ExprPtr expr;

  UnaryExpression(const std::string &oper, ExprPtr expr)
      : op(oper), expr(std::move(expr)) {}

  void write(std::ostream &os, int indent = 0) const override;
};

class BinaryExpression : public Expression {
public:
  ExprPtr left;
  std::string op;
  ExprPtr right;

  BinaryExpression(ExprPtr lhs, const std::string &oper, ExprPtr rhs)
      : left(std::move(lhs)), op(oper), right(std::move(rhs)) {}

  void write(std::ostream &os, int indent = 0) const override;
};

class Identifier : public Expression {
public:
  std::string name;
  explicit Identifier(const std::string &name) : name(name) {}
  void write(std::ostream &os, int indent = 0) const override;
};

// Statements
class Assignment : public Statement {
public:
  std::string variableName;
  ExprPtr expression;
  Assignment(const std::string &varName, ExprPtr expr)
      : variableName(varName), expression(std::move(expr)) {}
  void write(std::ostream &os, int indent = 0) const override;
};

class FunctionCall : public Expression {
public:
  std::string functionName;
  std::vector<ExprPtr> arguments;
  FunctionCall(const std::string &funcName, std::vector<ExprPtr> args)
      : functionName(funcName), arguments(std::move(args)) {}
  void write(std::ostream &os, int indent = 0) const override;
};

class ReturnStatement : public Statement {
public:
  ExprPtr expression;
  explicit ReturnStatement(ExprPtr expr) : expression(std::move(expr)) {}
  void write(std::ostream &os, int indent = 0) const override;
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
};

class WhileLoop : public Statement {
public:
  ExprPtr condition;
  std::vector<StmtPtr> body;
  WhileLoop(ExprPtr cond, std::vector<StmtPtr> b)
      : condition(std::move(cond)), body(std::move(b)) {}
  void write(std::ostream &os, int indent = 0) const override;
};

class ForLoop : public Statement {
public:
  std::shared_ptr<Assignment> initializer;
  ExprPtr condition;
  std::shared_ptr<Assignment> increment;
  std::vector<StmtPtr> body;
  ForLoop(std::shared_ptr<Assignment> init, ExprPtr cond,
          std::shared_ptr<Assignment> inc, std::vector<StmtPtr> b)
      : initializer(std::move(init)), condition(std::move(cond)),
        increment(std::move(inc)), body(std::move(b)) {}
  void write(std::ostream &os, int indent = 0) const override;
};

// Function
class Parameter {
public:
  std::pair<std::string, std::string> param;
  Parameter(std::pair<std::string, std::string> &p) : param(std::move(p)) {}
};

class Function : public Statement {
public:
  std::shared_ptr<Identifier> name;
  std::vector<Parameter> parameters;
  std::string returnType;
  std::shared_ptr<StatementList> body;
  Function(const std::shared_ptr<Identifier> &funcName,
           std::vector<Parameter> params, const std::string &retType,
           std::shared_ptr<StatementList> b)

      : name(funcName), returnType(retType), parameters(std::move(params)),
        body(std::move(b)) {}
  void write(std::ostream &os, int indent = 0) const override;
};

// Program
class Program : public Node {
public:
  std::vector<std::shared_ptr<Node>> nodes;
  void write(std::ostream &os, int indent = 0) const override;
};

#endif // AST_H_
