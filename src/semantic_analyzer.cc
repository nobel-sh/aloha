#include "semantic_analyzer.h"
#include "ast.h"
#include "symbolTable.h"
#include "type.h"
#include <vector>

void SemanticAnalyzer::analyze(Aloha::Program *program) {
  program->accept(*this);
  if (!error.isEmpty()) {
    throw error;
  }
}

void SemanticAnalyzer::visit(Aloha::Number *node) {}

void SemanticAnalyzer::visit(Aloha::Boolean *node) {}

void SemanticAnalyzer::visit(Aloha::String *node) {}

void SemanticAnalyzer::visit(Aloha::ExpressionStatement *node) {
  node->expr->accept(*this);
}

void SemanticAnalyzer::visit(Aloha::UnaryExpression *node) {
  node->expr->accept(*this);
}

void SemanticAnalyzer::visit(Aloha::BinaryExpression *node) {
  node->left->accept(*this);
  node->right->accept(*this);

  if (node->left->get_type() != node->right->get_type()) {
    error.addError("Type mismatch in binary expression");
  }
}

void SemanticAnalyzer::visit(Aloha::Identifier *node) {
  VariableInfo *varInfo = symbol_table.getVariable(node->name);
  if (!varInfo) {
    error.addError("Undeclared variable: " + node->name);
    return;
  }
  node->type = varInfo->type;
}

void SemanticAnalyzer::visit(Aloha::Declaration *node) {
  if (node->is_assigned) {
    node->expression->accept(*this);
    if (!node->type) {
      node->type = node->expression->get_type();
    } else if (node->type != node->expression->get_type()) {
      if (AlohaType::is_struct_type(node->type.value()) &&
          AlohaType::is_struct_type(node->expression->get_type())) {

        if (node->type.value() != node->expression->get_type()) {
          error.addError("Struct type mismatch in declaration of " +
                         node->variable_name);
          return;
        }
      } else {
        error.addError("Type mismatch in declaration of " +
                       node->variable_name);
        return;
      }
    }
  }

  if (node->type && AlohaType::is_struct_type(node->type.value())) {
    std::shared_ptr<Aloha::StructInstantiation> struct_ints =
        std::static_pointer_cast<Aloha::StructInstantiation>(node->expression);
    if (!symbol_table.getStruct(struct_ints->m_struct_name)) {
      error.addError("Undeclared struct type: " +
                     AlohaType::to_string(node->type.value()));
      return;
    }
  }

  if (!symbol_table.addVariable(node->variable_name,
                                node->type.value_or(AlohaType::Type::UNKNOWN),
                                node->is_assigned, node->is_mutable)) {
    error.addError("Variable redeclaration: " + node->variable_name);
  }
}

void SemanticAnalyzer::visit(Aloha::Assignment *node) {
  VariableInfo *var_info = symbol_table.getVariable(node->variable_name);
  if (!var_info) {
    error.addError("Cannnot assign to an undeclared variable: " +
                   node->variable_name);
    return;
  }
  if (!var_info->is_mutable && var_info->is_assigned) {
    error.addError("Cannot mutate immutable assigned variable: " +
                   node->variable_name);
    return;
  }

  var_info->is_assigned = true;
  node->expression->accept(*this);

  if (var_info->type == AlohaType::Type::UNKNOWN) {
    var_info->type = node->expression->get_type();
    node->type = var_info->type;
  } else {
    node->type = var_info->type;
    if (node->type != node->expression->get_type()) {
      if (AlohaType::is_struct_type(node->type) &&
          AlohaType::is_struct_type(node->expression->get_type())) {
        if (node->type != node->expression->get_type()) {
          error.addError("Struct type mismatch in assignment to " +
                         node->variable_name);
        }
      } else {
        std::string expr_type =
            AlohaType::to_string(node->expression->get_type());
        std::string var_type = AlohaType::to_string(node->type);
        error.addError("Cannot assign expr of type " + expr_type +
                       " to var of type " + var_type);
      }
    }
  }
}

