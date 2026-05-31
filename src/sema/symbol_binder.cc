#include "symbol_binder.h"
#include <iostream>
#include "../error/diagnostic.h"

namespace aloha
{

  bool SymbolBinder::bind(ast::Program *program, TySpecArena &type_arena)
  {
    if (!program)
    {
      ALOHA_ICE("Null program passed to SymbolBinder::bind");
    }

    bind_declarations(program, type_arena);

    if (!diagnostics.has_errors())
    {
      bind_function_bodies(program);
    }

    if (diagnostics.has_errors())
    {
      return false;
    }

    return true;
  }

  void SymbolBinder::bind_declarations(ast::Program *program, const TySpecArena &type_arena)
  {
    for (const auto &node : program->m_nodes)
    {
      if (auto struct_decl = dynamic_cast<ast::StructDecl *>(node.get()))
      {
        bind_struct_declaration(struct_decl);
      }
      else if (auto enum_decl = dynamic_cast<ast::EnumDecl *>(node.get()))
      {
        bind_enum_declaration(enum_decl);
      }
      else if (auto func = dynamic_cast<ast::Function *>(node.get()))
      {
        bind_function_declaration(func, type_arena);
      }
    }
  }

  void SymbolBinder::bind_struct_declaration(ast::StructDecl *struct_decl)
  {
    const std::string &name = struct_decl->m_name;
    Location loc = struct_decl->loc();

    if (check_duplicate_struct(name, loc))
    {
      return;
    }

    StructId struct_id = ty_table.allocate_struct_id();
    TyId type_id = ty_table.register_struct(name, struct_id);

    symbol_table_ptr->register_struct(name, struct_id, type_id, loc);
  }

  void SymbolBinder::bind_enum_declaration(ast::EnumDecl *enum_decl)
  {
    const std::string &name = enum_decl->m_name;
    Location loc = enum_decl->loc();

    if (check_duplicate_enum(name, loc))
    {
      return;
    }

    EnumId enum_id = ty_table.allocate_enum_id();
    TyId type_id = ty_table.register_enum(name, enum_id);
    symbol_table_ptr->register_enum(name, enum_id, type_id, loc);

    std::unordered_set<std::string> seen_variants;
    for (size_t i = 0; i < enum_decl->m_variants.size(); ++i)
    {
      const std::string &variant = enum_decl->m_variants[i];
      if (!seen_variants.insert(variant).second)
      {
        diagnostics.error(DiagnosticPhase::SymbolBinding, loc,
                          "Duplicate enum variant declaration: '" + name + "::" + variant + "'");
        continue;
      }
      symbol_table_ptr->register_enum_variant(name, variant, enum_id, type_id,
                                              static_cast<uint32_t>(i), loc);
    }
  }

  void SymbolBinder::bind_function_declaration(ast::Function *func, const TySpecArena &type_arena)
  {
    const std::string &name = func->m_name->m_name;
    Location loc = func->loc();

    if (check_duplicate_function(name, loc))
    {
      return;
    }

    FunctionId func_id = symbol_table_ptr->allocate_func_id();

    std::vector<TyId> param_types;
    for (const auto &param : func->m_parameters)
    {
      auto param_ty_opt = resolve_signature_type(param.m_type, type_arena, loc,
                                                 "parameter type");
      if (!param_ty_opt.has_value())
      {
        diagnostics.error(DiagnosticPhase::SymbolBinding, loc, "Unknown parameter type: " + type_arena.to_string(param.m_type));
        param_types.push_back(TyIds::ERROR);
      }
      else
      {
        param_types.push_back(param_ty_opt.value());
      }
    }

    auto return_ty_opt = resolve_signature_type(func->m_return_type, type_arena, loc,
                                                "return type");
    TyId return_ty = TyIds::ERROR;
    if (!return_ty_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, loc, "Unknown return type: " + type_arena.to_string(func->m_return_type));
    }
    else
    {
      return_ty = return_ty_opt.value();
    }

