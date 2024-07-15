#include "ast.h"
#include "ASTVisitor.h"
#include <iostream>
#include <string>

namespace aloha {

void StatementBlock::accept(ASTVisitor &visitor) { visitor.visit(this); }
void ExpressionStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
void Number::accept(ASTVisitor &visitor) { visitor.visit(this); }
void Boolean::accept(ASTVisitor &visitor) { visitor.visit(this); }
void String::accept(ASTVisitor &visitor) { visitor.visit(this); }
void UnaryExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }
void BinaryExpression::accept(ASTVisitor &visitor) { visitor.visit(this); }
void Identifier::accept(ASTVisitor &visitor) { visitor.visit(this); }
void StructFieldAccess::accept(ASTVisitor &visitor) { visitor.visit(this); }
void StructFieldAssignment::accept(ASTVisitor &visitor) { visitor.visit(this); }
void Declaration::accept(ASTVisitor &visitor) { visitor.visit(this); }
void Assignment::accept(ASTVisitor &visitor) { visitor.visit(this); }
void FunctionCall::accept(ASTVisitor &visitor) { visitor.visit(this); }
void ReturnStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
void IfStatement::accept(ASTVisitor &visitor) { visitor.visit(this); }
void WhileLoop::accept(ASTVisitor &visitor) { visitor.visit(this); }
void ForLoop::accept(ASTVisitor &visitor) { visitor.visit(this); }
void Function::accept(ASTVisitor &visitor) { visitor.visit(this); }
void StructDecl::accept(ASTVisitor &visitor) { visitor.visit(this); }
void StructInstantiation::accept(ASTVisitor &visitor) { visitor.visit(this); }
void Array::accept(ASTVisitor &visitor) { visitor.visit(this); }
void Program::accept(ASTVisitor &visitor) { visitor.visit(this); }

StatementBlock::StatementBlock(std::vector<StmtPtr> stmts)
    : m_statements(std::move(stmts)) {}

ExpressionStatement::ExpressionStatement(ExprPtr expr)
    : m_expr(std::move(expr)) {}

Number::Number(std::string val) : m_value(std::move(val)) {}
Type Number::get_type() const { return Type::NUMBER; }

Boolean::Boolean(bool val) : m_value(val) {}
Type Boolean::get_type() const { return Type::BOOL; }

String::String(std::string val) : m_value(std::move(val)) {}
Type String::get_type() const { return Type::STRING; }

UnaryExpression::UnaryExpression(std::string oper, ExprPtr expr)
    : m_op(std::move(oper)), m_expr(std::move(expr)) {}
Type UnaryExpression::get_type() const { return m_expr->get_type(); }

BinaryExpression::BinaryExpression(ExprPtr lhs, std::string oper, ExprPtr rhs)
    : m_left(std::move(lhs)), m_op(std::move(oper)), m_right(std::move(rhs)) {}
Type BinaryExpression::get_type() const { return m_left->get_type(); }

Identifier::Identifier(std::string name, Type t)
    : m_name(std::move(name)), m_type(t) {}
Type Identifier::get_type() const { return m_type; }

StructFieldAccess::StructFieldAccess(ExprPtr struct_expr,
                                     std::string field_name)
    : m_struct_expr(std::move(struct_expr)),
      m_field_name(std::move(field_name)), m_type(Type::UNKNOWN) {}
Type StructFieldAccess::get_type() const { return m_type; }
void StructFieldAccess::set_type(Type type) { m_type = type; }

StructFieldAssignment::StructFieldAssignment(ExprPtr struct_expr,
                                             std::string field_name,
                                             ExprPtr value)
    : m_struct_expr(std::move(struct_expr)),
      m_field_name(std::move(field_name)), m_value(std::move(value)),
      m_type(Type::UNKNOWN) {}
Type StructFieldAssignment::get_type() const { return m_type; }

Declaration::Declaration(std::string var_name, std::optional<Type> type,
                         ExprPtr expr, bool is_mutable)
    : m_variable_name(std::move(var_name)), m_type(type),
      m_expression(std::move(expr)), m_is_assigned(m_expression != nullptr),
      m_is_mutable(is_mutable) {}

Assignment::Assignment(std::string var_name, ExprPtr expr)
    : m_variable_name(std::move(var_name)), m_expression(std::move(expr)),
      m_type(Type::UNKNOWN) {}

FunctionCall::FunctionCall(std::unique_ptr<Identifier> func_name,
                           std::vector<ExprPtr> args)
    : m_func_name(std::move(func_name)), m_arguments(std::move(args)),
      m_type(Type::UNKNOWN) {}
Type FunctionCall::get_type() const { return m_type; }

