#ifndef MODULES_IMPORT_RESOLVER_H_
#define MODULES_IMPORT_RESOLVER_H_

#include "../ast/ast.h"
#include "../ty/ty.h"
#include "../frontend/lexer.h"
#include "../frontend/parser.h"
#include "../sema/symbol_binder.h"
#include "../error/diagnostic_engine.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>

namespace aloha
{

  class ImportResolver
  {
  public:
    ImportResolver(TyTable &ty_table,
                   SymbolTable &main_symbol_table,
                   aloha::TySpecArena &type_arena,
                   aloha::DiagnosticEngine &diag,
                   const std::string &current_file_path,
                   bool skip_prelude_injection = false);

    ~ImportResolver();

    bool resolve_imports(ast::Program *ast);

    bool inject_prelude();

    bool has_errors() const { return diagnostics.has_errors(); }

    const std::vector<std::string> &get_import_paths() const
    {
      return resolved_import_paths;
    }

    const std::vector<std::unique_ptr<ast::Program>> &get_imported_asts() const
    {
      return imported_asts;
    }

  private:
    TyTable &ty_table;
    SymbolTable &main_symbol_table;
    TySpecArena &type_arena;
    DiagnosticEngine &diagnostics;

    bool skip_prelude_injection;
    std::filesystem::path current_file_dir;
    std::vector<std::filesystem::path> search_paths;

    // circular import detection - shared across all nested resolvers
    std::unordered_set<std::string> *currently_importing;
    std::unordered_set<std::string> *already_imported;
    bool owns_import_sets; // true if this resolver owns the sets

    std::vector<std::string> resolved_import_paths;

    std::vector<std::unique_ptr<ast::Program>> imported_asts;

    bool resolve_import(ast::Import *import_node);

    std::string resolve_import_path(const std::string &import_path,
                                    const Location &loc);

    bool process_imported_file(const std::string &file_path,
                               const Location &import_loc);

    void initialize_search_paths();

    std::filesystem::path get_stdlib_path() const;

    std::string normalize_path(const std::filesystem::path &path) const;
  };

} // namespace aloha

#endif // MODULES_IMPORT_RESOLVER_H_
