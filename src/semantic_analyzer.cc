#include "semantic_analyzer.h"
#include "ast.h"
#include "symbolTable.h"
#include "type.h"
#include <memory>
#include <vector>

void SemanticAnalyzer::analyze(aloha::Program *program) {
  program->accept(*this);
  if (!error.isEmpty()) {
    throw error;
  }
}

void SemanticAnalyzer::visit(aloha::Number *node) {}

void SemanticAnalyzer::visit(aloha::Boolean *node) {}

void SemanticAnalyzer::visit(aloha::String *node) {}

void SemanticAnalyzer::visit(aloha::ExpressionStatement *node) {
  node->m_expr->accept(*this);
}

void SemanticAnalyzer::visit(aloha::UnaryExpression *node) {
  node->m_expr->accept(*this);
}

void SemanticAnalyzer::visit(aloha::BinaryExpression *node) {
  node->m_left->accept(*this);
  node->m_right->accept(*this);

  if (node->m_left->get_type() != node->m_right->get_type()) {
    error.addError("Type mismatch in binary expression");
  }
}

void SemanticAnalyzer::visit(aloha::Identifier *node) {
  VariableInfo *varInfo = symbol_table.getVariable(node->m_name);
  if (!varInfo) {
    error.addError("Undeclared variable: " + node->m_name);
    return;
  }
  node->m_type = varInfo->type;
}

void SemanticAnalyzer::visit(aloha::Declaration *node) {
  if (node->m_is_assigned) {
    node->m_expression->accept(*this);
    if (!node->m_type) {
      node->m_type = node->m_expression->get_type();
    } else if (node->m_type != node->m_expression->get_type()) {
      if (AlohaType::is_struct_type(node->m_type.value()) &&
          AlohaType::is_struct_type(node->m_expression->get_type())) {

        if (node->m_type.value() != node->m_expression->get_type()) {
          error.addError("Struct type mismatch in declaration of " +
                         node->m_variable_name);
          return;
        }
      } else {
        if (node->m_expression->get_type() != AlohaType::Type::UNKNOWN) {
          error.addError("Type mismatch in declaration of " +
                         node->m_variable_name);
          return;
        }
      }
    }
  }

  if (node->m_type && AlohaType::is_struct_type(node->m_type.value())) {
    if (auto *struct_inst = dynamic_cast<aloha::StructInstantiation *>(
            node->m_expression.get())) {
      if (!symbol_table.getStruct(struct_inst->m_struct_name)) {
        error.addError("Undeclared struct type: " +
                       AlohaType::to_string(node->m_type.value()));
        return;
      }
    } else {
      error.addError("Expected struct instantiation for struct type");
      return;
    }
  }

  if (!symbol_table.addVariable(node->m_variable_name,
                                node->m_type.value_or(AlohaType::Type::UNKNOWN),
                                node->m_is_assigned, node->m_is_mutable)) {
    error.addError("Variable redeclaration: " + node->m_variable_name);
  }
}

void SemanticAnalyzer::visit(aloha::Assignment *node) {
  VariableInfo *var_info = symbol_table.getVariable(node->m_variable_name);
  if (!var_info) {
    error.addError("Cannnot assign to an undeclared variable: " +
                   node->m_variable_name);
    return;
  }
  if (!var_info->is_mutable && var_info->is_assigned) {
    error.addError("Cannot mutate immutable assigned variable: " +
                   node->m_variable_name);
    return;
  }

  var_info->is_assigned = true;
  node->m_expression->accept(*this);

  if (var_info->type == AlohaType::Type::UNKNOWN) {
    var_info->type = node->m_expression->get_type();
    node->m_type = var_info->type;
  } else {
    node->m_type = var_info->type;
    if (node->m_type != node->m_expression->get_type()) {
      if (AlohaType::is_struct_type(node->m_type) &&
          AlohaType::is_struct_type(node->m_expression->get_type())) {
        if (node->m_type != node->m_expression->get_type()) {
          error.addError("Struct type mismatch in assignment to " +
                         node->m_variable_name);
        }
      } else {
        std::string expr_type =
            AlohaType::to_string(node->m_expression->get_type());
        std::string var_type = AlohaType::to_string(node->m_type);
        error.addError("Cannot assign expr of type " + expr_type +
                       " to var of type " + var_type);
      }
    }
  }
}

void SemanticAnalyzer::visit(aloha::FunctionCall *node) {
  auto isBuiltin = symbol_table.isBuiltinFunction(node->m_func_name->m_name);
  if (isBuiltin) {
    return; // HACK: dont check anything for now but should be a good idea to
            // add protoypes and check for types and so on
  }

  FunctionInfo *funcInfo = symbol_table.getFunction(node->m_func_name->m_name);
  if (!funcInfo) {
    error.addError("Undeclared function: " + node->m_func_name->m_name);
    return;
  }

  // infer type from calle fn's return type
  node->m_type = funcInfo->return_type;

  if (node->m_arguments.size() != funcInfo->param_types.size()) {
    error.addError("Argument count mismatch in function call: " +
                   node->m_func_name->m_name);
    return;
  }
  for (size_t i = 0; i < node->m_arguments.size(); ++i) {
    node->m_arguments[i]->accept(*this);
    if (node->m_arguments[i]->get_type() != funcInfo->param_types[i]) {
      error.addError("Argument type mismatch in function call: " +
                     node->m_func_name->m_name);
    }
  }
}

void SemanticAnalyzer::visit(aloha::ReturnStatement *node) {
  node->m_expression->accept(*this);

  if (current_fn &&
      node->m_expression->get_type() != current_fn->m_return_type) {
    symbol_table.dump();
    std::cout << "Expr type: "
              << AlohaType::to_string(node->m_expression->get_type())
              << std::endl;
    std::cout << "Curr fn return type: "
              << AlohaType::to_string(current_fn->m_return_type) << std::endl;
    error.addError("Return type mismatch in function: " +
                   current_fn->m_name->m_name);
  }
}

