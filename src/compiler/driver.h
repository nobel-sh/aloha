#ifndef DRIVER_H_
#define DRIVER_H_

#include "../frontend/lexer.h"
#include "../frontend/parser.h"
#include "../ast/ty_spec.h"
#include "../ty/ty.h"
#include "../error/diagnostic_engine.h"
#include "../sema/symbol_binder.h"
#include "../modules/import_resolver.h"
#include "../sema/type_resolver.h"
#include "../air/builder.h"
#include "../codegen/codegen.h"
#include <memory>
#include <string>
#include <llvm/IR/Module.h>

namespace aloha
{
  struct CompilerOptions
  {
    std::string input_file;
    std::string output_file;
    bool dump_ast = false;
    bool dump_air = false;
    bool dump_ir = false;
    bool emit_llvm = false;
    bool emit_object = true;
    bool emit_executable = true;
    bool enable_optimization = false;
    bool verbose = false;
  };

  class CompilerDriver
  {
  public:
    explicit CompilerDriver(const CompilerOptions &options);
    ~CompilerDriver();
    int compile();

    bool has_errors() const;
    void print_errors() const;

  private:
    CompilerOptions options;

    aloha::DiagnosticEngine diagnostics;
    aloha::TySpecArena type_arena; // shared type_spec arena

    std::unique_ptr<Lexer> lexer;
    std::unique_ptr<Parser> parser;
    std::unique_ptr<TyTable> ty_table;
    std::unique_ptr<SymbolBinder> symbol_binder;
    std::unique_ptr<ImportResolver> import_resolver;
    std::unique_ptr<TypeResolver> type_resolver;
    std::unique_ptr<AIRBuilder> air_builder;
    std::unique_ptr<CodeGenerator> codegen;

    std::unique_ptr<ast::Program> ast;
    std::unique_ptr<air::Module> air_module;
    std::unique_ptr<llvm::Module> llvm_module;

    bool has_compilation_errors;

    bool stage_parse();
    bool stage_symbol_binding();
    bool stage_import_resolution();
    bool stage_type_resolution();
    bool stage_air_building();
    bool stage_codegen();
    bool stage_optimize();
    bool stage_emit_llvm_ir();
    bool stage_emit_object();
    bool stage_link_executable();

    std::string get_base_name() const;
    std::string get_output_name(const std::string &extension) const;
    std::string get_stdlib_archive_path() const;
    void log(const std::string &message) const;
    void log_stage(const std::string &stage_name) const;
    void dump_ast() const;
    void dump_air() const;
    void dump_llvm_ir() const;
  };
} // namespace aloha

#endif // DRIVER_H_