void SemanticAnalyzer::visit(Aloha::FunctionCall *node) {
  auto isBuiltin = symbol_table.isBuiltinFunction(node->funcName->name);
  if (isBuiltin) {
    return; // HACK: dont check anything for now but should be a good idea to
            // add protoypes and check for types and so on
  }

  FunctionInfo *funcInfo = symbol_table.getFunction(node->funcName->name);
  if (!funcInfo) {
    error.addError("Undeclared function: " + node->funcName->name);
    return;
  }

  // infer type from calle fn's return type
  node->type = funcInfo->return_type;

  if (node->arguments.size() != funcInfo->param_types.size()) {
    error.addError("Argument count mismatch in function call: " +
                   node->funcName->name);
    return;
  }
  for (size_t i = 0; i < node->arguments.size(); ++i) {
    node->arguments[i]->accept(*this);
    if (node->arguments[i]->get_type() != funcInfo->param_types[i]) {
      error.addError("Argument type mismatch in function call: " +
                     node->funcName->name);
    }
  }
}

void SemanticAnalyzer::visit(Aloha::ReturnStatement *node) {
  node->expression->accept(*this);

  if (current_fn && node->expression->get_type() != current_fn->return_type) {
    symbol_table.dump();
    std::cout << "Expr type: "
              << AlohaType::to_string(node->expression->get_type())
              << std::endl;
    std::cout << "Curr fn return type: "
              << AlohaType::to_string(current_fn->return_type) << std::endl;
    error.addError("Return type mismatch in function: " +
                   current_fn->name->name);
  }
}

void SemanticAnalyzer::visit(Aloha::IfStatement *node) {
  node->condition->accept(*this);
  symbol_table.enterScope();
  node->then_branch->accept(*this);
  symbol_table.leaveScope();
  if (node->has_else_branch()) {
    symbol_table.enterScope();
    node->else_branch->accept(*this);
    symbol_table.leaveScope();
  }
}

void SemanticAnalyzer::visit(Aloha::WhileLoop *node) {
  node->condition->accept(*this);
  symbol_table.enterScope();
  node->body->accept(*this);
  symbol_table.leaveScope();
}

void SemanticAnalyzer::visit(Aloha::ForLoop *node) {
  symbol_table.enterScope();
  node->initializer->accept(*this);
  node->condition->accept(*this);
  node->increment->accept(*this);
  for (auto &stmt : node->body) {
    stmt->accept(*this);
  }
  symbol_table.leaveScope();
}

void SemanticAnalyzer::visit(Aloha::Function *node) {
  std::vector<AlohaType::Type> parameterType;
  for (const auto &param : node->parameters) {
    parameterType.push_back(param.type);
  }
  if (!symbol_table.addFunction(node->name->name, node->return_type,
                                parameterType)) {
    error.addError("Function redeclaration: " + node->name->name);
  }

  symbol_table.enterScope();
  current_fn = node;

  for (const auto &param : node->parameters) {
    if (!symbol_table.addVariable(
            param.name, param.type, false,
            true)) { // TODO: allow mutability to be defined in params
      error.addError("Parameter redeclaration: " + param.name);
    }
  }
  node->body->accept(*this);
  current_fn = nullptr;
  symbol_table.leaveScope();
}

void SemanticAnalyzer::visit(Aloha::StructDecl *node) {
  std::vector<StructField> fields;
  for (const auto &field : node->m_fields) {
    fields.push_back({field.m_name, field.m_type});
  }

  AlohaType::Type structType = symbol_table.addStruct(node->m_name, fields);
  if (structType == AlohaType::Type::UNKNOWN) {
    error.addError("Struct redeclaration: " + node->m_name);
  }
}

void SemanticAnalyzer::visit(Aloha::StructInstantiation *node) {
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

void SemanticAnalyzer::visit(Aloha::StructFieldAccess *node) {
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

void SemanticAnalyzer::visit(Aloha::StructFieldAssignment *node) {}

void SemanticAnalyzer::visit(Aloha::StatementList *node) {
  for (auto &stmt : node->statements) {
    stmt->accept(*this);
  }
}

void SemanticAnalyzer::visit(Aloha::Program *node) {
  for (auto &n : node->nodes) {
    n->accept(*this);
  }
}
