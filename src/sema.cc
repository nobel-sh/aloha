#include "sema.h"
#include "ast/ast.h"
#include "type.h"
#include <memory>
#include <vector>

void SemanticAnalyzer::analyze(aloha::Program *program)
{
  program->accept(*this);
  if (!error.is_empty())
  {
    throw error;
  }
}

void SemanticAnalyzer::visit(aloha::Number *node) {}

void SemanticAnalyzer::visit(aloha::Boolean *node) {}

void SemanticAnalyzer::visit(aloha::String *node) {}

void SemanticAnalyzer::visit(aloha::Array *node)
{

  if (node->get_type() == AlohaType::Type::UNKNOWN)
  {
    if (!node->m_members.empty())
    {
      node->set_type(node->m_members[0]->get_type());
    }
  }
  AlohaType::Type arrayType = AlohaType::Type::UNKNOWN;
  for (size_t i = 0; i < node->m_members.size(); ++i)
  {
    node->m_members[i]->accept(*this);
    AlohaType::Type memberType = node->m_members[i]->get_type();

    if (i == 0)
    {
      arrayType = memberType;
    }
    else if (memberType != arrayType)
    {
      error.add_error("Array elements must have the same type");
      return;
    }
  }
  if (node->m_size != node->m_members.size())
  {
    error.add_error("Array size mismatch: declared size " +
                    std::to_string(node->m_size) +
                    " doesn't match number of elements " +
                    std::to_string(node->m_members.size()));
  }
}

void SemanticAnalyzer::visit(aloha::ExpressionStatement *node)
{
  node->m_expr->accept(*this);
}

void SemanticAnalyzer::visit(aloha::UnaryExpression *node)
{
  node->m_expr->accept(*this);
}

void SemanticAnalyzer::visit(aloha::BinaryExpression *node)
{
  node->m_left->accept(*this);
  node->m_right->accept(*this);

  if (node->m_left->get_type() != node->m_right->get_type())
  {
    error.add_error("Type mismatch in binary expression");
  }
}

void SemanticAnalyzer::visit(aloha::Identifier *node)
{
  VariableInfo *varInfo = symbol_table.get_variable(node->m_name);
  if (!varInfo)
  {
    error.add_error("Undeclared variable: " + node->m_name);
    return;
  }
  node->m_type = varInfo->type;
}

void SemanticAnalyzer::visit(aloha::Declaration *node)
{
  if (node->m_is_assigned)
  {
    node->m_expression->accept(*this);
    if (!node->m_type)
    {
      node->m_type = node->m_expression->get_type();
    }
    else if (node->m_type != node->m_expression->get_type())
    {
      if (AlohaType::is_struct_type(node->m_type.value()) &&
          AlohaType::is_struct_type(node->m_expression->get_type()))
      {

        if (node->m_type.value() != node->m_expression->get_type())
        {
          error.add_error("Struct type mismatch in declaration of " +
                          node->m_variable_name);
          return;
        }
      }
      else
      {
        if (node->m_expression->get_type() != AlohaType::Type::UNKNOWN)
        {
          error.add_error("Type mismatch in declaration of " +
                          node->m_variable_name);
          return;
        }
      }
    }
  }

  if (node->m_type && AlohaType::is_struct_type(node->m_type.value()))
  {
    if (auto *struct_inst = dynamic_cast<aloha::StructInstantiation *>(
            node->m_expression.get()))
    {
      if (!symbol_table.get_struct(struct_inst->m_struct_name))
      {
        error.add_error("Undeclared struct type: " +
                        AlohaType::to_string(node->m_type.value()));
        return;
      }
    }
    else
    {
      error.add_error("Expected struct instantiation for struct type");
      return;
    }
  }

  if (!symbol_table.add_variable(
          node->m_variable_name,
          node->m_type.value_or(AlohaType::Type::UNKNOWN), node->m_is_assigned,
          node->m_is_mutable))
  {
    error.add_error("Variable redeclaration: " + node->m_variable_name);
  }
}

