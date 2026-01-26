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
    [[nodiscard]] bool match(std::string value, bool use_next = false);
    [[nodiscard]] bool match(TokenKind value, bool use_next = false);
    template <typename T>
    void consume(const T &value, std::string message);
    [[nodiscard]] std::optional<Token> get_token(bool use_next) const;
    [[nodiscard]] std::optional<ParseTy> optional_type();
    ParseTy parse_type();
    Location current_location() const;

    void panic_parser(const std::string &message);

    std::unique_ptr<ast::Identifier> expect_identifier();
    bool is_reserved_ident() const;
    bool is_reserved_ident(Token t) const;

    std::unique_ptr<ast::Function> parse_function();
    std::unique_ptr<ast::Function> parse_extern_function();
    std::unique_ptr<ast::Import> parse_import();

    std::vector<ast::Parameter> parse_parameters();
    std::vector<ast::StructField> parse_struct_field();

    std::unique_ptr<ast::Statement> parse_struct_decl();
    std::unique_ptr<ast::Statement> parse_struct_field_assignment();
    std::unique_ptr<ast::Statement> parse_variable_declaration();
    std::unique_ptr<ast::Statement> parse_variable_assignment();
    std::unique_ptr<ast::Statement> parse_return_statement();
    std::unique_ptr<ast::Statement> parse_if_statement();
    std::unique_ptr<ast::Statement> parse_while_loop();

    std::unique_ptr<ast::Expression> parse_struct_field_access();
    std::unique_ptr<ast::Expression> parse_struct_instantiation();
    std::unique_ptr<ast::Expression> parse_array();

    std::unique_ptr<ast::Expression>
    parse_infix_expressions(std::unique_ptr<ast::Expression> left,
                            int min_precedence);
    std::unique_ptr<ast::Expression> parse_primary();
    std::unique_ptr<ast::Expression> parse_function_call();
    std::unique_ptr<ast::Statement> parse_expression_statement();

    std::unique_ptr<ast::Expression> parse_array_access();
  };
} // namespace aloha

#endif // PARSER_H_
