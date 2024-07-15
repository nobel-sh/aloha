#include "ast.h"
#include "ASTVisitor.h"
#include <algorithm>
#include <iostream>
#include <string>

namespace aloha {

// StatementList implementation
StatementList::StatementList(std::vector<StmtPtr> stmts)
    : m_statements(std::move(stmts)) {}

void StatementList::write(std::ostream &os, unsigned long indent) const {
  for (const auto &stmt : m_statements) {
    stmt->write(os, indent);
  }
}

void StatementList::accept(ASTVisitor &visitor) { visitor.visit(this); }

bool StatementList::is_empty() const { return m_statements.empty(); }

// ExpressionStatement implementation
ExpressionStatement::ExpressionStatement(ExprPtr expr)
    : m_expr(std::move(expr)) {}

void ExpressionStatement::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ');
  m_expr->write(os);
  os << ";\n";
}

void ExpressionStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }

// Number implementation
Number::Number(std::string val) : m_value(std::move(val)) {}

void Number::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << m_value;
}

void Number::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type Number::get_type() const { return Type::NUMBER; }

// Boolean implementation
Boolean::Boolean(bool val) : m_value(val) {}

void Boolean::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << (m_value ? "true" : "false");
}

void Boolean::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type Boolean::get_type() const { return Type::BOOL; }

// String implementation
String::String(std::string val) : m_value(std::move(val)) {}

void String::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "\"" << m_value << "\"";
}

void String::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type String::get_type() const { return Type::STRING; }

// UnaryExpression implementation
UnaryExpression::UnaryExpression(std::string oper, ExprPtr expr)
    : m_op(std::move(oper)), m_expr(std::move(expr)) {}

void UnaryExpression::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << m_op;
  m_expr->write(os);
}

void UnaryExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type UnaryExpression::get_type() const {
  // The type depends on the operator and operand type
  // This is a simplification and might need to be implemented based on specific
  // language rules
  return m_expr->get_type();
}

// BinaryExpression implementation
BinaryExpression::BinaryExpression(ExprPtr lhs, std::string oper, ExprPtr rhs)
    : m_left(std::move(lhs)), m_op(std::move(oper)), m_right(std::move(rhs)) {}

void BinaryExpression::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ');
  m_left->write(os);
  os << " " << m_op << " ";
  m_right->write(os);
}

void BinaryExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type BinaryExpression::get_type() const {
  // The type depends on the operator and operands' types
  // This is a simplification and might need to be implemented based on specific
  // language rules
  return m_left->get_type();
}

// Identifier implementation
Identifier::Identifier(std::string name, Type t)
    : m_name(std::move(name)), m_type(t) {}

void Identifier::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << m_name;
}

void Identifier::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type Identifier::get_type() const { return m_type; }

// StructFieldAccess implementation
StructFieldAccess::StructFieldAccess(ExprPtr struct_expr,
                                     std::string field_name)
    : m_struct_expr(std::move(struct_expr)),
      m_field_name(std::move(field_name)), m_type(Type::UNKNOWN) {}

void StructFieldAccess::write(std::ostream &os, unsigned long indent) const {
  m_struct_expr->write(os, indent);
  os << "." << m_field_name;
}

void StructFieldAccess::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type StructFieldAccess::get_type() const { return m_type; }

void StructFieldAccess::set_type(Type type) { m_type = type; }

// StructFieldAssignment implementation
StructFieldAssignment::StructFieldAssignment(ExprPtr struct_expr,
                                             std::string field_name,
                                             ExprPtr value)
    : m_struct_expr(std::move(struct_expr)),
      m_field_name(std::move(field_name)), m_value(std::move(value)),
      m_type(Type::UNKNOWN) {}

void StructFieldAssignment::write(std::ostream &os,
                                  unsigned long indent) const {
  os << std::string(indent, ' ');
  m_struct_expr->write(os);
  os << "." << m_field_name << " = ";
  m_value->write(os);
  os << ";\n";
}

void StructFieldAssignment::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type StructFieldAssignment::get_type() const { return m_type; }

// Declaration implementation
Declaration::Declaration(std::string var_name, std::optional<Type> type,
                         ExprPtr expr, bool is_mutable)
    : m_variable_name(std::move(var_name)), m_type(type),
      m_expression(std::move(expr)), m_is_assigned(m_expression != nullptr),
      m_is_mutable(is_mutable) {}

void Declaration::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ');
  os << (m_is_mutable ? "var " : "let ") << m_variable_name;
  if (m_type) {
    os << ": " << AlohaType::to_string(*m_type);
  }
  if (m_expression) {
    os << " = ";
    m_expression->write(os);
  }
  os << ";\n";
}

void Declaration::accept(ASTVisitor &visitor) { visitor.visit(this); }

// Assignment implementation
Assignment::Assignment(std::string var_name, ExprPtr expr)
    : m_variable_name(std::move(var_name)), m_expression(std::move(expr)),
      m_type(Type::UNKNOWN) {}

void Assignment::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << m_variable_name << " = ";
  m_expression->write(os);
  os << ";\n";
}

void Assignment::accept(ASTVisitor &visitor) { visitor.visit(this); }

// FunctionCall implementation
FunctionCall::FunctionCall(std::unique_ptr<Identifier> func_name,
                           std::vector<ExprPtr> args)
    : m_func_name(std::move(func_name)), m_arguments(std::move(args)),
      m_type(Type::UNKNOWN) {}

void FunctionCall::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ');
  m_func_name->write(os);
  os << "(";
  for (size_t i = 0; i < m_arguments.size(); ++i) {
    if (i > 0)
      os << ", ";
    m_arguments[i]->write(os);
  }
  os << ")";
}