ReturnStatement::ReturnStatement(ExprPtr expr)
    : m_expression(std::move(expr)) {}

IfStatement::IfStatement(ExprPtr cond,
                         std::unique_ptr<StatementBlock> then_branch,
                         std::unique_ptr<StatementBlock> else_branch)
    : m_condition(std::move(cond)), m_then_branch(std::move(then_branch)),
      m_else_branch(std::move(else_branch)) {}
bool IfStatement::has_else_branch() const { return m_else_branch != nullptr; }

WhileLoop::WhileLoop(ExprPtr cond, std::unique_ptr<StatementBlock> body)
    : m_condition(std::move(cond)), m_body(std::move(body)) {}

ForLoop::ForLoop(std::unique_ptr<Declaration> init, ExprPtr cond,
                 std::unique_ptr<Declaration> inc, std::vector<StmtPtr> body)
    : m_initializer(std::move(init)), m_condition(std::move(cond)),
      m_increment(std::move(inc)), m_body(std::move(body)) {}

Parameter::Parameter(std::string name, Type type)
    : m_name(std::move(name)), m_type(type) {}

Function::Function(std::unique_ptr<Identifier> func_name,
                   std::vector<Parameter> params, Type return_type,
                   std::unique_ptr<StatementBlock> body)
    : m_name(std::move(func_name)), m_parameters(std::move(params)),
      m_return_type(return_type), m_body(std::move(body)) {}

StructField::StructField(std::string name, Type type)
    : m_name(std::move(name)), m_type(type) {}

StructDecl::StructDecl(std::string name, std::vector<StructField> fields)
    : m_name(std::move(name)), m_fields(std::move(fields)) {}

StructInstantiation::StructInstantiation(std::string name,
                                         std::vector<ExprPtr> values)
    : m_struct_name(std::move(name)), m_field_values(std::move(values)),
      m_type(Type::UNKNOWN) {}
Type StructInstantiation::get_type() const { return m_type; }
void StructInstantiation::set_type(Type type) { m_type = type; }

Array::Array(std::vector<ExprPtr> members)
    : m_members(std::move(members)), m_type(members[0]->get_type()),
      m_size(members.size()) {}
Type Array::get_type() const { return m_type; }
void Array::set_type(Type t) { m_type = t; }

void StatementBlock::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "StatementBlock:{\n";
  for (const auto &stmt : m_statements) {
    stmt->write(os, indent + 2);
  }
  os << std::string(indent, ' ') << "}\n";
}

void ExpressionStatement::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "ExpressionStatement:{\n";
  m_expr->write(os, indent + 2);
  os << std::string(indent, ' ') << "}\n";
}

void Number::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Number: " << m_value << "\n";
}

void Boolean::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Boolean: " << (m_value ? "true" : "false")
     << "\n";
}

void String::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "String: \"" << m_value << "\"\n";
}

void UnaryExpression::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "UnaryExpression:{\n";
  os << std::string(indent + 2, ' ') << "Operator: " << m_op << "\n";
  os << std::string(indent + 2, ' ') << "Operand:{\n";
  m_expr->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent, ' ') << "}\n";
}

void BinaryExpression::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "BinaryExpression:{\n";
  os << std::string(indent + 2, ' ') << "Left:{\n";
  m_left->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent + 2, ' ') << "Operator: " << m_op << "\n";
  os << std::string(indent + 2, ' ') << "Right:{\n";
  m_right->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent, ' ') << "}\n";
}

void Identifier::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Identifier: " << m_name << "\n";
}

void StructFieldAccess::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "StructFieldAccess:{\n";
  os << std::string(indent + 2, ' ') << "Struct:{\n";
  m_struct_expr->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent + 2, ' ') << "Field: " << m_field_name << "\n";
  os << std::string(indent, ' ') << "}\n";
}

void StructFieldAssignment::write(std::ostream &os,
                                  unsigned long indent) const {
  os << std::string(indent, ' ') << "StructFieldAssignment:{\n";
  os << std::string(indent + 2, ' ') << "Struct:{\n";
  m_struct_expr->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent + 2, ' ') << "Field: " << m_field_name << "\n";
  os << std::string(indent + 2, ' ') << "Value:{\n";
  m_value->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent, ' ') << "}\n";
}

void Declaration::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Declaration:{\n";
  os << std::string(indent + 2, ' ') << "Name: " << m_variable_name << "\n";
  os << std::string(indent + 2, ' ')
     << "Type: " << (m_type ? AlohaType::to_string(*m_type) : "Inferred")
     << "\n";
  os << std::string(indent + 2, ' ')
     << "Mutable: " << (m_is_mutable ? "true" : "false") << "\n";
  if (m_expression) {
    os << std::string(indent + 2, ' ') << "Value:{\n";
    m_expression->write(os, indent + 4);
    os << std::string(indent + 2, ' ') << "}\n";
  }
  os << std::string(indent, ' ') << "}\n";
}

