#ifndef AIR_BUILDER_H_
#define AIR_BUILDER_H_

#include "../ty/ty.h"
#include "../ast/ast.h"
#include "../ast/visitor.h"
#include "../error/diagnostic_engine.h"
#include "air.h"
#include "expr.h"
#include "stmt.h"
#include "../frontend/location.h"
#include "../ast/operator.h"
#include "../sema/symbol_binder.h"
#include "../sema/type_resolver.h"
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace aloha
{

  class AIRBuilder : public ASTVisitor
  {
  private:
    TyTable &ty_table;
    SymbolTable &symbol_table;
    const std::unordered_map<StructId, ResolvedStruct> &resolved_structs;
    const std::unordered_map<FunctionId, ResolvedFunction> &resolved_functions;
    const TySpecArena &type_arena;
    TypeResolver &type_resolver;
    DiagnosticEngine &diagnostics;

    std::unordered_map<std::string, TyId> var_types; // variable name -> type
    std::unordered_map<std::string, VarId> var_ids;  // variable name -> varId for current scope
    TyId current_function_return_type;               // for checking return statements

    air::ExprPtr current_expr;
    air::StmtPtr current_stmt;

  public:
    AIRBuilder(TyTable &table,
               SymbolTable &symbols,
               const std::unordered_map<StructId, ResolvedStruct> &structs,
               const std::unordered_map<FunctionId, ResolvedFunction> &funcs,
               const TySpecArena &arena,
               TypeResolver &resolver,
               DiagnosticEngine &diag)
        : ty_table(table), symbol_table(symbols),
          resolved_structs(structs), resolved_functions(funcs),
          type_arena(arena), type_resolver(resolver), diagnostics(diag),
          current_function_return_type(TyIds::VOID) {}

    virtual ~AIRBuilder() = default;

    std::unique_ptr<air::Module> build(ast::Program *program);

    bool has_errors() const { return diagnostics.has_errors(); }

    void visit(ast::Integer *node) override;
    void visit(ast::Float *node) override;
    void visit(ast::Boolean *node) override;
    void visit(ast::String *node) override;
    void visit(ast::UnaryExpression *node) override;
    void visit(ast::BinaryExpression *node) override;
    void visit(ast::Identifier *node) override;
    void visit(ast::Declaration *node) override;
    void visit(ast::Assignment *node) override;
    void visit(ast::FunctionCall *node) override;
    void visit(ast::ReturnStatement *node) override;
    void visit(ast::IfStatement *node) override;
    void visit(ast::WhileLoop *node) override;
    void visit(ast::ForLoop *node) override;
    void visit(ast::Function *node) override;
    void visit(ast::StructDecl *node) override;
    void visit(ast::StructInstantiation *node) override;
    void visit(ast::StructFieldAccess *node) override;
    void visit(ast::StructFieldAssignment *node) override;
    void visit(ast::Array *node) override;
    void visit(ast::ArrayAccess *node) override;
    void visit(ast::ExpressionStatement *node) override;
    void visit(ast::StatementBlock *node) override;
    void visit(ast::Program *node) override;
    void visit(ast::Import *node) override;

  private:
    air::FunctionPtr lower_function(ast::Function *func);
    air::StructDeclPtr lower_struct(ast::StructDecl *struct_decl);

    air::ExprPtr lower_expr(ast::Expression *expr);
    air::StmtPtr lower_stmt(ast::Statement *stmt);
    std::vector<air::StmtPtr> lower_block(ast::StatementBlock *block);

    bool check_types_compatible(TyId expected, TyId actual, Location loc,
                                const std::string &context);
    air::BinaryOpKind ast_op_to_air_binop(const ast::Operator::Binary &op);
    air::UnaryOpKind ast_op_to_air_unop(const ast::Operator::Unary &op);
    bool is_arithmetic_op(air::BinaryOpKind op);
    bool is_comparison_op(air::BinaryOpKind op);
    bool is_logical_op(air::BinaryOpKind op);

    void register_variable(const std::string &name, TyId type);
    void register_variable_id(const std::string &name, VarId id);
    std::optional<TyId> lookup_variable_type(const std::string &name);
    std::optional<VarId> lookup_variable_id(const std::string &name);

    const ResolvedStruct *lookup_resolved_struct(const std::string &name);
    const ResolvedStruct *lookup_resolved_struct_by_id(StructId struct_id);
  };

} // namespace aloha

#endif // SEMA_AIR_BUILDER_H_
