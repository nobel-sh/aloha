#include "symbol_binder.h"
#include <iostream>

namespace aloha
{

  bool SymbolBinder::bind(Program *program, TySpecArena &type_arena)
  {
    if (!program)
    {
      errors.add_error(Location(0, 0), "Null program passed to SymbolBinder");
      return false;
    }

    bind_declarations(program, type_arena);

    if (!errors.has_errors())
    {
      bind_function_bodies(program);
    }

    if (errors.has_errors())
    {
      errors.print();
      return false;
    }

    return true;
  }

  void SymbolBinder::bind_declarations(Program *program, const TySpecArena &type_arena)
  {
    for (const auto &node : program->m_nodes)
    {
      if (auto struct_decl = dynamic_cast<StructDecl *>(node.get()))
      {
        bind_struct_declaration(struct_decl);
      }
      else if (auto func = dynamic_cast<Function *>(node.get()))
      {
        bind_function_declaration(func, type_arena);
      }
    }
  }

  void SymbolBinder::bind_struct_declaration(StructDecl *struct_decl)
  {
    const std::string &name = struct_decl->m_name;
    Location loc = struct_decl->get_location();

    if (check_duplicate_struct(name, loc))
    {
      return;
    }

    AIR::StructId struct_id = ty_table.allocate_struct_id();
    AIR::TyId type_id = ty_table.register_struct(name, struct_id);

    symbol_table_ptr->register_struct(name, struct_id, type_id, loc);
  }

  void SymbolBinder::bind_function_declaration(Function *func, const TySpecArena &type_arena)
  {
    const std::string &name = func->m_name->m_name;
    Location loc = func->get_location();

    if (check_duplicate_function(name, loc))
    {
      return;
    }

    FunctionId func_id = symbol_table_ptr->allocate_func_id();

    std::vector<AIR::TyId> param_types;
    for (const auto &param : func->m_parameters)
    {
      auto param_ty_opt = ty_table.lookup_by_name(type_arena.to_string(param.m_type));
      if (!param_ty_opt.has_value())
      {
        errors.add_error(loc, "Unknown parameter type: " + type_arena.to_string(param.m_type));
        param_types.push_back(AIR::TyIds::ERROR);
      }
      else
      {
        param_types.push_back(param_ty_opt.value());
      }
    }

    auto return_ty_opt = ty_table.lookup_by_name(type_arena.to_string(func->m_return_type));
    AIR::TyId return_ty = AIR::TyIds::ERROR;
    if (!return_ty_opt.has_value())
    {
      errors.add_error(loc, "Unknown return type: " + type_arena.to_string(func->m_return_type));
    }
    else
    {
      return_ty = return_ty_opt.value();
    }

    symbol_table_ptr->register_function(func_id, name, return_ty, param_types,
                                        func->m_is_extern, loc);
  }

  void SymbolBinder::bind_function_bodies(Program *program)
  {
    for (const auto &node : program->m_nodes)
    {
      if (auto func = dynamic_cast<Function *>(node.get()))
      {
        bind_function_body(func);
      }
    }
  }

  void SymbolBinder::bind_function_body(Function *func)
  {
    Scope function_scope(nullptr);
    current_scope = &function_scope;

    for (const auto &param : func->m_parameters)
    {
      VarId var_id = symbol_table_ptr->allocate_var_id();
      Location loc = func->get_location(); // parameters don't have individual locations

      if (check_duplicate_variable(param.m_name, loc, current_scope))
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

  void SymbolBinder::bind_statement(Statement *stmt, Scope *scope)
  {
    if (!stmt)
      return;

    current_scope = scope;

    if (auto decl = dynamic_cast<Declaration *>(stmt))
    {
      const std::string &name = decl->m_variable_name;
      Location loc = decl->get_location();

      if (check_duplicate_variable(name, loc, scope))
      {
        return;
      }

      VarId var_id = symbol_table_ptr->allocate_var_id();

      symbol_table_ptr->register_variable(var_id, name, decl->m_is_mutable, loc);
      scope->add_variable(name, var_id);
    }
    else if (auto if_stmt = dynamic_cast<IfStatement *>(stmt))
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
    else if (auto while_loop = dynamic_cast<WhileLoop *>(stmt))
    {
      if (while_loop->m_body)
      {
        bind_statement_block(while_loop->m_body.get(), scope);
      }
    }
    else if (auto for_loop = dynamic_cast<ForLoop *>(stmt))
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

  void SymbolBinder::bind_statement_block(StatementBlock *block,
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
      errors.add_error(loc, "Duplicate function declaration: '" + name + "'");
      return true;
    }
    return false;
  }

  bool SymbolBinder::check_duplicate_struct(const std::string &name,
                                            Location loc)
  {
    if (symbol_table_ptr->lookup_struct(name).has_value())
    {
      errors.add_error(loc, "Duplicate struct declaration: '" + name + "'");
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
      errors.add_error(loc, "Duplicate variable declaration in same scope: '" +
                                name + "'");
      return true;
    }
    return false;
  }

} // namespace aloha