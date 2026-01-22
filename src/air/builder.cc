#include "builder.h"
#include <iostream>
#include <sstream>

namespace aloha
{

  void AIRBuilder::visit(Number *node)
  {
    double value = std::stod(node->m_value);
    current_expr = std::make_unique<AIR::NumberLiteral>(node->get_location(), value);
  }

  void AIRBuilder::visit(Boolean *node)
  {
    current_expr = std::make_unique<AIR::BoolLiteral>(node->get_location(), node->m_value);
  }

  void AIRBuilder::visit(String *node)
  {
    current_expr = std::make_unique<AIR::StringLiteral>(node->get_location(), node->m_value);
  }

  void AIRBuilder::visit(UnaryExpression *node)
  {
    auto operand = lower_expr(node->m_expr.get());
    if (!operand)
    {
      current_expr.reset();
      return;
    }

    AIR::UnaryOpKind op = ast_op_to_air_unop(node->m_op);
    AIR::TyId operand_ty = operand->ty;

    if (op == AIR::UnaryOpKind::NEG)
    {
      if (operand_ty != AIR::TyIds::NUMBER)
      {
        errors.add_error(node->get_location(),
                         "Negation operator requires numeric operand");
        current_expr = std::make_unique<AIR::UnaryOp>(node->get_location(), op,
                                                      std::move(operand), AIR::TyIds::ERROR);
        return;
      }
      current_expr = std::make_unique<AIR::UnaryOp>(node->get_location(), op,
                                                    std::move(operand), AIR::TyIds::NUMBER);
      return;
    }
    else if (op == AIR::UnaryOpKind::NOT)
    {
      if (operand_ty != AIR::TyIds::BOOL)
      {
        errors.add_error(node->get_location(),
                         "Logical NOT operator requires boolean operand");
        current_expr = std::make_unique<AIR::UnaryOp>(node->get_location(), op,
                                                      std::move(operand), AIR::TyIds::ERROR);
        return;
      }
      current_expr = std::make_unique<AIR::UnaryOp>(node->get_location(), op,
                                                    std::move(operand), AIR::TyIds::BOOL);
      return;
    }

    errors.add_error(node->get_location(),
                     "Unknown unary operator '" + node->m_op + "'");
    current_expr = std::make_unique<AIR::UnaryOp>(node->get_location(), op,
                                                  std::move(operand), AIR::TyIds::ERROR);
  }

  void AIRBuilder::visit(BinaryExpression *node)
  {
    auto left = lower_expr(node->m_left.get());
    auto right = lower_expr(node->m_right.get());

    if (!left || !right)
    {
      current_expr.reset();
      return;
    }

    AIR::TyId left_ty = left->ty;
    AIR::TyId right_ty = right->ty;

    AIR::BinaryOpKind op = ast_op_to_air_binop(node->m_op);

    if (is_arithmetic_op(op))
    {
      if (left_ty != AIR::TyIds::NUMBER || right_ty != AIR::TyIds::NUMBER)
      {
        errors.add_error(node->get_location(),
                         "Arithmetic operation '" + node->m_op +
                             "' requires numeric operands");
        current_expr = std::make_unique<AIR::BinaryOp>(node->get_location(), op,
                                                       std::move(left), std::move(right),
                                                       AIR::TyIds::ERROR);
        return;
      }
      current_expr = std::make_unique<AIR::BinaryOp>(node->get_location(), op,
                                                     std::move(left), std::move(right),
                                                     AIR::TyIds::NUMBER);
      return;
    }
    else if (is_comparison_op(op))
    {
      if (left_ty != right_ty)
      {
        errors.add_error(node->get_location(),
                         "Comparison operation '" + node->m_op +
                             "' requires operands of the same type");
        current_expr = std::make_unique<AIR::BinaryOp>(node->get_location(), op,
                                                       std::move(left), std::move(right),
                                                       AIR::TyIds::ERROR);
        return;
      }
      current_expr = std::make_unique<AIR::BinaryOp>(node->get_location(), op,
                                                     std::move(left), std::move(right),
                                                     AIR::TyIds::BOOL);
      return;
    }
    else if (is_logical_op(op))
    {
      if (left_ty != AIR::TyIds::BOOL || right_ty != AIR::TyIds::BOOL)
      {
        errors.add_error(node->get_location(),
                         "Logical operation '" + node->m_op +
                             "' requires boolean operands");
        current_expr = std::make_unique<AIR::BinaryOp>(node->get_location(), op,
                                                       std::move(left), std::move(right),
                                                       AIR::TyIds::ERROR);
        return;
      }
      current_expr = std::make_unique<AIR::BinaryOp>(node->get_location(), op,
                                                     std::move(left), std::move(right),
                                                     AIR::TyIds::BOOL);
      return;
    }

    errors.add_error(node->get_location(),
                     "Unknown binary operator '" + node->m_op + "'");
    current_expr = std::make_unique<AIR::BinaryOp>(node->get_location(), op,
                                                   std::move(left), std::move(right),
                                                   AIR::TyIds::ERROR);
  }

