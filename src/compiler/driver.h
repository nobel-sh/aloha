#ifndef DRIVER_H_
#define DRIVER_H_

#include "../frontend/lexer.h"
#include "../frontend/parser.h"
#include "../ty/ty.h"
#include "../sema/symbol_binder.h"
#include "../modules/import_resolver.h"
#include "../sema/type_resolver.h"
#include "../air/builder.h"
#include "../codegen/codegen.h"

// forward declarations
namespace aloha
{
  class SymbolBinder;
  class ImportResolver;
  class TypeResolver;
  class AIRBuilder;
}
#include <memory>
#include <string>
#include <llvm/IR/Module.h>

namespace AlohaPipeline
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

    std::unique_ptr<Lexer> lexer;
    std::unique_ptr<Parser> parser;
    std::unique_ptr<AIR::TyTable> ty_table;
    std::unique_ptr<aloha::SymbolBinder> symbol_binder;
    std::unique_ptr<aloha::ImportResolver> import_resolver;
    std::unique_ptr<aloha::TypeResolver> type_resolver;
    std::unique_ptr<aloha::AIRBuilder> air_builder;
    std::unique_ptr<Codegen::CodeGenerator> codegen;

    std::unique_ptr<aloha::Program> ast;
    std::unique_ptr<AIR::Module> air_module;
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

} // namespace AlohaPipeline

#endif // DRIVER_H_