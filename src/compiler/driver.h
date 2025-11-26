#ifndef DRIVER_H_
#define DRIVER_H_

#include "../codegen.h"
#include "../lexer.h"
#include "../parser.h"
#include "../sema.h"
#include "../resolver/import.h"
#include <memory>
#include <string>

struct CompilerConfig
{
    std::string input_file;
    std::string file_name;
    bool dump_debug_info;
    bool enable_optimization;

    CompilerConfig()
        : dump_debug_info(false), enable_optimization(true) {}
};

class CompilerDriver
{
public:
    explicit CompilerDriver(const CompilerConfig &config);
    ~CompilerDriver();

    // entrypoint
    int compile();

    Lexer *get_lexer() { return lexer; }
    Parser *get_parser() { return parser; }
    SemanticAnalyzer *get_analyzer() { return &analyzer; }
    CodeGen *get_codegen() { return &codegen; }

    const CompilerConfig &get_config() const { return config; }

private:
    CompilerConfig config;

    Lexer *lexer;
    Parser *parser;
    SemanticAnalyzer analyzer;
    CodeGen codegen;
    Aloha::ImportResolver import_resolver;

    std::unique_ptr<aloha::Program> ast;

    bool parse_and_resolve_imports();
    bool analyze_semantics();
    bool generate_code();
    bool optimize_code();
    bool emit_object_file();
    bool link_executable();

    void print_separator() const;
    void dump_untyped_ast() const;
    void dump_typed_ast() const;
    void dump_unoptimized_ir() const;
    void dump_optimized_ir() const;
};

#endif // DRIVER_H_