  void AIRBuilder::visit(Identifier *node)
  {
    const std::string &name = node->m_name;

    auto ty_opt = lookup_variable_type(name);
    if (!ty_opt.has_value())
    {
      errors.add_error(node->get_location(),
                       "Undefined variable '" + name + "'");
      current_expr = std::make_unique<AIR::VarRef>(node->get_location(), name, 0, AIR::TyIds::ERROR);
      return;
    }

    auto var_id_opt = lookup_variable_id(name);
    if (!var_id_opt.has_value())
    {
      errors.add_error(node->get_location(),
                       "Internal error: Variable '" + name + "' has no VarId");
      current_expr = std::make_unique<AIR::VarRef>(node->get_location(), name, 0, AIR::TyIds::ERROR);
      return;
    }

    current_expr = std::make_unique<AIR::VarRef>(node->get_location(), name, var_id_opt.value(), ty_opt.value());
  }

  void AIRBuilder::visit(Declaration *node)
  {
    const std::string &var_name = node->m_variable_name;

    AIR::ExprPtr init_expr;
    AIR::TyId var_ty = AIR::TyIds::VOID;

    if (node->m_expression)
    {
      init_expr = lower_expr(node->m_expression.get());
      if (init_expr)
      {
        var_ty = init_expr->ty;
      }
    }
    else
    {
      errors.add_error(node->get_location(),
                       "Variable '" + var_name + "' requires an initializer");
      var_ty = AIR::TyIds::ERROR;
    }

    register_variable(var_name, var_ty);

    VarId var_id = 0;
    for (const auto &[id, var_symbol] : symbol_table.variables)
    {
      if (var_symbol.name == var_name)
      {
        var_id = id;
        register_variable_id(var_name, id);
        break;
      }
    }
    current_stmt = std::make_unique<AIR::VarDecl>(node->get_location(), var_name, var_id,
                                                  node->m_is_mutable, var_ty,
                                                  std::move(init_expr));
  }

  void AIRBuilder::visit(Assignment *node)
  {
    const std::string &var_name = node->m_variable_name;

    auto var_ty_opt = lookup_variable_type(var_name);
    if (!var_ty_opt.has_value())
    {
      errors.add_error(node->get_location(),
                       "Undefined variable '" + var_name + "'");
      var_ty_opt = AIR::TyIds::ERROR;
    }

    auto value_expr = lower_expr(node->m_expression.get());
    if (!value_expr)
    {
      current_stmt.reset();
      return;
    }

    if (var_ty_opt.has_value())
    {
      check_types_compatible(var_ty_opt.value(), value_expr->ty,
                             node->get_location(), "assignment");
    }

    auto var_id_opt = lookup_variable_id(var_name);
    if (!var_id_opt.has_value())
    {
      errors.add_error(node->get_location(),
                       "Assignment to undefined variable: '" + var_name + "'");
      current_stmt = std::make_unique<AIR::Assignment>(node->get_location(), var_name, 0,
                                                       std::move(value_expr));
      return;
    }

    current_stmt = std::make_unique<AIR::Assignment>(node->get_location(), var_name, var_id_opt.value(),
                                                     std::move(value_expr));
  }

