#include "semantic_analyzer.h"
#include "ast.h"
#include "symbolTable.h"
#include "type.h"
#include <vector>

void SemanticAnalyzer::analyze(Program *program) {
  program->accept(*this);
  if (!error.isEmpty()) {
    throw error;
  }
}

void SemanticAnalyzer::visit(Number *node) {}

void SemanticAnalyzer::visit(Boolean *node) {}

void SemanticAnalyzer::visit(AlohaString *node) {}

void SemanticAnalyzer::visit(ExpressionStatement *node) {
  node->expr->accept(*this);
}

void SemanticAnalyzer::visit(UnaryExpression *node) {
  node->expr->accept(*this);
  node->type = node->expr->get_type();
}

void SemanticAnalyzer::visit(BinaryExpression *node) {
  node->left->accept(*this);
  node->right->accept(*this);

  if (node->left->get_type() == node->right->get_type()) {
    node->type = node->left->get_type();
  } else {
    error.addError("Type mismatch in binary expression");
  }
}

void SemanticAnalyzer::visit(Identifier *node) {
  VariableInfo *varInfo = symbol_table.getVariable(node->name);
  if (!varInfo) {
    error.addError("Undeclared variable: " + node->name);
  }
  node->type = varInfo->type;
}

void SemanticAnalyzer::visit(Declaration *node) {
  if (node->is_assigned) {
    node->expression->accept(*this);
    if (!node->type) {
      node->type = node->expression->get_type();
    }
  }
  AlohaType::Type type =
      node->type ? node->type.value() : AlohaType::Type::UNKNOWN;
  if (!symbol_table.addVariable(node->variable_name, type, node->is_assigned,
                                node->is_mutable)) {
    error.addError("Variable redeclaration: " + node->variable_name);
    // throw TypeError("Variable redeclaration: " + node->variableName);
  }
}

void SemanticAnalyzer::visit(Assignment *node) {
  VariableInfo *var_info = symbol_table.getVariable(node->variable_name);
  if (!var_info) {
    error.addError("Cannnot assign to an undeclared variable: " +
                   node->variable_name);
  }
  if (!var_info->is_mutable && var_info->is_assigned) {
    error.addError("Cannot mutate immutable assigned variable: " +
                   node->variable_name);
  }
  var_info->is_assigned = true;
  node->expression->accept(*this);
  if (var_info->type == AlohaType::Type::UNKNOWN) {
    node->type = node->expression->get_type();
  } else {
    node->type = var_info->type;
    if (node->type != node->expression->get_type()) {
      std::string expr_type =
          AlohaType::to_string(node->expression->get_type());
      std::string var_type = AlohaType::to_string(node->type);
      error.addError("Cannot assign expr of type " + expr_type +
                     " to var of type " + var_type);
    }
  }
}

void SemanticAnalyzer::visit(FunctionCall *node) {
  FunctionInfo *funcInfo = symbol_table.getFunction(node->funcName->name);
  auto isBuiltin = symbol_table.isBuiltinFunction(node->funcName->name);
  if (isBuiltin) {
    return; // HACK: dont check anything for now but should be a good idea to
            // add protoypes and check for types and so on
  }
  if (!funcInfo) {
    error.addError("Undeclared function: " + node->funcName->name);
    return;
  }
  if (node->arguments.size() != funcInfo->param_types.size()) {
    error.addError("Argument count mismatch in function call: " +
                   node->funcName->name);
  }
  for (size_t i = 0; i < node->arguments.size(); ++i) {
    node->arguments[i]->accept(*this);
    if (node->arguments[i]->get_type() != funcInfo->param_types[i]) {
      error.addError("Argument type mismatch in function call: " +
                     node->funcName->name);
    }
  }
}

void SemanticAnalyzer::visit(ReturnStatement *node) {
  node->expression->accept(*this);

  if (current_fn && node->expression->get_type() != current_fn->return_type) {
    std::cout << AlohaType::to_string(node->expression->get_type())
              << std::endl;
    std::cout << AlohaType::to_string(current_fn->return_type) << std::endl;

    symbol_table.dump();
    error.addError("Return type mismatch in function: " +
                   current_fn->name->name);
  }
}

void SemanticAnalyzer::visit(IfStatement *node) {
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

void SemanticAnalyzer::visit(WhileLoop *node) {
  node->condition->accept(*this);
  symbol_table.enterScope();
  node->body->accept(*this);
  symbol_table.leaveScope();
}

void SemanticAnalyzer::visit(ForLoop *node) {
  symbol_table.enterScope();
  node->initializer->accept(*this);
  node->condition->accept(*this);
  node->increment->accept(*this);
  for (auto &stmt : node->body) {
    stmt->accept(*this);
  }
  symbol_table.leaveScope();
}

void SemanticAnalyzer::visit(Function *node) {
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

void SemanticAnalyzer::visit(StatementList *node) {
  for (auto &stmt : node->statements) {
    stmt->accept(*this);
  }
}

void SemanticAnalyzer::visit(Program *node) {
  for (auto &n : node->nodes) {
    n->accept(*this);
  }
}