void SemanticAnalyzer::visit(aloha::Assignment *node)
{
  VariableInfo *var_info = symbol_table.get_variable(node->m_variable_name);
  if (!var_info)
  {
    error.add_error("Cannnot assign to an undeclared variable: " +
                    node->m_variable_name);
    return;
  }
  if (!var_info->is_mutable && var_info->is_assigned)
  {
    error.add_error("Cannot mutate immutable assigned variable: " +
                    node->m_variable_name);
    return;
  }

  var_info->is_assigned = true;
  node->m_expression->accept(*this);

  if (var_info->type == AlohaType::Type::UNKNOWN)
  {
    var_info->type = node->m_expression->get_type();
    node->m_type = var_info->type;
  }
  else
  {
    node->m_type = var_info->type;
    if (node->m_type != node->m_expression->get_type())
    {
      if (AlohaType::is_struct_type(node->m_type) &&
          AlohaType::is_struct_type(node->m_expression->get_type()))
      {
        if (node->m_type != node->m_expression->get_type())
        {
          error.add_error("Struct type mismatch in assignment to " +
                          node->m_variable_name);
        }
      }
      else
      {
        std::string expr_type =
            AlohaType::to_string(node->m_expression->get_type());
        std::string var_type = AlohaType::to_string(node->m_type);
        error.add_error("Cannot assign expr of type " + expr_type +
                        " to var of type " + var_type);
      }
    }
  }
}

void SemanticAnalyzer::visit(aloha::FunctionCall *node)
{
  auto isBuiltin = symbol_table.is_builtin_function(node->m_func_name->m_name);
  if (isBuiltin)
  {
    return; // HACK: dont check anything for now but should be a good idea to
            // add protoypes and check for types and so on
  }

  FunctionInfo *funcInfo = symbol_table.get_function(node->m_func_name->m_name);
  if (!funcInfo)
  {
    error.add_error("Undeclared function: " + node->m_func_name->m_name);
    return;
  }

  // infer type from calle fn's return type
  node->m_type = funcInfo->return_type;

  if (node->m_arguments.size() != funcInfo->param_types.size())
  {
    error.add_error("Argument count mismatch in function call: " +
                    node->m_func_name->m_name);
    return;
  }
  for (size_t i = 0; i < node->m_arguments.size(); ++i)
  {
    node->m_arguments[i]->accept(*this);
    if (node->m_arguments[i]->get_type() != funcInfo->param_types[i])
    {
      error.add_error("Argument type mismatch in function call: " +
                      node->m_func_name->m_name);
    }
  }
}

void SemanticAnalyzer::visit(aloha::ReturnStatement *node)
{
  // Handle void returns (no expression)
  if (!node->m_expression)
  {
    if (current_fn && current_fn->m_return_type != AlohaType::Type::VOID)
    {
      error.add_error("Non-void function must return a value");
    }
    return;
  }

  node->m_expression->accept(*this);

  if (current_fn &&
      node->m_expression->get_type() != current_fn->m_return_type)
  {
    symbol_table.dump();
    std::cout << "Expr type: "
              << AlohaType::to_string(node->m_expression->get_type())
              << std::endl;
    std::cout << "Curr fn return type: "
              << AlohaType::to_string(current_fn->m_return_type) << std::endl;
    error.add_error("Return type mismatch in function: " +
                    current_fn->m_name->m_name);
  }
}

void SemanticAnalyzer::visit(aloha::IfStatement *node)
{
  node->m_condition->accept(*this);
  symbol_table.enter_scope();
  node->m_then_branch->accept(*this);
  symbol_table.leave_scope();
  if (node->has_else_branch())
  {
    symbol_table.enter_scope();
    node->m_else_branch->accept(*this);
    symbol_table.leave_scope();
  }
}

void SemanticAnalyzer::visit(aloha::WhileLoop *node)
{
  node->m_condition->accept(*this);
  symbol_table.enter_scope();
  node->m_body->accept(*this);
  symbol_table.leave_scope();
}

void SemanticAnalyzer::visit(aloha::ForLoop *node)
{
  symbol_table.enter_scope();
  node->m_initializer->accept(*this);
  node->m_condition->accept(*this);
  node->m_increment->accept(*this);
  for (auto &stmt : node->m_body)
  {
    stmt->accept(*this);
  }
  symbol_table.leave_scope();
}

void SemanticAnalyzer::visit(aloha::Function *node)
{
  // Special validation for main function
  if (node->m_name->m_name == "main")
  {
    if (node->m_return_type != AlohaType::Type::NUMBER)
    {
      error.add_error("main function must return numeric value");
    }
  }

  std::vector<AlohaType::Type> parameterType;
  for (const auto &param : node->m_parameters)
  {
    parameterType.push_back(param.m_type);
  }
  if (!symbol_table.add_function(node->m_name->m_name, node->m_return_type,
                                 parameterType))
  {
    error.add_error("Function redeclaration: " + node->m_name->m_name);
  }

  // For extern functions, we don't need to analyze the body
  if (node->m_is_extern)
  {
    return;
  }

  symbol_table.enter_scope();
  current_fn = node;

  for (const auto &param : node->m_parameters)
  {
    if (!symbol_table.add_variable(
            param.m_name, param.m_type, false,
            true))
    { // TODO: allow mutability to be defined in params
      error.add_error("Parameter redeclaration: " + param.m_name);
    }
  }
  node->m_body->accept(*this);
  current_fn = nullptr;
  symbol_table.leave_scope();
}