  void AIRBuilder::visit(FunctionCall *node)
  {
    const std::string &func_name = node->m_func_name->m_name;

    auto func_opt = symbol_table.lookup_function(func_name);
    if (!func_opt.has_value())
    {
      errors.add_error(node->get_location(),
                       "Undefined function '" + func_name + "'");
      current_expr.reset();
      return;
    }

    const FunctionSymbol &func_symbol = func_opt.value();

    if (node->m_arguments.size() != func_symbol.param_types.size())
    {
      std::ostringstream msg;
      msg << "Function '" << func_name << "' expects "
          << func_symbol.param_types.size() << " argument(s), got "
          << node->m_arguments.size();
      errors.add_error(node->get_location(), msg.str());
    }

    std::vector<AIR::ExprPtr> args;
    for (size_t i = 0; i < node->m_arguments.size(); ++i)
    {
      auto arg = lower_expr(node->m_arguments[i].get());
      if (!arg)
      {
        continue;
      }

      if (i < func_symbol.param_types.size())
      {
        AIR::TyId expected_ty = func_symbol.param_types[i];
        AIR::TyId actual_ty = arg->ty;

        check_types_compatible(expected_ty, actual_ty,
                               node->m_arguments[i]->get_location(),
                               "function argument");
      }

      args.emplace_back(std::move(arg));
    }

    current_expr = std::make_unique<AIR::Call>(node->get_location(), func_name, func_symbol.id,
                                               std::move(args), func_symbol.return_type);
  }

  void AIRBuilder::visit(ReturnStatement *node)
  {
    AIR::ExprPtr value;
    AIR::TyId return_ty = AIR::TyIds::VOID;

    if (node->m_expression)
    {
      value = lower_expr(node->m_expression.get());
      if (value)
      {
        return_ty = value->ty;
      }
    }

    check_types_compatible(current_function_return_type, return_ty,
                           node->get_location(), "return statement");

    current_stmt = std::make_unique<AIR::Return>(node->get_location(), std::move(value));
  }

  void AIRBuilder::visit(IfStatement *node)
  {
    auto condition = lower_expr(node->m_condition.get());
    if (!condition)
    {
      current_stmt.reset();
      return;
    }

    if (condition->ty != AIR::TyIds::BOOL)
    {
      errors.add_error(node->m_condition->get_location(),
                       "If condition must be of type bool");
    }

    std::vector<AIR::StmtPtr> then_branch;
    if (node->m_then_branch)
    {
      then_branch = lower_block(node->m_then_branch.get());
    }

    std::vector<AIR::StmtPtr> else_branch;
    if (node->has_else_branch() && node->m_else_branch)
    {
      else_branch = lower_block(node->m_else_branch.get());
    }

    current_stmt = std::make_unique<AIR::If>(node->get_location(), std::move(condition),
                                             std::move(then_branch), std::move(else_branch));
  }

  void AIRBuilder::visit(WhileLoop *node)
  {
    errors.add_error(node->get_location(),
                     "While loops not yet supported in AIR lowering");
    current_stmt.reset();
  }

  void AIRBuilder::visit(ForLoop *node)
  {
    errors.add_error(node->get_location(),
                     "For loops not yet supported in AIR lowering");
    current_stmt.reset();
  }