void SemanticAnalyzer::visit(aloha::IfStatement *node) {
  node->m_condition->accept(*this);
  symbol_table.enterScope();
  node->m_then_branch->accept(*this);
  symbol_table.leaveScope();
  if (node->has_else_branch()) {
    symbol_table.enterScope();
    node->m_else_branch->accept(*this);
    symbol_table.leaveScope();
  }
}

void SemanticAnalyzer::visit(aloha::WhileLoop *node) {
  node->m_condition->accept(*this);
  symbol_table.enterScope();
  node->m_body->accept(*this);
  symbol_table.leaveScope();
}

void SemanticAnalyzer::visit(aloha::ForLoop *node) {
  symbol_table.enterScope();
  node->m_initializer->accept(*this);
  node->m_condition->accept(*this);
  node->m_increment->accept(*this);
  for (auto &stmt : node->m_body) {
    stmt->accept(*this);
  }
  symbol_table.leaveScope();
}

void SemanticAnalyzer::visit(aloha::Function *node) {
  std::vector<AlohaType::Type> parameterType;
  for (const auto &param : node->m_parameters) {
    parameterType.push_back(param.m_type);
  }
  if (!symbol_table.addFunction(node->m_name->m_name, node->m_return_type,
                                parameterType)) {
    error.addError("Function redeclaration: " + node->m_name->m_name);
  }

  symbol_table.enterScope();
  current_fn = node;

  for (const auto &param : node->m_parameters) {
    if (!symbol_table.addVariable(
            param.m_name, param.m_type, false,
            true)) { // TODO: allow mutability to be defined in params
      error.addError("Parameter redeclaration: " + param.m_name);
    }
  }
  node->m_body->accept(*this);
  current_fn = nullptr;
  symbol_table.leaveScope();
}

void SemanticAnalyzer::visit(aloha::StructDecl *node) {
  std::vector<StructField> fields;
  for (const auto &field : node->m_fields) {
    fields.push_back({field.m_name, field.m_type});
  }

  AlohaType::Type structType = symbol_table.addStruct(node->m_name, fields);
  if (structType == AlohaType::Type::UNKNOWN) {
    error.addError("Struct redeclaration: " + node->m_name);
  }
}

void SemanticAnalyzer::visit(aloha::StructInstantiation *node) {
  StructInfo *structInfo = symbol_table.getStruct(node->m_struct_name);
  if (!structInfo) {
    error.addError("Undeclared struct: " + node->m_struct_name);
    return;
  }

  if (node->m_field_values.size() != structInfo->fields.size()) {
    error.addError("Field count mismatch in struct instantiation: " +
                   node->m_struct_name);
    return;
  }

  for (size_t i = 0; i < node->m_field_values.size(); ++i) {
    node->m_field_values[i]->accept(*this);
    if (node->m_field_values[i]->get_type() != structInfo->fields[i].type) {
      error.addError("Field type mismatch in struct instantiation: " +
                     structInfo->fields[i].name + " in " + node->m_struct_name);
    }
  }

  node->set_type(structInfo->type);
}

void SemanticAnalyzer::visit(aloha::StructFieldAccess *node) {
  node->m_struct_expr->accept(*this);
  AlohaType::Type struct_type = node->m_struct_expr->get_type();

  if (!AlohaType::is_struct_type(struct_type)) {
    error.addError("Trying to access field of non-struct type");
    return;
  }
  StructInfo *struct_info = symbol_table.getStructByType(struct_type);
  if (!struct_info) {
    error.addError("Unknown struct type");
    return;
  }

  AlohaType::Type field_type = AlohaType::Type::UNKNOWN;
  for (const auto &field : struct_info->fields) {
    if (field.name == node->m_field_name) {
      field_type = field.type;
      break;
    }
  }

  if (field_type == AlohaType::Type::UNKNOWN) {
    error.addError("Struct has no field named '" + node->m_field_name + "'");
    return;
  }
  node->set_type(field_type);
}

void SemanticAnalyzer::visit(aloha::StructFieldAssignment *node) {
  node->m_struct_expr->accept(*this);
  AlohaType::Type struct_type = node->m_struct_expr->get_type();

  if (!AlohaType::is_struct_type(struct_type)) {
    error.addError("Trying to assign to a field of non-struct type");
    return;
  }

  StructInfo *struct_info = symbol_table.getStructByType(struct_type);
  if (!struct_info) {
    error.addError("Unknown struct type");
    return;
  }

  AlohaType::Type field_type = AlohaType::Type::UNKNOWN;
  for (const auto &field : struct_info->fields) {
    if (field.name == node->m_field_name) {
      field_type = field.type;
      break;
    }
  }

  if (field_type == AlohaType::Type::UNKNOWN) {
    error.addError("Struct has no field named '" + node->m_field_name + "'");
    return;
  }

  node->m_value->accept(*this);

  if (node->m_value->get_type() != field_type) {
    std::string value_type = AlohaType::to_string(node->m_value->get_type());
    std::string field_type_str = AlohaType::to_string(field_type);
    error.addError("Cannot assign value of type '" + value_type +
                   "' to struct field of type '" + field_type_str + "'");
    return;
  }
  node->m_type = field_type;
}

void SemanticAnalyzer::visit(aloha::StatementList *node) {
  for (auto &stmt : node->m_statements) {
    stmt->accept(*this);
  }
}

void SemanticAnalyzer::visit(aloha::Program *node) {
  for (auto &n : node->m_nodes) {
    n->accept(*this);
  }
}
