#include "ast.h"
#include "type.h"
#include <ostream>
#include <string>

namespace Aloha {

void Number::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Number: " << value << std::endl;
}

void Boolean::write(std::ostream &os, unsigned long indent) const {
  auto value_str = value == true ? "True" : "False";
  os << std::string(indent, ' ') << "Boolean: " << value_str << std::endl;
}

void String::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "String: " << value << std::endl;
}

void ExpressionStatement::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Expression Statement:" << std::endl;
  expr->write(os, indent + 2);
}
void UnaryExpression::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Unary Expression: " << op << std::endl;
  expr->write(os, indent + 2);
}

void BinaryExpression::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Binary Expression: " << op << std::endl;
  left->write(os, indent + 2);
  right->write(os, indent + 2);
}

void Identifier::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Identifier: " << name << std::endl;
  auto str_type = AlohaType::to_string(type);
  os << std::string(indent, ' ') << "Type: " << str_type << std::endl;
}

void StructFieldAccess::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Field Access: " << m_struct_name << " -> "
     << m_field_name << std::endl;
}

void Declaration::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Declaration: " << variable_name
     << std::endl;
  if (type) {
    os << std::string(indent + 2, ' ')
       << "Type: " << AlohaType::to_string(type.value()) << std::endl;
  }
  std::string mutability = is_mutable ? "Yes" : "No";
  os << std::string(indent + 2, ' ') << "Mutable: " << mutability << std::endl;
  if (is_assigned) {
    expression->write(os, indent + 2);
  } else {
    os << std::string(indent + 2, ' ') << "Expression: Not assigned"
       << std::endl;
  }
}

void Assignment::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Assignment: " << variable_name
     << std::endl;
  os << std::string(indent + 2, ' ') << "Type: " << AlohaType::to_string(type)
     << std::endl;
  expression->write(os, indent + 2);
}
void FunctionCall::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Function Call: " << funcName->name
     << std::endl;
  for (const auto &arg : arguments) {
    arg->write(os, indent + 2);
  }
}

void ReturnStatement::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Return Statement:" << std::endl;
  expression->write(os, indent + 2);
}

void IfStatement::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "If Statement:" << std::endl;
  os << std::string(indent + 2, ' ') << "Condition:" << std::endl;
  condition->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "Then Branch:" << std::endl;
  for (const auto &stmt : then_branch->statements) {
    stmt->write(os, indent + 4);
  }
  if (!has_else_branch())
    return;
  os << std::string(indent + 2, ' ') << "Else Branch:" << std::endl;
  for (const auto &stmt : else_branch->statements) {
    stmt->write(os, indent + 4);
  }
}

void WhileLoop::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "While Loop:" << std::endl;
  os << std::string(indent + 2, ' ') << "Condition:" << std::endl;
  condition->write(os, indent + 4);
  os << std::string(indent + 2, ' ') << "Body:" << std::endl;
  for (const auto &stmt : body->statements) {
    stmt->write(os, indent + 4);
  }
}

void ForLoop::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "For Loop:" << std::endl;
  os << std::string(indent, ' ') << "Initializer:" << std::endl;
  initializer->write(os, indent + 2);
  os << std::string(indent, ' ') << "Condition:" << std::endl;
  condition->write(os, indent + 2);
  os << std::string(indent, ' ') << "Increment:" << std::endl;
  increment->write(os, indent + 2);
  os << std::string(indent, ' ') << "Body:" << std::endl;
  for (const auto &stmt : body) {
    stmt->write(os, indent + 2);
  }
}

void Function::write(std::ostream &os, unsigned long indent) const {
  os << std::endl;
  os << std::string(indent, ' ') << "Function: " << name->name << std::endl;
  os << std::string(indent, ' ') << "Parameters:" << std::endl;
  for (const auto &p : parameters) {
    os << std::string(indent + 2, ' ') << p.name << ": "
       << AlohaType::to_string(p.type) << std::endl;
  }
  os << std::string(indent, ' ')
     << "Return Type: " << AlohaType::to_string(return_type) << std::endl;
  os << std::string(indent, ' ') << "Body:" << std::endl;
  for (const auto &stmt : body.get()->statements) {
    stmt->write(os, indent + 2);
  }
}

void StatementList::write(std::ostream &os, unsigned long indent) const {
  for (const auto &statement : statements) {
    statement->write(os, indent + 2);
  }
}
void StructDecl::write(std::ostream &os, unsigned long indent) const {
  os << std::endl;
  os << std::string(indent, ' ') << "Struct : " << m_name << std::endl;
  os << std::string(indent, ' ') << "Fields:" << std::endl;
  for (const auto &field : m_fields) {
    os << std::string(indent + 4, ' ') << field.m_name << ": "
       << AlohaType::to_string(field.m_type) << std::endl;
  }
}

void StructInstantiation::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Struct Name: " << m_struct_name
     << std::endl;
  os << std::string(indent, ' ') << "Struct Fields:" << std::endl;
  for (const auto &field : m_field_values) {
    field->write(os, indent + 4);
  }
}

void Program::write(std::ostream &os, unsigned long indent) const {
  os << std::string(indent, ' ') << "Program:" << std::endl;
  for (const auto &node : nodes) {
    node->write(os, indent + 2);
  }
}

} // namespace Aloha