  void AIRBuilder::visit(Function *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(StructDecl *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(StructInstantiation *node)
  {
    const std::string &struct_name = node->m_struct_name;

    const ResolvedStruct *resolved = lookup_resolved_struct(struct_name);
    if (!resolved)
    {
      errors.add_error(node->get_location(),
                       "Undefined struct '" + struct_name + "'");
      current_expr.reset();
      return;
    }

    if (node->m_field_values.size() != resolved->fields.size())
    {
      std::ostringstream msg;
      msg << "Struct '" << struct_name << "' expects "
          << resolved->fields.size() << " field(s), got "
          << node->m_field_values.size();
      errors.add_error(node->get_location(), msg.str());
    }

    std::vector<AIR::ExprPtr> field_values;
    for (size_t i = 0; i < node->m_field_values.size(); ++i)
    {
      auto value = lower_expr(node->m_field_values[i].get());
      if (!value)
      {
        continue;
      }

      if (i < resolved->fields.size())
      {
        AIR::TyId expected_ty = resolved->fields[i].type_id;
        AIR::TyId actual_ty = value->ty;

        check_types_compatible(expected_ty, actual_ty,
                               node->m_field_values[i]->get_location(),
                               "struct field");
      }

      field_values.emplace_back(std::move(value));
    }

    current_expr = std::make_unique<AIR::StructInstantiation>(node->get_location(), struct_name,
                                                              resolved->struct_id,
                                                              std::move(field_values), resolved->type_id);
  }

  void AIRBuilder::visit(StructFieldAccess *node)
  {
    auto struct_expr = lower_expr(node->m_struct_expr.get());
    if (!struct_expr)
    {
      current_expr.reset();
      return;
    }

    AIR::TyId struct_ty = struct_expr->ty;

    if (!ty_table.is_struct(struct_ty))
    {
      errors.add_error(node->get_location(),
                       "Field access requires struct type");
      current_expr = std::make_unique<AIR::FieldAccess>(node->get_location(),
                                                        std::move(struct_expr),
                                                        node->m_field_name, 0, AIR::TyIds::ERROR);
      return;
    }

    auto ty_info = ty_table.get_ty_info(struct_ty);
    if (!ty_info || !ty_info->struct_id.has_value())
    {
      errors.add_error(node->get_location(),
                       "Internal error: invalid struct type");
      current_expr = std::make_unique<AIR::FieldAccess>(node->get_location(),
                                                        std::move(struct_expr),
                                                        node->m_field_name, 0, AIR::TyIds::ERROR);
      return;
    }

    const ResolvedStruct *resolved = lookup_resolved_struct_by_id(ty_info->struct_id.value());
    if (!resolved)
    {
      errors.add_error(node->get_location(),
                       "Internal error: struct not resolved");
      current_expr = std::make_unique<AIR::FieldAccess>(node->get_location(),
                                                        std::move(struct_expr),
                                                        node->m_field_name, 0, AIR::TyIds::ERROR);
      return;
    }

    const std::string &field_name = node->m_field_name;
    uint32_t field_index = 0;
    AIR::TyId field_ty = AIR::TyIds::ERROR;
    bool found = false;

    for (size_t i = 0; i < resolved->fields.size(); ++i)
    {
      if (resolved->fields[i].name == field_name)
      {
        field_index = static_cast<uint32_t>(i);
        field_ty = resolved->fields[i].type_id;
        found = true;
        break;
      }
    }

    if (!found)
    {
      errors.add_error(node->get_location(),
                       "Struct '" + resolved->name + "' has no field '" +
                           field_name + "'");
      current_expr = std::make_unique<AIR::FieldAccess>(node->get_location(),
                                                        std::move(struct_expr),
                                                        field_name, 0, AIR::TyIds::ERROR);
      return;
    }

    current_expr = std::make_unique<AIR::FieldAccess>(node->get_location(),
                                                      std::move(struct_expr),
                                                      field_name, field_index, field_ty);
  }

  void AIRBuilder::visit(StructFieldAssignment *node)
  {
    auto struct_expr = lower_expr(node->m_struct_expr.get());
    if (!struct_expr)
    {
      current_stmt.reset();
      return;
    }

    auto value_expr = lower_expr(node->m_value.get());
    if (!value_expr)
    {
      current_stmt.reset();
      return;
    }

    AIR::TyId struct_ty = struct_expr->ty;
    if (!ty_table.is_struct(struct_ty))
    {
      errors.add_error(node->get_location(),
                       "Field assignment requires struct type");
      current_stmt = std::make_unique<AIR::FieldAssignment>(node->get_location(),
                                                            std::move(struct_expr),
                                                            node->m_field_name, 0,
                                                            std::move(value_expr));
      return;
    }

    auto ty_info = ty_table.get_ty_info(struct_ty);
    if (!ty_info || !ty_info->struct_id.has_value())
    {
      errors.add_error(node->get_location(),
                       "Internal error: invalid struct type");
      current_stmt = std::make_unique<AIR::FieldAssignment>(node->get_location(),
                                                            std::move(struct_expr),
                                                            node->m_field_name, 0,
                                                            std::move(value_expr));
      return;
    }

    const ResolvedStruct *resolved = lookup_resolved_struct_by_id(ty_info->struct_id.value());
    if (!resolved)
    {
      errors.add_error(node->get_location(),
                       "Internal error: struct not resolved");
      current_stmt = std::make_unique<AIR::FieldAssignment>(node->get_location(),
                                                            std::move(struct_expr),
                                                            node->m_field_name, 0,
                                                            std::move(value_expr));
      return;
    }

    const std::string &field_name = node->m_field_name;
    uint32_t field_index = 0;
    AIR::TyId field_ty = AIR::TyIds::ERROR;
    bool found = false;

    for (size_t i = 0; i < resolved->fields.size(); ++i)
    {
      if (resolved->fields[i].name == field_name)
      {
        field_index = static_cast<uint32_t>(i);
        field_ty = resolved->fields[i].type_id;
        found = true;
        break;
      }
    }

    if (!found)
    {
      errors.add_error(node->get_location(),
                       "Struct '" + resolved->name + "' has no field '" +
                           field_name + "'");
    }
    else
    {
      check_types_compatible(field_ty, value_expr->ty,
                             node->get_location(), "field assignment");
    }

    current_stmt = std::make_unique<AIR::FieldAssignment>(node->get_location(),
                                                          std::move(struct_expr),
                                                          field_name, field_index,
                                                          std::move(value_expr));
  }

  void AIRBuilder::visit(Array *node)
  {
    errors.add_error(node->get_location(),
                     "Arrays not yet supported in AIR lowering");
    current_expr.reset();
  }

  void AIRBuilder::visit(ExpressionStatement *node)
  {
    auto expr = lower_expr(node->m_expr.get());
    if (!expr)
    {
      current_stmt.reset();
      return;
    }

    current_stmt = std::make_unique<AIR::ExprStmt>(node->get_location(), std::move(expr));
  }

  void AIRBuilder::visit(StatementBlock *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(Program *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(Import *node)
  {
    (void)node;
  }

  std::unique_ptr<AIR::Module> AIRBuilder::build(Program *program)
  {
    if (!program)
    {
      errors.add_error(Location(0, 0), "Null program passed to AIRBuilder");
      return nullptr;
    }

    (void)resolved_functions;

    auto module = std::make_unique<AIR::Module>(Location(0, 0), "");

    for (const auto &node : program->m_nodes)
    {
      if (auto struct_decl = dynamic_cast<StructDecl *>(node.get()))
      {
        auto air_struct = lower_struct(struct_decl);
        if (air_struct)
        {
          module->structs.emplace_back(std::move(air_struct));
        }
      }
    }

    for (const auto &node : program->m_nodes)
    {
      if (auto func = dynamic_cast<Function *>(node.get()))
      {
        auto air_func = lower_function(func);
        if (air_func)
        {
          module->functions.emplace_back(std::move(air_func));
        }
      }
    }

    if (errors.has_errors())
    {
      errors.print_errors();
      return nullptr;
    }

    return module;
  }

  AIR::StructDeclPtr AIRBuilder::lower_struct(StructDecl *struct_decl)
  {
    const std::string &name = struct_decl->m_name;

    // look up resolved struct info
    const ResolvedStruct *resolved = lookup_resolved_struct(name);
    if (!resolved)
    {
      errors.add_error(struct_decl->get_location(),
                       "Internal error: struct '" + name + "' not resolved");
      return nullptr;
    }

    // create air struct declaration with field objects
    std::vector<AIR::Field> fields;
    for (size_t i = 0; i < resolved->fields.size(); ++i)
    {
      const auto &field = resolved->fields[i];
      fields.emplace_back(field.name, field.type_id, static_cast<uint32_t>(i), field.location);
    }

    return std::make_unique<AIR::StructDecl>(struct_decl->get_location(), name,
                                             resolved->struct_id, resolved->type_id, fields);
  }

  AIR::FunctionPtr AIRBuilder::lower_function(Function *func)
  {
    const std::string &name = func->m_name->m_name;

    // look up resolved function info
    auto func_opt = symbol_table.lookup_function(name);
    if (!func_opt.has_value())
    {
      errors.add_error(func->get_location(),
                       "Internal error: function '" + name + "' not in symbol table");
      return nullptr;
    }

    const FunctionSymbol &func_symbol = func_opt.value();

    // clear variable types and ids for new function
    var_types.clear();
    var_ids.clear();

    // set current function return type for return statement checking
    current_function_return_type = func_symbol.return_type;

    // register parameter types
    std::vector<AIR::Param> params;
    for (size_t i = 0; i < func->m_parameters.size(); ++i)
    {
      const auto &param = func->m_parameters[i];
      AIR::TyId param_ty = func_symbol.param_types[i];

      // look up parameter's varId from symbol table
      // parameters are registered as variables during def collection
      VarId param_var_id = 0;
      for (const auto &[var_id, var_symbol] : symbol_table.variables)
      {
        if (var_symbol.name == param.m_name)
        {
          param_var_id = var_id;
          break;
        }
      }

      // create AIR::Param with all required fields
      params.emplace_back(param.m_name, param_var_id, param_ty, false, func->get_location());

      // register parameter in variable type and id maps
      register_variable(param.m_name, param_ty);
      register_variable_id(param.m_name, param_var_id);
    }

    // lower function body if not extern
    std::vector<AIR::StmtPtr> body;
    if (!func->m_is_extern && func->m_body)
    {
      body = lower_block(func->m_body.get());
    }

    return std::make_unique<AIR::Function>(func->get_location(), name, func_symbol.id,
                                           std::move(params), func_symbol.return_type,
                                           std::move(body), func->m_is_extern);
  }

  AIR::ExprPtr AIRBuilder::lower_expr(Expression *expr)
  {
    if (!expr)
    {
      return nullptr;
    }

    current_expr.reset();
    expr->accept(*this);
    if (!current_expr)
    {
      errors.add_error(expr->get_location(), "Unknown expression type");
    }
    return std::move(current_expr);
  }

  AIR::StmtPtr AIRBuilder::lower_stmt(Statement *stmt)
  {
    if (!stmt)
    {
      return nullptr;
    }

    current_stmt.reset();
    stmt->accept(*this);
    if (!current_stmt)
    {
      errors.add_error(stmt->get_location(), "Unknown statement type");
    }
    return std::move(current_stmt);
  }

  std::vector<AIR::StmtPtr> AIRBuilder::lower_block(StatementBlock *block)
  {
    std::vector<AIR::StmtPtr> stmts;

    for (const auto &stmt : block->m_statements)
    {
      auto air_stmt = lower_stmt(stmt.get());
      if (air_stmt)
      {
        stmts.emplace_back(std::move(air_stmt));
      }
    }

    return stmts;
  }

  bool AIRBuilder::check_types_compatible(AIR::TyId expected, AIR::TyId actual,
                                          Location loc, const std::string &context)
  {
    if (expected == actual)
    {
      return true;
    }

    // error type is always compatible to avoid cascading errors
    if (expected == AIR::TyIds::ERROR || actual == AIR::TyIds::ERROR)
    {
      return true;
    }

    std::ostringstream msg;
    msg << "Type mismatch in " << context << ": expected '"
        << ty_table.ty_name(expected) << "', got '"
        << ty_table.ty_name(actual) << "'";
    errors.add_error(loc, msg.str());
    return false;
  }

  AIR::BinaryOpKind AIRBuilder::ast_op_to_air_binop(const std::string &op)
  {
    if (op == "+")
      return AIR::BinaryOpKind::ADD;
    if (op == "-")
      return AIR::BinaryOpKind::SUB;
    if (op == "*")
      return AIR::BinaryOpKind::MUL;
    if (op == "/")
      return AIR::BinaryOpKind::DIV;
    if (op == "%")
      return AIR::BinaryOpKind::MOD;
    if (op == "==")
      return AIR::BinaryOpKind::EQ;
    if (op == "!=")
      return AIR::BinaryOpKind::NE;
    if (op == "<")
      return AIR::BinaryOpKind::LT;
    if (op == "<=")
      return AIR::BinaryOpKind::LE;
    if (op == ">")
      return AIR::BinaryOpKind::GT;
    if (op == ">=")
      return AIR::BinaryOpKind::GE;
    if (op == "&&")
      return AIR::BinaryOpKind::AND;
    if (op == "||")
      return AIR::BinaryOpKind::OR;

    // unknown operator
    return AIR::BinaryOpKind::ADD;
  }

  AIR::UnaryOpKind AIRBuilder::ast_op_to_air_unop(const std::string &op)
  {
    if (op == "-")
      return AIR::UnaryOpKind::NEG;
    if (op == "!")
      return AIR::UnaryOpKind::NOT;

    // unknown operator
    return AIR::UnaryOpKind::NOT;
  }

  bool AIRBuilder::is_arithmetic_op(AIR::BinaryOpKind op)
  {
    return op == AIR::BinaryOpKind::ADD ||
           op == AIR::BinaryOpKind::SUB ||
           op == AIR::BinaryOpKind::MUL ||
           op == AIR::BinaryOpKind::DIV ||
           op == AIR::BinaryOpKind::MOD;
  }

  bool AIRBuilder::is_comparison_op(AIR::BinaryOpKind op)
  {
    return op == AIR::BinaryOpKind::EQ ||
           op == AIR::BinaryOpKind::NE ||
           op == AIR::BinaryOpKind::LT ||
           op == AIR::BinaryOpKind::LE ||
           op == AIR::BinaryOpKind::GT ||
           op == AIR::BinaryOpKind::GE;
  }

  bool AIRBuilder::is_logical_op(AIR::BinaryOpKind op)
  {
    return op == AIR::BinaryOpKind::AND ||
           op == AIR::BinaryOpKind::OR;
  }

  void AIRBuilder::register_variable(const std::string &name, AIR::TyId type)
  {
    var_types[name] = type;
  }

  void AIRBuilder::register_variable_id(const std::string &name, VarId id)
  {
    var_ids[name] = id;
  }

  std::optional<AIR::TyId> AIRBuilder::lookup_variable_type(const std::string &name)
  {
    auto it = var_types.find(name);
    if (it != var_types.end())
    {
      return it->second;
    }
    return std::nullopt;
  }

  std::optional<VarId> AIRBuilder::lookup_variable_id(const std::string &name)
  {
    auto it = var_ids.find(name);
    if (it != var_ids.end())
    {
      return it->second;
    }
    return std::nullopt;
  }

  const ResolvedStruct *AIRBuilder::lookup_resolved_struct(const std::string &name)
  {
    // look up struct in symbol table first
    auto struct_opt = symbol_table.lookup_struct(name);
    if (!struct_opt.has_value())
    {
      return nullptr;
    }

    // then look up in resolved structs
    auto it = resolved_structs.find(struct_opt->struct_id);
    if (it != resolved_structs.end())
    {
      return &it->second;
    }

    return nullptr;
  }

  const ResolvedStruct *AIRBuilder::lookup_resolved_struct_by_id(AIR::StructId struct_id)
  {
    auto it = resolved_structs.find(struct_id);
    if (it != resolved_structs.end())
    {
      return &it->second;
    }
    return nullptr;
  }

} // namespace aloha