    symbol_table_ptr->register_function(func_id, name, return_ty, param_types,
                                        func->m_is_extern, loc);
  }

  std::optional<TyId> SymbolBinder::resolve_signature_type(TySpecId ty_spec_id,
                                                           const TySpecArena &type_arena,
                                                           Location loc,
                                                           const std::string &context)
  {
    if (ty_spec_id >= type_arena.nodes.size())
    {
      ALOHA_ICE("TySpecId out of bound in SymbolBinder::resolve_signature_type");
    }

    const TySpec &spec = type_arena[ty_spec_id];
    switch (spec.kind)
    {
    case TySpec::Kind::Builtin:
      switch (spec.builtin)
      {
      case TySpec::Builtin::Int:
        return TyIds::INTEGER;
      case TySpec::Builtin::Float:
        return TyIds::FLOAT;
      case TySpec::Builtin::String:
        return TyIds::STRING;
      case TySpec::Builtin::Bool:
        return TyIds::BOOL;
      case TySpec::Builtin::Void:
        return TyIds::VOID;
      }
      return std::nullopt;

    case TySpec::Kind::Named:
    {
      if (auto struct_opt = symbol_table_ptr->lookup_struct(spec.name))
      {
        return struct_opt->type_id;
      }
      if (auto enum_opt = symbol_table_ptr->lookup_enum(spec.name))
      {
        return enum_opt->type_id;
      }
      (void)loc;
      (void)context;
      return std::nullopt;
    }

    case TySpec::Kind::Array:
    {
      auto element_ty = resolve_signature_type(spec.element, type_arena, loc, context);
      if (!element_ty.has_value())
      {
        return std::nullopt;
      }
      return ty_table.register_array(element_ty.value());
    }

    case TySpec::Kind::Ref:
    {
      auto pointee_ty = resolve_signature_type(spec.element, type_arena, loc, context);
      if (!pointee_ty.has_value())
      {
        return std::nullopt;
      }
      return ty_table.register_ref(pointee_ty.value());
    }
    }

    return std::nullopt;
  }

  void SymbolBinder::bind_function_bodies(ast::Program *program)
  {
    for (const auto &node : program->m_nodes)
    {
      if (auto func = dynamic_cast<ast::Function *>(node.get()))
      {
        bind_function_body(func);
      }
    }
  }

  void SymbolBinder::bind_function_body(ast::Function *func)
  {
    Scope function_scope(nullptr);
    current_scope = &function_scope;

    for (const auto &param : func->m_parameters)
    {
      VarId var_id = symbol_table_ptr->allocate_var_id();
      Location loc = func->loc(); // parameters don't have individual locations

      if (check_duplicate_parameter(param.m_name, loc, current_scope))
      {
        continue;
      }

      symbol_table_ptr->register_variable(var_id, param.m_name, false, loc);
      current_scope->add_variable(param.m_name, var_id);
    }

    if (func->m_body && !func->m_is_extern)
    {
      bind_statement_block(func->m_body.get(), current_scope);
    }

    current_scope = nullptr;
  }

  void SymbolBinder::bind_statement(ast::Statement *stmt, Scope *scope)
  {
    if (!stmt)
      return;

    current_scope = scope;

    if (auto decl = dynamic_cast<ast::Declaration *>(stmt))
    {
      const std::string &name = decl->m_variable_name;
      Location loc = decl->loc();

      if (check_duplicate_variable(name, loc, scope))
      {
        return;
      }

      VarId var_id = symbol_table_ptr->allocate_var_id();

      symbol_table_ptr->register_variable(var_id, name, decl->m_is_mutable, loc);
      scope->add_variable(name, var_id);
    }
    else if (auto if_stmt = dynamic_cast<ast::IfStatement *>(stmt))
    {
      if (if_stmt->m_then_branch)
      {
        bind_statement_block(if_stmt->m_then_branch.get(), scope);
      }

      if (if_stmt->has_else_branch() && if_stmt->m_else_branch)
      {
        bind_statement_block(if_stmt->m_else_branch.get(), scope);
      }
    }
    else if (auto while_loop = dynamic_cast<ast::WhileLoop *>(stmt))
    {
      if (while_loop->m_body)
      {
        bind_statement_block(while_loop->m_body.get(), scope);
      }
    }
    else if (auto for_loop = dynamic_cast<ast::ForLoop *>(stmt))
    {
      Scope loop_scope(scope);

      if (for_loop->m_initializer)
      {
        bind_statement(for_loop->m_initializer.get(), &loop_scope);
      }

      for (const auto &body_stmt : for_loop->m_body)
      {
        bind_statement(body_stmt.get(), &loop_scope);
      }
    }
  }

  void SymbolBinder::bind_statement_block(ast::StatementBlock *block,
                                          Scope *parent_scope)
  {
    if (!block)
      return;

    Scope block_scope(parent_scope);

    for (const auto &stmt : block->m_statements)
    {
      bind_statement(stmt.get(), &block_scope);
    }
  }

  bool SymbolBinder::check_duplicate_function(const std::string &name,
                                              Location loc)
  {
    if (symbol_table_ptr->lookup_function(name).has_value())
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, loc, "Duplicate function declaration: '" + name + "'");
      return true;
    }
    return false;
  }

  bool SymbolBinder::check_duplicate_struct(const std::string &name,
                                            Location loc)
  {
    if (symbol_table_ptr->lookup_struct(name).has_value() ||
        symbol_table_ptr->lookup_enum(name).has_value())
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, loc, "Duplicate struct declaration: '" + name + "'");
      return true;
    }
    return false;
  }

  bool SymbolBinder::check_duplicate_enum(const std::string &name,
                                          Location loc)
  {
    if (symbol_table_ptr->lookup_enum(name).has_value() ||
        symbol_table_ptr->lookup_struct(name).has_value())
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, loc, "Duplicate enum declaration: '" + name + "'");
      return true;
    }
    return false;
  }

  bool SymbolBinder::check_duplicate_parameter(const std::string &name,
                                               Location loc, Scope *scope)
  {
    if (scope && scope->has_variable_local(name))
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, loc,
                        "Duplicate parameter declaration: '" + name + "'");
      return true;
    }
    return false;
  }

  bool SymbolBinder::check_duplicate_variable(const std::string &name,
                                              Location loc, Scope *scope)
  {
    // check only in the current scope (not parent scopes)
    // shadowing is allowed in nested scopes
    if (scope && scope->has_variable_local(name))
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, loc, "Duplicate variable declaration in same scope: '" + name + "'");
      return true;
    }
    return false;
  }

} // namespace aloha