void FunctionCall::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type FunctionCall::get_type() const { return m_type; }

// ReturnStatement implementation
ReturnStatement::ReturnStatement(ExprPtr expr)
    : m_expression(std::move(expr)) {}

void ReturnStatement::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "return ";
  if (m_expression) {
    m_expression->write(os);
  }
  os << ";\n";
}

void ReturnStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }

// IfStatement implementation
IfStatement::IfStatement(ExprPtr cond,
                         std::unique_ptr<StatementList> then_branch,
                         std::unique_ptr<StatementList> else_branch)
    : m_condition(std::move(cond)), m_then_branch(std::move(then_branch)),
      m_else_branch(std::move(else_branch)) {}

void IfStatement::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "if (";
  m_condition->write(os);
  os << ") {\n";
  m_then_branch->write(os, indent + 2);
  os << std::string(indent, ' ') << "}";
  if (m_else_branch) {
    os << " else {\n";
    m_else_branch->write(os, indent + 2);
    os << std::string(indent, ' ') << "}";
  }
  os << "\n";
}

void IfStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }

bool IfStatement::has_else_branch() const { return m_else_branch != nullptr; }

// WhileLoop implementation
WhileLoop::WhileLoop(ExprPtr cond, std::unique_ptr<StatementList> body)
    : m_condition(std::move(cond)), m_body(std::move(body)) {}

void WhileLoop::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "while (";
  m_condition->write(os);
  os << ") {\n";
  m_body->write(os, indent + 2);
  os << std::string(indent, ' ') << "}\n";
}

void WhileLoop::accept(ASTVisitor &visitor) { visitor.visit(this); }

// ForLoop implementation
ForLoop::ForLoop(std::unique_ptr<Declaration> init, ExprPtr cond,
                 std::unique_ptr<Declaration> inc, std::vector<StmtPtr> body)
    : m_initializer(std::move(init)), m_condition(std::move(cond)),
      m_increment(std::move(inc)), m_body(std::move(body)) {}

void ForLoop::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "for (";
  m_initializer->write(os);
  os << " ";
  m_condition->write(os);
  os << "; ";
  m_increment->write(os);
  os << ") {\n";
  for (const auto &stmt : m_body) {
    stmt->write(os, indent + 2);
  }
  os << std::string(indent, ' ') << "}\n";
}

void ForLoop::accept(ASTVisitor &visitor) { visitor.visit(this); }

// Parameter implementation
Parameter::Parameter(std::string name, Type type)
    : m_name(std::move(name)), m_type(type) {}

// Function implementation
Function::Function(std::unique_ptr<Identifier> func_name,
                   std::vector<Parameter> params, Type return_type,
                   std::unique_ptr<StatementList> body)
    : m_name(std::move(func_name)), m_parameters(std::move(params)),
      m_return_type(return_type), m_body(std::move(body)) {}

void Function::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "fn ";
  m_name->write(os);
  os << "(";
  for (size_t i = 0; i < m_parameters.size(); ++i) {
    if (i > 0)
      os << ", ";
    os << m_parameters[i].m_name << ": "
       << AlohaType::to_string(m_parameters[i].m_type);
  }
  os << ") -> " << AlohaType::to_string(m_return_type) << " {\n";
  m_body->write(os, indent + 2);
  os << std::string(indent, ' ') << "}\n";
}

void Function::accept(ASTVisitor &visitor) { visitor.visit(this); }

// StructField implementation
StructField::StructField(std::string name, Type type)
    : m_name(std::move(name)), m_type(type) {}

// StructDecl implementation
StructDecl::StructDecl(std::string name, std::vector<StructField> fields)
    : m_name(std::move(name)), m_fields(std::move(fields)) {}

void StructDecl::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "struct " << m_name << " {\n";
  for (const auto &field : m_fields) {
    os << std::string(indent + 2, ' ') << field.m_name << ": "
       << AlohaType::to_string(field.m_type) << ",\n";
  }
  os << std::string(indent, ' ') << "}\n";
}

void StructDecl::accept(ASTVisitor &visitor) { visitor.visit(this); }

// StructInstantiation implementation
StructInstantiation::StructInstantiation(std::string name,
                                         std::vector<ExprPtr> values)
    : m_struct_name(std::move(name)), m_field_values(std::move(values)),
      m_type(Type::UNKNOWN) {}

void StructInstantiation::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << m_struct_name << " { ";
  for (size_t i = 0; i < m_field_values.size(); ++i) {
    if (i > 0)
      os << ", ";
    m_field_values[i]->write(os);
  }
  os << " }";
}

void StructInstantiation::accept(ASTVisitor &visitor) { visitor.visit(this); }

Type StructInstantiation::get_type() const { return m_type; }

void StructInstantiation::set_type(Type type) { m_type = type; }

Array::Array(std::vector<ExprPtr> members)
    : m_members(std::move(members)), m_type(members[0]->get_type()),
      m_size(members.size()) {}

void Array::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "[]" << m_size << std::endl;
}

Type Array::get_type() const { return m_type; }
void Array::set_type(Type t) { m_type = t; }
void Array::accept(ASTVisitor &visitor) { visitor.visit(this); }

// Program implementation
void Program::write(std::ostream &os, unsigned long indent) const {
  for (const auto &node : m_nodes) {
    node->write(os, indent);
    os << "\n";
  }
}

void Program::accept(ASTVisitor &visitor) { visitor.visit(this); }

} // namespace aloha