void Assignment::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Assignment:{\n";
  os << std::string(indent + 2, ' ') << "Variable: " << m_variable_name << "\n";
  os << std::string(indent + 2, ' ') << "Value:{\n";
  m_expression->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent, ' ') << "}\n";
}

void FunctionCall::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "FunctionCall:{\n";
  os << std::string(indent + 2, ' ') << "Name: ";
  m_func_name->write(os, 0);
  os << std::string(indent + 2, ' ') << "Arguments:[\n";
  for (const auto &arg : m_arguments) {
    arg->write(os, indent + 4);
  }
  os << std::string(indent + 2, ' ') << "]\n";
  os << std::string(indent, ' ') << "}\n";
}

void ReturnStatement::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "ReturnStatement:{\n";
  if (m_expression) {
    m_expression->write(os, indent + 2);
  }
  os << std::string(indent, ' ') << "}\n";
}

void IfStatement::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "IfStatement:{\n";
  os << std::string(indent + 2, ' ') << "Condition:{\n";
  m_condition->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent + 2, ' ') << "ThenBranch:{\n";
  m_then_branch->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  if (m_else_branch) {
    os << std::string(indent + 2, ' ') << "ElseBranch:{\n";
    m_else_branch->write(os, indent + 4);
    os << std::string(indent + 2, ' ') << "}\n";
  }
  os << std::string(indent, ' ') << "}\n";
}

void WhileLoop::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "WhileLoop:{\n";
  os << std::string(indent + 2, ' ') << "Condition:{\n";
  m_condition->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent + 2, ' ') << "Body:{\n";
  m_body->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent, ' ') << "}\n";
}

void ForLoop::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "ForLoop:{\n";
  os << std::string(indent + 2, ' ') << "Initializer:{\n";
  m_initializer->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent + 2, ' ') << "Condition:{\n";
  m_condition->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent + 2, ' ') << "Increment:{\n";
  m_increment->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent + 2, ' ') << "Body:{\n";
  for (const auto &stmt : m_body) {
    stmt->write(os, indent + 4);
  }
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent, ' ') << "}\n";
}

void Function::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Function:{\n";
  os << std::string(indent + 2, ' ') << "Name: ";
  m_name->write(os, 0);
  os << std::string(indent + 2, ' ') << "Parameters:[\n";
  for (const auto &param : m_parameters) {
    os << std::string(indent + 4, ' ') << param.m_name << ": "
       << AlohaType::to_string(param.m_type) << "\n";
  }
  os << std::string(indent + 2, ' ') << "]\n";
  os << std::string(indent + 2, ' ')
     << "ReturnType: " << AlohaType::to_string(m_return_type) << "\n";
  os << std::string(indent + 2, ' ') << "Body:{\n";
  m_body->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "}\n";
  os << std::string(indent, ' ') << "}\n";
}

void StructDecl::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "StructDecl:{\n";
  os << std::string(indent + 2, ' ') << "Name: " << m_name << "\n";
  os << std::string(indent + 2, ' ') << "Fields:[\n";
  for (const auto &field : m_fields) {
    os << std::string(indent + 4, ' ') << field.m_name << ": "
       << AlohaType::to_string(field.m_type) << "\n";
  }
  os << std::string(indent + 2, ' ') << "]\n";
  os << std::string(indent, ' ') << "}\n";
}

void StructInstantiation::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "StructInstantiation:{\n";
  os << std::string(indent + 2, ' ') << "Name: " << m_struct_name << "\n";
  os << std::string(indent + 2, ' ') << "Fields:[\n";
  for (const auto &field : m_field_values) {
    field->write(os, indent + 4);
  }
  os << std::string(indent + 2, ' ') << "]\n";
  os << std::string(indent, ' ') << "}\n";
}

void Array::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Array:{\n";
  os << std::string(indent + 2, ' ') << "Type: " << AlohaType::to_string(m_type)
     << "\n";
  os << std::string(indent + 2, ' ') << "Size: " << m_size << "\n";
  os << std::string(indent + 2, ' ') << "Elements:[\n";
  for (const auto &member : m_members) {
    member->write(os, indent + 4);
  }
  os << std::string(indent + 2, ' ') << "]\n";
  os << std::string(indent, ' ') << "}\n";
}

void Program::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Program:{\n";
  for (const auto &node : m_nodes) {
    node->write(os, indent + 2);
  }
  os << std::string(indent, ' ') << "}\n";
}

bool StatementBlock::is_empty() const { return m_statements.empty(); }

} // namespace aloha
