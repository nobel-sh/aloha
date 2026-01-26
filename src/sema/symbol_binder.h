#ifndef SEMA_SYMBOL_BINDER_H_
#define SEMA_SYMBOL_BINDER_H_

#include "symbol_table.h"
#include "../ty/ty.h"
#include "../ast/ast.h"
#include "../frontend/location.h"
#include "../error/diagnostic_engine.h"
#include "../ast/ty_spec.h"
#include <memory>

namespace aloha
{

  class SymbolBinder
  {
  private:
    AIR::TyTable &ty_table;
    SymbolTable symbol_table;
    SymbolTable *symbol_table_ptr; // allow using external symbol table for imports
    DiagnosticEngine &diagnostics;
    Scope *current_scope;

  public:
    explicit SymbolBinder(AIR::TyTable &table, DiagnosticEngine &diag)
        : ty_table(table), symbol_table_ptr(&symbol_table), diagnostics(diag), current_scope(nullptr) {}

    // set an external symbol table for import processing
    void set_symbol_table(SymbolTable *table)
    {
      symbol_table_ptr = table;
    }

    bool bind(Program *program, TySpecArena &type_arena);

    SymbolTable &get_symbol_table() { return *symbol_table_ptr; }
    const SymbolTable &get_symbol_table() const { return *symbol_table_ptr; }

    bool has_errors() const { return diagnostics.has_errors(); }

  private:
    // register all struct names and function names
    void bind_declarations(Program *program, const TySpecArena &type_arena);
    void bind_struct_declaration(StructDecl *struct_decl);
    void bind_function_declaration(Function *func, const TySpecArena &type_arena);

    // bind variables in function bodies
    void bind_function_bodies(Program *program);
    void bind_function_body(Function *func);
    void bind_statement(Statement *stmt, Scope *scope);
    void bind_statement_block(StatementBlock *block, Scope *parent_scope);

    bool check_duplicate_function(const std::string &name, Location loc);
    bool check_duplicate_struct(const std::string &name, Location loc);
    bool check_duplicate_variable(const std::string &name, Location loc,
                                  Scope *scope);
  };

} // namespace aloha

#endif // SEMA_SYMBOL_BINDER_H_