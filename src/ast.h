#ifndef ALOHA_AST_H_
#define ALOHA_AST_H_

#include "ASTVisitor.h"
#include "type.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace aloha {

using Type = AlohaType::Type;

class Node {
public:
  virtual ~Node() = default;
  virtual void write(std::ostream &os, unsigned long indent = 0) const = 0;
  virtual void accept(ASTVisitor &visitor) = 0;
};

class Expression : public Node {
public:
  virtual Type get_type() const = 0;
};

class Statement : public Node {};

using NodePtr = std::unique_ptr<Node>;
using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;

class StatementBlock : public Statement {
public:
  std::vector<StmtPtr> m_statements;

  explicit StatementBlock(std::vector<StmtPtr> stmts = {});
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  bool is_empty() const;
};

class ExpressionStatement : public Statement {
public:
  ExprPtr m_expr;

  explicit ExpressionStatement(ExprPtr expr);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
};

class Number : public Expression {
public:
  std::string m_value;

  explicit Number(std::string val);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const override;
};

class Boolean : public Expression {
public:
  bool m_value;

  explicit Boolean(bool val);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const override;
};

class String : public Expression {
public:
  std::string m_value;

  explicit String(std::string val);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const override;
};

class UnaryExpression : public Expression {
public:
  std::string m_op;
  ExprPtr m_expr;

  UnaryExpression(std::string oper, ExprPtr expr);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const override;
};

class BinaryExpression : public Expression {
public:
  ExprPtr m_left;
  std::string m_op;
  ExprPtr m_right;

  BinaryExpression(ExprPtr lhs, std::string oper, ExprPtr rhs);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const override;
};

class Identifier : public Expression {
public:
  std::string m_name;
  Type m_type;

  explicit Identifier(std::string name, Type t = Type::UNKNOWN);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const override;
};

class StructFieldAccess : public Expression {
public:
  ExprPtr m_struct_expr;
  std::string m_field_name;
  Type m_type;

  StructFieldAccess(ExprPtr struct_expr, std::string field_name);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const override;
  void set_type(Type type);
};

class StructFieldAssignment : public Statement {
public:
  ExprPtr m_struct_expr;
  std::string m_field_name;
  ExprPtr m_value;
  Type m_type;

  StructFieldAssignment(ExprPtr struct_expr, std::string field_name,
                        ExprPtr value);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const;
};

class Declaration : public Statement {
public:
  std::string m_variable_name;
  std::optional<Type> m_type;
  ExprPtr m_expression;
  bool m_is_assigned;
  bool m_is_mutable;

  Declaration(std::string var_name, std::optional<Type> type, ExprPtr expr,
              bool is_mutable);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
};

class Assignment : public Statement {
public:
  std::string m_variable_name;
  ExprPtr m_expression;
  Type m_type;

  Assignment(std::string var_name, ExprPtr expr);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
};

class FunctionCall : public Expression {
public:
  std::unique_ptr<Identifier> m_func_name;
  std::vector<ExprPtr> m_arguments;
  Type m_type;

  FunctionCall(std::unique_ptr<Identifier> func_name,
               std::vector<ExprPtr> args);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const override;
};

class ReturnStatement : public Statement {
public:
  ExprPtr m_expression;

  explicit ReturnStatement(ExprPtr expr);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
};

class IfStatement : public Statement {
public:
  ExprPtr m_condition;
  std::unique_ptr<StatementBlock> m_then_branch;
  std::unique_ptr<StatementBlock> m_else_branch;

  IfStatement(ExprPtr cond, std::unique_ptr<StatementBlock> then_branch,
              std::unique_ptr<StatementBlock> else_branch);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  bool has_else_branch() const;
};

class WhileLoop : public Statement {
public:
  ExprPtr m_condition;
  std::unique_ptr<StatementBlock> m_body;

  WhileLoop(ExprPtr cond, std::unique_ptr<StatementBlock> body);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
};

class ForLoop : public Statement {
public:
  std::unique_ptr<Declaration> m_initializer;
  ExprPtr m_condition;
  std::unique_ptr<Declaration> m_increment;
  std::vector<StmtPtr> m_body;

  ForLoop(std::unique_ptr<Declaration> init, ExprPtr cond,
          std::unique_ptr<Declaration> inc, std::vector<StmtPtr> body);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
};

class Parameter {
public:
  std::string m_name;
  Type m_type;

  Parameter(std::string name, Type type);
};

class Function : public Statement {
public:
  std::unique_ptr<Identifier> m_name;
  std::vector<Parameter> m_parameters;
  Type m_return_type;
  std::unique_ptr<StatementBlock> m_body;

  Function(std::unique_ptr<Identifier> func_name, std::vector<Parameter> params,
           Type return_type, std::unique_ptr<StatementBlock> body);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
};

class StructField {
public:
  std::string m_name;
  Type m_type;

  StructField(std::string name, Type type);
};

class StructDecl : public Statement {
public:
  std::string m_name;
  std::vector<StructField> m_fields;

  StructDecl(std::string name, std::vector<StructField> fields);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
};

class StructInstantiation : public Expression {
public:
  std::string m_struct_name;
  std::vector<ExprPtr> m_field_values;
  Type m_type;

  StructInstantiation(std::string name, std::vector<ExprPtr> values);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
  Type get_type() const override;
  void set_type(Type type);
};

class Array : public Expression {
public:
  std::vector<ExprPtr> m_members;
  Type m_type;
  uint64_t m_size;

  Array(std::vector<ExprPtr> members);
  void write(std::ostream &os, unsigned long indent = 0) const override;
  Type get_type() const override;
  void set_type(Type type);
  void accept(ASTVisitor &visitor) override;
};

class Program : public Node {
public:
  std::vector<NodePtr> m_nodes;

  void write(std::ostream &os, unsigned long indent = 0) const override;
  void accept(ASTVisitor &visitor) override;
};

} // namespace aloha

#endif // ALOHA_AST_H_