void SemanticAnalyzer::visit(aloha::StructDecl *node)
{
  std::vector<StructField> fields;
  for (const auto &field : node->m_fields)
  {
    fields.push_back({field.m_name, field.m_type});
  }

  AlohaType::Type structType = symbol_table.add_struct(node->m_name, fields);
  if (structType == AlohaType::Type::UNKNOWN)
  {
    error.add_error("Struct redeclaration: " + node->m_name);
  }
}

void SemanticAnalyzer::visit(aloha::StructInstantiation *node)
{
  StructInfo *structInfo = symbol_table.get_struct(node->m_struct_name);
  if (!structInfo)
  {
    error.add_error("Undeclared struct: " + node->m_struct_name);
    return;
  }

  if (node->m_field_values.size() != structInfo->fields.size())
  {
    error.add_error("Field count mismatch in struct instantiation: " +
                    node->m_struct_name);
    return;
  }

  for (size_t i = 0; i < node->m_field_values.size(); ++i)
  {
    node->m_field_values[i]->accept(*this);
    if (node->m_field_values[i]->get_type() != structInfo->fields[i].type)
    {
      error.add_error("Field type mismatch in struct instantiation: " +
                      structInfo->fields[i].name + " in " +
                      node->m_struct_name);
    }
  }

  node->set_type(structInfo->type);
}

void SemanticAnalyzer::visit(aloha::StructFieldAccess *node)
{
  node->m_struct_expr->accept(*this);
  AlohaType::Type struct_type = node->m_struct_expr->get_type();

  if (!AlohaType::is_struct_type(struct_type))
  {
    error.add_error("Trying to access field of non-struct type");
    return;
  }
  StructInfo *struct_info = symbol_table.get_struct_by_type(struct_type);
  if (!struct_info)
  {
    error.add_error("Unknown struct type");
    return;
  }

  AlohaType::Type field_type = AlohaType::Type::UNKNOWN;
  for (const auto &field : struct_info->fields)
  {
    if (field.name == node->m_field_name)
    {
      field_type = field.type;
      break;
    }
  }

  if (field_type == AlohaType::Type::UNKNOWN)
  {
    error.add_error("Struct has no field named '" + node->m_field_name + "'");
    return;
  }
  node->set_type(field_type);
}

void SemanticAnalyzer::visit(aloha::StructFieldAssignment *node)
{
  node->m_struct_expr->accept(*this);
  AlohaType::Type struct_type = node->m_struct_expr->get_type();

  if (!AlohaType::is_struct_type(struct_type))
  {
    error.add_error("Trying to assign to a field of non-struct type");
    return;
  }

  aloha::Identifier *ident = dynamic_cast<aloha::Identifier *>(node->m_struct_expr.get());
  if (ident)
  {
    VariableInfo *var_info = symbol_table.get_variable(ident->m_name);
    if (var_info && !var_info->is_mutable)
    {
      error.add_error("Cannot assign to field of immutable struct variable: " +
                      ident->m_name);
      return;
    }
  }

  StructInfo *struct_info = symbol_table.get_struct_by_type(struct_type);
  if (!struct_info)
  {
    error.add_error("Unknown struct type");
    return;
  }

  AlohaType::Type field_type = AlohaType::Type::UNKNOWN;
  for (const auto &field : struct_info->fields)
  {
    if (field.name == node->m_field_name)
    {
      field_type = field.type;
      break;
    }
  }

  if (field_type == AlohaType::Type::UNKNOWN)
  {
    error.add_error("Struct has no field named '" + node->m_field_name + "'");
    return;
  }

  node->m_value->accept(*this);

  if (node->m_value->get_type() != field_type)
  {
    std::string value_type = AlohaType::to_string(node->m_value->get_type());
    std::string field_type_str = AlohaType::to_string(field_type);
    error.add_error("Cannot assign value of type '" + value_type +
                    "' to struct field of type '" + field_type_str + "'");
    return;
  }
  node->m_type = field_type;
}

void SemanticAnalyzer::visit(aloha::StatementBlock *node)
{
  for (auto &stmt : node->m_statements)
  {
    stmt->accept(*this);
  }
}

void SemanticAnalyzer::visit(aloha::Program *node)
{
  for (auto &n : node->m_nodes)
  {
    n->accept(*this);
  }
}

void SemanticAnalyzer::visit(aloha::Import *node)
{
}
