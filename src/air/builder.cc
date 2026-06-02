#include "builder.h"
#include <iostream>
#include <sstream>
#include "../error/internal.h"

namespace aloha
{

  void AIRBuilder::visit(ast::Integer *node)
  {
    current_expr = std::make_unique<air::IntegerLiteral>(node->m_loc,
                                                         node->m_value);
  }
  void AIRBuilder::visit(ast::Float *node)
  {
    current_expr = std::make_unique<air::FloatLiteral>(node->m_loc,
                                                       node->m_value);
  }

  void AIRBuilder::visit(ast::Boolean *node)
  {
    current_expr = std::make_unique<air::BoolLiteral>(node->m_loc, node->m_value);
  }

  void AIRBuilder::visit(ast::Null *node)
  {
    current_expr = std::make_unique<air::NullLiteral>(node->m_loc);
  }

  void AIRBuilder::visit(ast::String *node)
  {
    current_expr = std::make_unique<air::StringLiteral>(node->m_loc, node->m_value);
  }

  void AIRBuilder::visit(ast::EnumVariant *node)
  {
    auto variant_opt = symbol_table.lookup_enum_variant(node->m_enum_name, node->m_variant_name);
    if (!variant_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Unknown enum variant '" + node->m_enum_name + "::" +
                            node->m_variant_name + "'");
      current_expr.reset();
      return;
    }

