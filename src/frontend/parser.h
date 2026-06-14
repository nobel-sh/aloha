#ifndef PARSER_H_
#define PARSER_H_

#include "../ast/ast.h"
#include "../error/diagnostic_engine.h"
#include "../ast/ty_spec.h"
#include "lexer.h"
#include "token.h"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace aloha
{
  using ParseTy = TySpecId;
  class Parser
  {
  public:
    using prefix_parser_func =
        std::function<std::unique_ptr<ast::Expression>(Parser &)>;
    using infix_parser_func = std::function<std::unique_ptr<ast::Expression>(
        Parser &, std::unique_ptr<ast::Expression>, const Token &)>;

    explicit Parser(Lexer &lexer, TySpecArena &arena, DiagnosticEngine &diag);

    std::unique_ptr<ast::Program> parse();
    void dump(ast::Program *p, const TySpecArena &arena) const;
    bool has_errors() const;

    // exposed for testing
    std::unique_ptr<ast::StatementBlock> parse_statements();
    std::unique_ptr<ast::Statement> parse_statement();
    std::unique_ptr<ast::Expression> parse_expression(int min_precedence);

  private:
    struct FunctionSignature
    {
      std::unique_ptr<ast::Identifier> identifier;
      std::vector<ast::Parameter> parameters;
      ParseTy return_type;
      std::string return_type_name;
    };

    Lexer *lexer;
    Token current_token;
    Token next_token;
    DiagnosticEngine &diagnostics;
    TySpecArena *type_arena; // Points to Program's type_arena during parsing
    void report_error(const std::string &message);
    static std::map<std::string, prefix_parser_func> prefix_parsers;
    static std::map<std::string, infix_parser_func> infix_parsers;

  private:
    [[nodiscard]] bool is_eof() const;
    void advance();
    [[nodiscard]] std::optional<Token> peek() const;
    [[nodiscard]] std::optional<Token> next() const;
    // template <typename T>
    // [[nodiscard]] bool match(const T &value, bool use_next = false);
    [[nodiscard]] bool match(std::string_view value, bool use_next = false);
    [[nodiscard]] bool match(TokenKind value, bool use_next = false);
    template <typename T>
    void consume(const T &value, std::string message);
    [[nodiscard]] std::optional<Token> get_token(bool use_next) const;
    [[nodiscard]] bool is_synchronization_boundary();
    void synchronize();
    [[nodiscard]] std::optional<ParseTy> optional_type();
    ParseTy parse_type();
    Location current_location() const;

    std::unique_ptr<ast::Identifier> expect_identifier();
    std::optional<ast::QualifiedPath> parse_qualified_path();
    bool is_reserved_ident() const;
    bool is_reserved_ident(Token t) const;

    ast::NodePtr parse_top_level_declaration();
    FunctionSignature parse_function_signature();
    std::unique_ptr<ast::Function> parse_function(bool is_public = false);
    std::unique_ptr<ast::Function> parse_extern_function(bool is_public = false);
    std::unique_ptr<ast::Statement> parse_extern_type_decl(bool is_public = false);
    std::unique_ptr<ast::Import> parse_import();

    std::vector<ast::Parameter> parse_parameters();
    std::vector<ast::StructField> parse_struct_field();
    std::vector<std::string> parse_enum_variants();
    std::vector<ast::StructInstantiation::FieldValue>
    parse_named_field_values(const std::string &unnamed_field_message);

    std::unique_ptr<ast::Statement> parse_struct_decl(bool is_public = false);
    std::unique_ptr<ast::Statement> parse_enum_decl(bool is_public = false);
    std::unique_ptr<ast::Statement> parse_struct_field_assignment();
    std::unique_ptr<ast::Statement> parse_variable_declaration();
    std::unique_ptr<ast::Statement> parse_variable_assignment();
    std::unique_ptr<ast::Statement> parse_array_assignment();
    std::unique_ptr<ast::Statement> parse_return_statement();
    std::unique_ptr<ast::Statement> parse_break_statement();
    std::unique_ptr<ast::Statement> parse_continue_statement();
    std::unique_ptr<ast::Statement> parse_if_statement();
    std::unique_ptr<ast::Statement> parse_match_statement();
    std::unique_ptr<ast::Statement> parse_while_loop();
    std::unique_ptr<ast::Statement> parse_identifier_statement();
    bool statement_requires_semicolon(const ast::Statement *stmt) const;
    void consume_statement_semicolon();

    std::unique_ptr<ast::Expression> parse_struct_field_access();
    std::unique_ptr<ast::Expression> parse_struct_instantiation();
    std::unique_ptr<ast::Expression> parse_match_expression();
    std::unique_ptr<ast::Expression> parse_match_scrutinee();
    std::optional<ast::MatchPattern> parse_match_pattern();
    std::unique_ptr<ast::Expression> parse_new_object_expression();
    std::unique_ptr<ast::Expression> parse_array();

    std::unique_ptr<ast::Expression>
    parse_infix_expressions(std::unique_ptr<ast::Expression> left,
                            int min_precedence);
    std::unique_ptr<ast::Expression> parse_primary();
    std::unique_ptr<ast::Expression> parse_identifier_primary(
        Location loc, const Token &token);
    std::unique_ptr<ast::Expression> parse_function_call();
    std::unique_ptr<ast::Expression> parse_function_call(ast::QualifiedPath path);
    std::unique_ptr<ast::Statement> parse_expression_statement();

    std::unique_ptr<ast::Expression> parse_array_access();
  };
} // namespace aloha

#endif // PARSER_H_