    const EnumVariantSymbol &variant = variant_opt.value();
    current_expr = std::make_unique<air::EnumValue>(
        node->m_loc, variant.enum_name, variant.variant_name,
        variant.value, variant.enum_type_id);
  }

  void AIRBuilder::visit(ast::MatchExpression *node)
  {
    auto scrutinee = lower_expr(node->m_scrutinee.get());
    if (!scrutinee)
    {
      current_expr.reset();
      return;
    }

    if (!ty_table.is_enum(scrutinee->m_ty))
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_scrutinee->m_loc,
                        "Match expression must be an enum");
      current_expr.reset();
      return;
    }

    const TyInfo *enum_ty = ty_table.get_ty_info(scrutinee->m_ty);
    std::string enum_name = enum_ty ? enum_ty->m_name : "";
    bool has_wildcard = false;
    std::unordered_set<std::string> covered_variants;
    std::vector<air::MatchExprArm> arms;
    TyId result_ty = TyIds::ERROR;

    for (const auto &arm : node->m_arms)
    {
      if (has_wildcard)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, arm.m_loc,
                          "Wildcard match arm must be the last arm");
      }

      auto value = lower_expr(arm.m_value.get());
      if (!value)
      {
        continue;
      }

      if (result_ty == TyIds::ERROR)
      {
        result_ty = value->m_ty;
      }
      else
      {
        check_types_compatible(result_ty, value->m_ty, arm.m_loc,
                               "match expression arm");
      }

      if (arm.m_is_wildcard)
      {
        has_wildcard = true;
        arms.emplace_back(std::move(value), arm.m_loc);
        continue;
      }

      auto variant_opt = symbol_table.lookup_enum_variant(arm.m_enum_name, arm.m_variant_name);
      if (!variant_opt.has_value())
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, arm.m_loc,
                          "Unknown enum variant '" + arm.m_enum_name + "::" +
                              arm.m_variant_name + "'");
        continue;
      }

      const EnumVariantSymbol &variant = variant_opt.value();
      if (variant.enum_type_id != scrutinee->m_ty)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, arm.m_loc,
                          "Match arm variant '" + arm.m_enum_name + "::" +
                              arm.m_variant_name + "' does not belong to enum '" +
                              enum_name + "'");
      }

      if (!covered_variants.insert(variant.variant_name).second)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, arm.m_loc,
                          "Duplicate match arm for variant '" + arm.m_enum_name +
                              "::" + arm.m_variant_name + "'");
      }

      arms.emplace_back(variant.enum_name, variant.variant_name, variant.value,
                        std::move(value), arm.m_loc);
    }

    if (!has_wildcard)
    {
      auto variants = symbol_table.get_enum_variants(enum_name);
      if (covered_variants.size() != variants.size())
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                          "Match expression must cover all variants of enum '" +
                              enum_name + "' or include '_'");
      }
    }

    current_expr = std::make_unique<air::MatchExpr>(node->m_loc, std::move(scrutinee),
                                                    std::move(arms), result_ty);
  }

  void AIRBuilder::visit(ast::UnaryExpression *node)
  {
    auto operand = lower_expr(node->m_expr.get());
    if (!operand)
    {
      current_expr.reset();
      return;
    }

    air::UnaryOpKind op = ast_op_to_air_unop(node->m_op);
    auto op_str = ast::Operator::string(node->m_op);
    TyId operand_ty = operand->m_ty;

    if (op == air::UnaryOpKind::NEG)
    {
      if (operand_ty != TyIds::INTEGER && operand_ty != TyIds::FLOAT)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                          "Negation operator requires numeric operand");
        current_expr = std::make_unique<air::UnaryOp>(node->m_loc, op,
                                                      std::move(operand), TyIds::ERROR);
        return;
      }
      current_expr = std::make_unique<air::UnaryOp>(node->m_loc, op,
                                                    std::move(operand), operand_ty);
      return;
    }
    else if (op == air::UnaryOpKind::NOT)
    {
      if (operand_ty != TyIds::BOOL)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                          "Logical NOT operator requires boolean operand");
        current_expr = std::make_unique<air::UnaryOp>(node->m_loc, op,
                                                      std::move(operand), TyIds::ERROR);
        return;
      }
      current_expr = std::make_unique<air::UnaryOp>(node->m_loc, op,
                                                    std::move(operand), TyIds::BOOL);
      return;
    }

    diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                      "Unknown unary operator '" + op_str + "'");
    current_expr = std::make_unique<air::UnaryOp>(node->m_loc, op,
                                                  std::move(operand), TyIds::ERROR);
  }

  void AIRBuilder::visit(ast::BinaryExpression *node)
  {
    auto left = lower_expr(node->m_left.get());
    auto right = lower_expr(node->m_right.get());

    if (!left || !right)
    {
      current_expr.reset();
      return;
    }

    TyId left_ty = left->m_ty;
    TyId right_ty = right->m_ty;

    air::BinaryOpKind op = ast_op_to_air_binop(node->m_op);
    auto op_str = ast::Operator::string(node->m_op);

    if (is_arithmetic_op(op))
    {
      if ((left_ty == TyIds::INTEGER && right_ty == TyIds::INTEGER) ||
          (left_ty == TyIds::FLOAT && right_ty == TyIds::FLOAT))
      {
        TyId result_ty = left_ty; // both are same type here
        current_expr = std::make_unique<air::BinaryOp>(node->m_loc, op,
                                                       std::move(left), std::move(right),
                                                       result_ty);
        return;
      }
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Arithmetic operation '" + op_str +
                            "' requires numeric operands");
      current_expr = std::make_unique<air::BinaryOp>(node->m_loc, op,
                                                     std::move(left), std::move(right),
                                                     TyIds::ERROR);
      return;
    }
    else if (is_comparison_op(op))
    {
      if (left_ty != right_ty)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                          "Comparison operation '" + op_str +
                              "' requires operands of the same type");
        current_expr = std::make_unique<air::BinaryOp>(node->m_loc, op,
                                                       std::move(left), std::move(right),
                                                       TyIds::ERROR);
        return;
      }
      if ((op == air::BinaryOpKind::LT || op == air::BinaryOpKind::LE ||
           op == air::BinaryOpKind::GT || op == air::BinaryOpKind::GE) &&
          left_ty != TyIds::INTEGER && left_ty != TyIds::FLOAT)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                          "Comparison operation '" + op_str +
                              "' requires numeric operands");
        current_expr = std::make_unique<air::BinaryOp>(node->m_loc, op,
                                                       std::move(left), std::move(right),
                                                       TyIds::ERROR);
        return;
      }
      current_expr = std::make_unique<air::BinaryOp>(node->m_loc, op,
                                                     std::move(left), std::move(right),
                                                     TyIds::BOOL);
      return;
    }
    else if (is_logical_op(op))
    {
      if (left_ty != TyIds::BOOL || right_ty != TyIds::BOOL)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                          "Logical operation '" + op_str +
                              "' requires boolean operands");
        current_expr = std::make_unique<air::BinaryOp>(node->m_loc, op,
                                                       std::move(left), std::move(right),
                                                       TyIds::ERROR);
        return;
      }
      current_expr = std::make_unique<air::BinaryOp>(node->m_loc, op,
                                                     std::move(left), std::move(right),
                                                     TyIds::BOOL);
      return;
    }

    diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                      "Unknown binary operator '" + op_str + "'");
    current_expr = std::make_unique<air::BinaryOp>(node->m_loc, op,
                                                   std::move(left), std::move(right),
                                                   TyIds::ERROR);
  }

  void AIRBuilder::visit(ast::Identifier *node)
  {
    const std::string &name = node->m_name;

    auto ty_opt = lookup_variable_type(name);
    if (!ty_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Undefined variable '" + name + "'");
      current_expr = std::make_unique<air::VarRef>(node->m_loc, name, 0, TyIds::ERROR);
      return;
    }

    auto var_id_opt = lookup_variable_id(name);
    if (!var_id_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Internal error: Variable '" + name + "' has no VarId");
      current_expr = std::make_unique<air::VarRef>(node->m_loc, name, 0, TyIds::ERROR);
      return;
    }

    current_expr = std::make_unique<air::VarRef>(node->m_loc, name, var_id_opt.value(), ty_opt.value());
  }

  void AIRBuilder::visit(ast::Declaration *node)
  {
    const std::string &var_name = node->m_variable_name;

    air::ExprPtr init_expr;
    TyId var_ty = TyIds::VOID;

    if (node->m_type.has_value())
    {
      auto ty_opt = type_resolver.resolve_type_spec(node->m_type.value(), type_arena);
      if (ty_opt.has_value())
      {
        var_ty = ty_opt.value();
      }
      else
      {
        var_ty = TyIds::ERROR;
      }
    }

    if (node->m_expression)
    {
      init_expr = lower_expr(node->m_expression.get());
      if (init_expr)
      {
        // infer from initializer
        if (!node->m_type.has_value())
        {
          if (init_expr->m_ty == TyIds::NULL_TY)
          {
            diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                              "Cannot infer type from null");
            var_ty = TyIds::ERROR;
          }
          else
          {
            var_ty = init_expr->m_ty;
          }
        }
        // else check from initializer
        else if (var_ty != TyIds::ERROR)
        {
          check_types_compatible(var_ty, init_expr->m_ty, node->m_loc, "variable initialization");
        }
      }
    }
    else
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Variable '" + var_name + "' requires an initializer");
      if (var_ty == TyIds::VOID)
      {
        var_ty = TyIds::ERROR;
      }
    }

    auto var_id = find_variable_id_for_declaration(node);
    if (!var_id.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Internal error: variable '" + var_name + "' has no VarId");
      var_id = 0;
    }

    register_variable(var_name, var_id.value(), var_ty, node->m_is_mutable);

    current_stmt = std::make_unique<air::VarDecl>(node->m_loc, var_name, var_id.value(),
                                                  node->m_is_mutable, var_ty,
                                                  std::move(init_expr));
  }

  void AIRBuilder::visit(ast::Assignment *node)
  {
    const std::string &var_name = node->m_variable_name;

    auto binding_opt = lookup_variable(var_name);
    if (!binding_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Undefined variable '" + var_name + "'");
    }

    auto value_expr = lower_expr(node->m_expression.get());
    if (!value_expr)
    {
      current_stmt.reset();
      return;
    }

    if (!binding_opt.has_value())
    {
      current_stmt = std::make_unique<air::Assignment>(node->m_loc, var_name, 0,
                                                       std::move(value_expr));
      return;
    }

    const VarBinding &binding = binding_opt.value();

    if (!binding.is_mutable)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Cannot assign to immutable variable '" + var_name + "'");
    }

    check_types_compatible(binding.type, value_expr->m_ty,
                           node->m_loc, "assignment");

    current_stmt = std::make_unique<air::Assignment>(node->m_loc, var_name, binding.id,
                                                     std::move(value_expr));
  }

  void AIRBuilder::visit(ast::ArrayAssignment *node)
  {
    const std::string &array_name = node->m_array_name;

    auto binding_opt = lookup_variable(array_name);
    if (!binding_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Undefined variable '" + array_name + "'");
    }

    auto index_expr = lower_expr(node->m_index_expr.get());
    auto value_expr = lower_expr(node->m_value.get());
    if (!index_expr || !value_expr)
    {
      current_stmt.reset();
      return;
    }

    if (!binding_opt.has_value())
    {
      current_stmt = std::make_unique<air::ArrayAssignment>(
          node->m_loc, array_name, 0, TyIds::ERROR, std::move(index_expr),
          std::move(value_expr));
      return;
    }

    const VarBinding &binding = binding_opt.value();

    if (!binding.is_mutable)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Cannot assign to element of immutable array '" + array_name + "'");
    }

    if (!ty_table.is_array(binding.type))
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Array assignment requires array type");
      current_stmt = std::make_unique<air::ArrayAssignment>(
          node->m_loc, array_name, binding.id, TyIds::ERROR, std::move(index_expr),
          std::move(value_expr));
      return;
    }

    if (index_expr->m_ty != TyIds::INTEGER)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Array index must be of type integer");
    }

    auto element_ty = ty_table.get_array_element_type(binding.type);
    if (!element_ty.has_value())
    {
      ALOHA_ICE("Internal error: unable to get array element type" + node->m_loc.to_string());
      current_stmt.reset();
      return;
    }

    check_types_compatible(element_ty.value(), value_expr->m_ty,
                           node->m_loc, "array assignment");

    current_stmt = std::make_unique<air::ArrayAssignment>(
        node->m_loc, array_name, binding.id, element_ty.value(), std::move(index_expr),
        std::move(value_expr));
  }

  void AIRBuilder::visit(ast::FunctionCall *node)
  {
    const std::string &func_name = node->m_func_name->m_name;

    auto func_opt = symbol_table.lookup_function(func_name);
    if (!func_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
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
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc, msg.str());
    }

    std::vector<air::ExprPtr> args;
    for (size_t i = 0; i < node->m_arguments.size(); ++i)
    {
      auto arg = lower_expr(node->m_arguments[i].get());
      if (!arg)
      {
        continue;
      }

      if (i < func_symbol.param_types.size())
      {
        TyId expected_ty = func_symbol.param_types[i];
        TyId actual_ty = arg->m_ty;

        check_types_compatible(expected_ty, actual_ty,
                               node->m_arguments[i]->m_loc,
                               "function argument");
      }

      args.emplace_back(std::move(arg));
    }

    current_expr = std::make_unique<air::Call>(node->m_loc, func_name, func_symbol.id,
                                               std::move(args), func_symbol.return_type);
  }

  void AIRBuilder::visit(ast::ReturnStatement *node)
  {
    air::ExprPtr value;
    TyId return_ty = TyIds::VOID;

    if (node->m_expression)
    {
      value = lower_expr(node->m_expression.get());
      if (value)
      {
        return_ty = value->m_ty;
      }
    }

    check_types_compatible(current_function_return_type, return_ty,
                           node->m_loc, "return statement");

    current_stmt = std::make_unique<air::Return>(node->m_loc, std::move(value));
  }

  void AIRBuilder::visit(ast::IfStatement *node)
  {
    auto condition = lower_expr(node->m_condition.get());
    if (!condition)
    {
      current_stmt.reset();
      return;
    }

    if (condition->m_ty != TyIds::BOOL)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_condition->m_loc,
                        "If condition must be of type bool");
    }

    std::vector<air::StmtPtr> then_branch;
    if (node->m_then_branch)
    {
      then_branch = lower_block(node->m_then_branch.get());
    }

    std::vector<air::StmtPtr> else_branch;
    if (node->has_else_branch() && node->m_else_branch)
    {
      else_branch = lower_block(node->m_else_branch.get());
    }

    current_stmt = std::make_unique<air::If>(node->m_loc, std::move(condition),
                                             std::move(then_branch), std::move(else_branch));
  }

  void AIRBuilder::visit(ast::MatchStatement *node)
  {
    auto scrutinee = lower_expr(node->m_scrutinee.get());
    if (!scrutinee)
    {
      current_stmt.reset();
      return;
    }

    if (!ty_table.is_enum(scrutinee->m_ty))
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_scrutinee->m_loc,
                        "Match expression must be an enum");
      current_stmt.reset();
      return;
    }

    const TyInfo *enum_ty = ty_table.get_ty_info(scrutinee->m_ty);
    std::string enum_name = enum_ty ? enum_ty->m_name : "";
    bool has_wildcard = false;
    std::unordered_set<std::string> covered_variants;
    std::vector<air::MatchArm> arms;

    for (size_t i = 0; i < node->m_arms.size(); ++i)
    {
      const auto &arm = node->m_arms[i];
      if (has_wildcard)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, arm.m_loc,
                          "Wildcard match arm must be the last arm");
      }

      auto body = lower_block(arm.m_body.get());

      if (arm.m_is_wildcard)
      {
        has_wildcard = true;
        arms.emplace_back(std::move(body), arm.m_loc);
        continue;
      }

      auto variant_opt = symbol_table.lookup_enum_variant(arm.m_enum_name, arm.m_variant_name);
      if (!variant_opt.has_value())
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, arm.m_loc,
                          "Unknown enum variant '" + arm.m_enum_name + "::" +
                              arm.m_variant_name + "'");
        continue;
      }

      const EnumVariantSymbol &variant = variant_opt.value();
      if (variant.enum_type_id != scrutinee->m_ty)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, arm.m_loc,
                          "Match arm variant '" + arm.m_enum_name + "::" +
                              arm.m_variant_name + "' does not belong to enum '" +
                              enum_name + "'");
      }

      if (!covered_variants.insert(variant.variant_name).second)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, arm.m_loc,
                          "Duplicate match arm for variant '" + arm.m_enum_name +
                              "::" + arm.m_variant_name + "'");
      }

      arms.emplace_back(variant.enum_name, variant.variant_name, variant.value,
                        std::move(body), arm.m_loc);
    }

    if (!has_wildcard)
    {
      auto variants = symbol_table.get_enum_variants(enum_name);
      if (covered_variants.size() != variants.size())
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                          "Match statement must cover all variants of enum '" +
                              enum_name + "' or include '_'");
      }
    }

    current_stmt = std::make_unique<air::Match>(node->m_loc, std::move(scrutinee),
                                                std::move(arms));
  }

  void AIRBuilder::visit(ast::BreakStatement *node)
  {
    if (loop_depth == 0)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "'break' can only be used inside a loop");
    }

    current_stmt = std::make_unique<air::Break>(node->m_loc);
  }

  void AIRBuilder::visit(ast::ContinueStatement *node)
  {
    if (loop_depth == 0)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "'continue' can only be used inside a loop");
    }

    current_stmt = std::make_unique<air::Continue>(node->m_loc);
  }

  void AIRBuilder::visit(ast::WhileLoop *node)
  {
    auto condition = lower_expr(node->m_condition.get());
    if (!condition)
    {
      current_stmt.reset();
      return;
    }

    if (condition->m_ty != TyIds::BOOL)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_condition->m_loc,
                        "While condition must be of type bool");
    }

    ++loop_depth;
    std::vector<air::StmtPtr> body;
    if (node->m_body)
    {
      body = lower_block(node->m_body.get());
    }
    --loop_depth;

    current_stmt = std::make_unique<air::While>(node->m_loc, std::move(condition),
                                                std::move(body));
  }

  void AIRBuilder::visit(ast::ForLoop *node)
  {
    diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                      "For loops not yet supported in AIR lowering");
    current_stmt.reset();
  }

  void AIRBuilder::visit(ast::Function *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(ast::StructDecl *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(ast::EnumDecl *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(ast::ExternTypeDecl *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(ast::StructInstantiation *node)
  {
    const std::string &struct_name = node->m_struct_name;

    const ResolvedStruct *resolved = lookup_resolved_struct(struct_name);
    if (!resolved)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Undefined struct '" + struct_name + "'");
      current_expr.reset();
      return;
    }

    std::unordered_map<std::string, size_t> field_indices;
    for (size_t i = 0; i < resolved->fields.size(); ++i)
    {
      field_indices[resolved->fields[i].name] = i;
    }

    std::unordered_set<std::string> seen_fields;
    std::vector<air::ExprPtr> field_values(resolved->fields.size());
    bool has_field_error = false;

    for (const auto &field_value : node->m_field_values)
    {
      auto value = lower_expr(field_value.m_value.get());
      if (!value)
      {
        has_field_error = true;
        continue;
      }

      const std::string &field_name = field_value.m_name;
      auto index_it = field_indices.find(field_name);
      if (index_it == field_indices.end())
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, field_value.m_value->m_loc,
                          "Struct '" + struct_name + "' has no field '" +
                              field_name + "'");
        has_field_error = true;
        continue;
      }

      if (!seen_fields.insert(field_name).second)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, field_value.m_value->m_loc,
                          "Duplicate initializer for field '" + field_name + "'");
        has_field_error = true;
        continue;
      }

      size_t field_index = index_it->second;
      TyId expected_ty = resolved->fields[field_index].type_id;
      check_types_compatible(expected_ty, value->m_ty,
                             field_value.m_value->m_loc,
                             "struct field");
      field_values[field_index] = std::move(value);
    }

    for (const auto &field : resolved->fields)
    {
      if (seen_fields.find(field.name) == seen_fields.end())
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                          "Missing initializer for field '" + field.name +
                              "' in struct '" + struct_name + "'");
        has_field_error = true;
      }
    }

    if (has_field_error)
    {
      current_expr.reset();
      return;
    }

    current_expr = std::make_unique<air::StructInstantiation>(node->m_loc, struct_name,
                                                              resolved->struct_id,
                                                              std::move(field_values), resolved->type_id);
  }

  void AIRBuilder::visit(ast::NewObjectExpression *node)
  {
    const std::string &struct_name = node->m_struct_name;

    const ResolvedStruct *resolved = lookup_resolved_struct(struct_name);
    if (!resolved)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Undefined struct '" + struct_name + "'");
      current_expr.reset();
      return;
    }

    auto arena = lower_expr(node->m_arena.get());
    if (!arena)
    {
      current_expr.reset();
      return;
    }

    TyId void_arena_ty = ty_table.register_ref(TyIds::VOID);
    std::optional<TyId> arena_handle_ty;
    if (auto arena_ty = ty_table.lookup_by_name("Arena"))
    {
      arena_handle_ty = ty_table.register_ref(arena_ty.value());
    }

    bool arena_ok = arena->m_ty == void_arena_ty ||
                    (arena_handle_ty.has_value() && arena->m_ty == arena_handle_ty.value());
    if (!arena_ok)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_arena->m_loc,
                        "Type mismatch in arena allocation: expected '&Arena', got '" +
                            ty_table.ty_name(arena->m_ty) + "'");
    }

    std::unordered_map<std::string, size_t> field_indices;
    for (size_t i = 0; i < resolved->fields.size(); ++i)
    {
      field_indices[resolved->fields[i].name] = i;
    }

    std::unordered_set<std::string> seen_fields;
    std::vector<air::ExprPtr> field_values(resolved->fields.size());
    bool has_field_error = false;

    for (const auto &field_value : node->m_field_values)
    {
      auto value = lower_expr(field_value.m_value.get());
      if (!value)
      {
        has_field_error = true;
        continue;
      }

      const std::string &field_name = field_value.m_name;
      auto index_it = field_indices.find(field_name);
      if (index_it == field_indices.end())
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, field_value.m_value->m_loc,
                          "Struct '" + struct_name + "' has no field '" +
                              field_name + "'");
        has_field_error = true;
        continue;
      }

      if (!seen_fields.insert(field_name).second)
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, field_value.m_value->m_loc,
                          "Duplicate initializer for field '" + field_name + "'");
        has_field_error = true;
        continue;
      }

      size_t field_index = index_it->second;
      TyId expected_ty = resolved->fields[field_index].type_id;
      check_types_compatible(expected_ty, value->m_ty,
                             field_value.m_value->m_loc,
                             "struct field");
      field_values[field_index] = std::move(value);
    }

    for (const auto &field : resolved->fields)
    {
      if (seen_fields.find(field.name) == seen_fields.end())
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                          "Missing initializer for field '" + field.name +
                              "' in struct '" + struct_name + "'");
        has_field_error = true;
      }
    }

    if (has_field_error)
    {
      current_expr.reset();
      return;
    }

    TyId ref_ty = ty_table.register_ref(resolved->type_id);
    current_expr = std::make_unique<air::NewObject>(node->m_loc, struct_name,
                                                    resolved->struct_id,
                                                    std::move(arena),
                                                    std::move(field_values),
                                                    ref_ty);
  }

  void AIRBuilder::visit(ast::StructFieldAccess *node)
  {
    auto struct_expr = lower_expr(node->m_struct_expr.get());
    if (!struct_expr)
    {
      current_expr.reset();
      return;
    }

    TyId struct_ty = struct_expr->m_ty;
    if (ty_table.is_ref(struct_ty))
    {
      auto pointee_ty = ty_table.get_ref_pointee_type(struct_ty);
      if (pointee_ty.has_value())
      {
        struct_ty = pointee_ty.value();
      }
    }

    if (!ty_table.is_struct(struct_ty))
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Field access requires struct type");
      current_expr = std::make_unique<air::FieldAccess>(node->m_loc,
                                                        std::move(struct_expr),
                                                        node->m_field_name, 0, TyIds::ERROR);
      return;
    }

    auto ty_info = ty_table.get_ty_info(struct_ty);
    if (!ty_info || !ty_info->m_struct_id.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Internal error: invalid struct type");
      current_expr = std::make_unique<air::FieldAccess>(node->m_loc,
                                                        std::move(struct_expr),
                                                        node->m_field_name, 0, TyIds::ERROR);
      return;
    }

    const ResolvedStruct *resolved = lookup_resolved_struct_by_id(ty_info->m_struct_id.value());
    if (!resolved)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Internal error: struct not resolved");
      current_expr = std::make_unique<air::FieldAccess>(node->m_loc,
                                                        std::move(struct_expr),
                                                        node->m_field_name, 0, TyIds::ERROR);
      return;
    }

    const std::string &field_name = node->m_field_name;
    uint32_t field_index = 0;
    TyId field_ty = TyIds::ERROR;
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
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Struct '" + resolved->name + "' has no field '" +
                            field_name + "'");
      current_expr = std::make_unique<air::FieldAccess>(node->m_loc,
                                                        std::move(struct_expr),
                                                        field_name, 0, TyIds::ERROR);
      return;
    }

    current_expr = std::make_unique<air::FieldAccess>(node->m_loc,
                                                      std::move(struct_expr),
                                                      field_name, field_index, field_ty);
  }

  void AIRBuilder::visit(ast::ArrayAccess *node)
  {
    auto array_expr = lower_expr(node->m_array_expr.get());
    if (!array_expr)
    {
      current_expr.reset();
      return;
    }

    auto index_expr = lower_expr(node->m_index_expr.get());
    if (!index_expr)
    {
      current_expr.reset();
      return;
    }

    if (!ty_table.is_array(array_expr->m_ty))
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Array access requires array type");
      current_expr = std::make_unique<air::ArrayAccess>(node->m_loc,
                                                        std::move(array_expr),
                                                        std::move(index_expr), TyIds::ERROR);
      return;
    }

    if (index_expr->m_ty != TyIds::INTEGER)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Array index must be of type integer");
      current_expr = std::make_unique<air::ArrayAccess>(node->m_loc,
                                                        std::move(array_expr),
                                                        std::move(index_expr), TyIds::ERROR);
      return;
    }

    auto element_ty = ty_table.get_array_element_type(array_expr->m_ty);
    if (!element_ty.has_value())
    {
      ALOHA_ICE(
          "Internal error: unable to get array element type" + node->m_loc.to_string());
      current_expr.reset();
      return;
    }

    current_expr = std::make_unique<air::ArrayAccess>(node->m_loc,
                                                      std::move(array_expr),
                                                      std::move(index_expr), element_ty.value());
  }

  void AIRBuilder::visit(ast::StructFieldAssignment *node)
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

    TyId struct_ty = struct_expr->m_ty;
    if (ty_table.is_ref(struct_ty))
    {
      auto pointee_ty = ty_table.get_ref_pointee_type(struct_ty);
      if (pointee_ty.has_value())
      {
        struct_ty = pointee_ty.value();
      }
    }

    if (!ty_table.is_struct(struct_ty))
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Field assignment requires struct type");
      current_stmt = std::make_unique<air::FieldAssignment>(node->m_loc,
                                                            std::move(struct_expr),
                                                            node->m_field_name, 0,
                                                            std::move(value_expr));
      return;
    }

    auto ty_info = ty_table.get_ty_info(struct_ty);
    if (!ty_info || !ty_info->m_struct_id.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Internal error: invalid struct type");
      current_stmt = std::make_unique<air::FieldAssignment>(node->m_loc,
                                                            std::move(struct_expr),
                                                            node->m_field_name, 0,
                                                            std::move(value_expr));
      return;
    }

    const ResolvedStruct *resolved = lookup_resolved_struct_by_id(ty_info->m_struct_id.value());
    if (!resolved)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Internal error: struct not resolved");
      current_stmt = std::make_unique<air::FieldAssignment>(node->m_loc,
                                                            std::move(struct_expr),
                                                            node->m_field_name, 0,
                                                            std::move(value_expr));
      return;
    }

    const std::string &field_name = node->m_field_name;
    uint32_t field_index = 0;
    TyId field_ty = TyIds::ERROR;
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
      diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                        "Struct '" + resolved->name + "' has no field '" +
                            field_name + "'");
    }
    else
    {
      check_types_compatible(field_ty, value_expr->m_ty,
                             node->m_loc, "field assignment");
    }

    current_stmt = std::make_unique<air::FieldAssignment>(node->m_loc,
                                                          std::move(struct_expr),
                                                          field_name, field_index,
                                                          std::move(value_expr));
  }

  void AIRBuilder::visit(ast::Array *node)
  {
    auto array_elements = std::vector<air::ExprPtr>{};
    for (const auto &member : node->m_members)
    {
      auto elem = lower_expr(member.get());
      if (!elem)
      {
        current_expr.reset();
        return;
      }
      array_elements.emplace_back(std::move(elem));
    }

    // Infer array type from first element
    TyId array_ty = TyIds::ERROR;
    if (!array_elements.empty())
    {
      TyId element_ty = array_elements[0]->m_ty;

      // Check all elements have the same type
      for (size_t i = 1; i < array_elements.size(); i++)
      {
        if (array_elements[i]->m_ty != element_ty)
        {
          diagnostics.error(DiagnosticPhase::AIRBuilding, node->m_loc,
                            "Array elements must have the same type");
          current_expr.reset();
          return;
        }
      }

      // Register array type
      array_ty = ty_table.register_array(element_ty);
    }

    current_expr = std::make_unique<air::ArrayExpr>(node->m_loc, std::move(array_elements), array_ty);
  }

  void AIRBuilder::visit(ast::ExpressionStatement *node)
  {
    auto expr = lower_expr(node->m_expr.get());
    if (!expr)
    {
      current_stmt.reset();
      return;
    }

    current_stmt = std::make_unique<air::ExprStmt>(node->m_loc, std::move(expr));
  }

  void AIRBuilder::visit(ast::StatementBlock *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(ast::Program *node)
  {
    (void)node;
  }

  void AIRBuilder::visit(ast::Import *node)
  {
    (void)node;
  }

  std::unique_ptr<air::Module> AIRBuilder::build(ast::Program *program)
  {
    if (!program)
    {
      ALOHA_ICE("Null program passed to AIRBuilder::build");
    }

    (void)resolved_functions;

    auto module = std::make_unique<air::Module>(Location(), "");

    for (const auto &node : program->m_nodes)
    {
      if (auto struct_decl = dynamic_cast<ast::StructDecl *>(node.get()))
      {
        auto air_struct = lower_struct(struct_decl);
        if (air_struct)
        {
          module->m_structs.emplace_back(std::move(air_struct));
        }
      }
    }

    for (const auto &node : program->m_nodes)
    {
      if (auto func = dynamic_cast<ast::Function *>(node.get()))
      {
        auto air_func = lower_function(func);
        if (air_func)
        {
          module->m_functions.emplace_back(std::move(air_func));
        }
      }
    }

    if (diagnostics.has_errors())
    {
      return nullptr;
    }

    return module;
  }

  air::StructDeclPtr AIRBuilder::lower_struct(ast::StructDecl *struct_decl)
  {
    const std::string &name = struct_decl->m_name;

    // look up resolved struct info
    const ResolvedStruct *resolved = lookup_resolved_struct(name);
    if (!resolved)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, struct_decl->m_loc,
                        "Internal error: struct '" + name + "' not resolved");
      return nullptr;
    }

    // create air struct declaration with field objects
    std::vector<air::Field> fields;
    for (size_t i = 0; i < resolved->fields.size(); ++i)
    {
      const auto &field = resolved->fields[i];
      fields.emplace_back(field.name, field.type_id, static_cast<uint32_t>(i), field.location);
    }

    return std::make_unique<air::StructDecl>(struct_decl->m_loc, name,
                                             resolved->struct_id, resolved->type_id, fields);
  }

  air::FunctionPtr AIRBuilder::lower_function(ast::Function *func)
  {
    const std::string &name = func->m_name->m_name;

    // look up resolved function info
    auto func_opt = symbol_table.lookup_function(name);
    if (!func_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, func->m_loc,
                        "Internal error: function '" + name + "' not in symbol table");
      return nullptr;
    }

    const FunctionSymbol &func_symbol = func_opt.value();

    variable_scopes.clear();
    push_scope();

    // set current function return type for return statement checking
    current_function_return_type = func_symbol.return_type;

    // register parameter types
    std::vector<air::Param> params;
    for (size_t i = 0; i < func->m_parameters.size(); ++i)
    {
      const auto &param = func->m_parameters[i];
      TyId param_ty = func_symbol.param_types[i];

      auto param_var_id = find_parameter_var_id(param.m_name, func->m_loc);
      if (!param_var_id.has_value())
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, func->m_loc,
                          "Internal error: parameter '" + param.m_name + "' has no VarId");
        param_var_id = 0;
      }

      // create air::Param with all required fields
      params.emplace_back(param.m_name, param_var_id.value(), param_ty, false, func->m_loc);

      register_variable(param.m_name, param_var_id.value(), param_ty, false);
    }

    // lower function body if not extern
    std::vector<air::StmtPtr> body;
    if (!func->m_is_extern && func->m_body)
    {
      if (func_symbol.return_type != TyIds::VOID &&
          !block_definitely_returns(func->m_body.get()))
      {
        diagnostics.error(DiagnosticPhase::AIRBuilding, func->m_loc,
                          "Function '" + name + "' must return a value on all paths");
      }

      body = lower_block(func->m_body.get());
    }

    return std::make_unique<air::Function>(func->m_loc, name, func_symbol.id,
                                           std::move(params), func_symbol.return_type,
                                           std::move(body), func->m_is_extern);
  }

  air::ExprPtr AIRBuilder::lower_expr(ast::Expression *expr)
  {
    if (!expr)
    {
      return nullptr;
    }

    current_expr.reset();
    expr->accept(*this);
    if (!current_expr)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, expr->m_loc, "Unknown expression type");
    }
    return std::move(current_expr);
  }

  air::StmtPtr AIRBuilder::lower_stmt(ast::Statement *stmt)
  {
    if (!stmt)
    {
      return nullptr;
    }

    current_stmt.reset();
    stmt->accept(*this);
    if (!current_stmt)
    {
      diagnostics.error(DiagnosticPhase::AIRBuilding, stmt->m_loc, "Unknown statement type");
    }
    return std::move(current_stmt);
  }

  std::vector<air::StmtPtr> AIRBuilder::lower_block(ast::StatementBlock *block)
  {
    std::vector<air::StmtPtr> stmts;

    push_scope();

    for (const auto &stmt : block->m_statements)
    {
      auto air_stmt = lower_stmt(stmt.get());
      if (air_stmt)
      {
        stmts.emplace_back(std::move(air_stmt));
      }
    }

    pop_scope();

    return stmts;
  }

  bool AIRBuilder::check_types_compatible(TyId expected, TyId actual,
                                          Location loc, const std::string &context)
  {
    if (expected == actual)
    {
      return true;
    }

    if (actual == TyIds::NULL_TY && ty_table.is_ref(expected))
    {
      return true;
    }

    // error type is always compatible to avoid cascading errors
    if (expected == TyIds::ERROR || actual == TyIds::ERROR)
    {
      return true;
    }

    std::ostringstream msg;
    msg << "Type mismatch in " << context << ": expected '"
        << ty_table.ty_name(expected) << "', got '"
        << ty_table.ty_name(actual) << "'";
    diagnostics.error(DiagnosticPhase::AIRBuilding, loc, msg.str());
    return false;
  }

  air::BinaryOpKind AIRBuilder::ast_op_to_air_binop(const ast::Operator::Binary &op)
  {
    switch (op)
    {
    case ast::Operator::Binary::PLUS:
      return air::BinaryOpKind::ADD;
    case ast::Operator::Binary::MINUS:
      return air::BinaryOpKind::SUB;
    case ast::Operator::Binary::MULTIPLY:
      return air::BinaryOpKind::MUL;
    case ast::Operator::Binary::DIVIDE:
      return air::BinaryOpKind::DIV;
    case ast::Operator::Binary::MODULO:
      return air::BinaryOpKind::MOD;
    case ast::Operator::Binary::EQUAL:
      return air::BinaryOpKind::EQ;
    case ast::Operator::Binary::NOT_EQUAL:
      return air::BinaryOpKind::NE;
    case ast::Operator::Binary::LESS:
      return air::BinaryOpKind::LT;
    case ast::Operator::Binary::LESS_EQUAL:
      return air::BinaryOpKind::LE;
    case ast::Operator::Binary::GREATER:
      return air::BinaryOpKind::GT;
    case ast::Operator::Binary::GREATER_EQUAL:
      return air::BinaryOpKind::GE;
    case ast::Operator::Binary::LOGICAL_AND:
      return air::BinaryOpKind::LOGICAL_AND;
    case ast::Operator::Binary::LOGICAL_OR:
      return air::BinaryOpKind::LOGICAL_OR;
    default:
      ALOHA_UNREACHABLE();
    }
  }

  air::UnaryOpKind AIRBuilder::ast_op_to_air_unop(const ast::Operator::Unary &op)
  {
    switch (op)
    {
    case ast::Operator::Unary::NEGATE:
      return air::UnaryOpKind::NEG;
    case ast::Operator::Unary::NOT:
      return air::UnaryOpKind::NOT;
    default:
      ALOHA_UNREACHABLE();
    }
  }

  bool AIRBuilder::is_arithmetic_op(air::BinaryOpKind op)
  {
    return op == air::BinaryOpKind::ADD ||
           op == air::BinaryOpKind::SUB ||
           op == air::BinaryOpKind::MUL ||
           op == air::BinaryOpKind::DIV ||
           op == air::BinaryOpKind::MOD;
  }

  bool AIRBuilder::is_comparison_op(air::BinaryOpKind op)
  {
    return op == air::BinaryOpKind::EQ ||
           op == air::BinaryOpKind::NE ||
           op == air::BinaryOpKind::LT ||
           op == air::BinaryOpKind::LE ||
           op == air::BinaryOpKind::GT ||
           op == air::BinaryOpKind::GE;
  }

  bool AIRBuilder::is_logical_op(air::BinaryOpKind op)
  {
    return op == air::BinaryOpKind::LOGICAL_AND ||
           op == air::BinaryOpKind::LOGICAL_OR;
  }

  bool AIRBuilder::block_definitely_returns(const ast::StatementBlock *block) const
  {
    if (!block)
    {
      return false;
    }

    for (const auto &stmt : block->m_statements)
    {
      if (stmt_definitely_returns(stmt.get()))
      {
        return true;
      }
    }

    return false;
  }

  bool AIRBuilder::stmt_definitely_returns(const ast::Statement *stmt) const
  {
    if (!stmt)
    {
      return false;
    }

    if (dynamic_cast<const ast::ReturnStatement *>(stmt) != nullptr)
    {
      return true;
    }

    if (const auto *if_stmt = dynamic_cast<const ast::IfStatement *>(stmt))
    {
      return if_stmt->m_then_branch &&
             if_stmt->m_else_branch &&
             block_definitely_returns(if_stmt->m_then_branch.get()) &&
             block_definitely_returns(if_stmt->m_else_branch.get());
    }

    if (const auto *match_stmt = dynamic_cast<const ast::MatchStatement *>(stmt))
    {
      if (match_stmt->m_arms.empty())
      {
        return false;
      }

      for (const auto &arm : match_stmt->m_arms)
      {
        if (!block_definitely_returns(arm.m_body.get()))
        {
          return false;
        }
      }
      return true;
    }

    return false;
  }

  void AIRBuilder::push_scope()
  {
    variable_scopes.emplace_back();
  }

  void AIRBuilder::pop_scope()
  {
    if (!variable_scopes.empty())
    {
      variable_scopes.pop_back();
    }
  }

  void AIRBuilder::register_variable(const std::string &name, VarId id, TyId type,
                                     bool is_mutable)
  {
    if (variable_scopes.empty())
    {
      push_scope();
    }

    variable_scopes.back()[name] = VarBinding{id, type, is_mutable};
  }

  std::optional<VarId> AIRBuilder::find_variable_id_for_declaration(const ast::Declaration *decl) const
  {
    for (const auto &[id, var_symbol] : symbol_table.variables)
    {
      if (var_symbol.name == decl->m_variable_name &&
          var_symbol.location.line == decl->m_loc.line &&
          var_symbol.location.col == decl->m_loc.col &&
          var_symbol.location.file_path == decl->m_loc.file_path)
      {
        return id;
      }
    }

    return std::nullopt;
  }

  std::optional<VarId> AIRBuilder::find_parameter_var_id(const std::string &name, Location loc) const
  {
    for (const auto &[id, var_symbol] : symbol_table.variables)
    {
      if (var_symbol.name == name &&
          var_symbol.location.line == loc.line &&
          var_symbol.location.col == loc.col &&
          var_symbol.location.file_path == loc.file_path)
      {
        return id;
      }
    }

    return std::nullopt;
  }

  std::optional<AIRBuilder::VarBinding> AIRBuilder::lookup_variable(const std::string &name)
  {
    for (auto it = variable_scopes.rbegin(); it != variable_scopes.rend(); ++it)
    {
      auto binding = it->find(name);
      if (binding != it->end())
      {
        return binding->second;
      }
    }

    return std::nullopt;
  }

  std::optional<TyId> AIRBuilder::lookup_variable_type(const std::string &name)
  {
    auto binding = lookup_variable(name);
    if (binding.has_value())
    {
      return binding->type;
    }

    return std::nullopt;
  }

  std::optional<VarId> AIRBuilder::lookup_variable_id(const std::string &name)
  {
    auto binding = lookup_variable(name);
    if (binding.has_value())
    {
      return binding->id;
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

  const ResolvedStruct *AIRBuilder::lookup_resolved_struct_by_id(StructId struct_id)
  {
    auto it = resolved_structs.find(struct_id);
    if (it != resolved_structs.end())
    {
      return &it->second;
    }
    return nullptr;
  }

} // namespace aloha
