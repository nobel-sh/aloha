#ifndef AIR_BUILDER_H_
#define AIR_BUILDER_H_

#include "../ty/ty.h"
#include "../ast/ast.h"
#include "../ast/visitor.h"
#include "air.h"
#include "expr.h"
#include "stmt.h"
#include "../frontend/location.h"
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

  class AIRBuildError
  {
  private:
    std::vector<std::string> errors;

  public:
    void add_error(Location loc, const std::string &message)
    {
      errors.push_back("[" + std::to_string(loc.line) + ":" +
                       std::to_string(loc.col) + "] " + message);
    }

    bool has_errors() const { return !errors.empty(); }

    void print_errors() const
    {
      for (const auto &error : errors)
      {
        std::cerr << "AIR Build Error: " << error << std::endl;
      }
    }

    const std::vector<std::string> &get_errors() const { return errors; }
  };

  class AIRBuilder : public ASTVisitor
  {
  private:
    AIR::TyTable &ty_table;
    SymbolTable &symbol_table;
    const std::unordered_map<AIR::StructId, ResolvedStruct> &resolved_structs;
    const std::unordered_map<FunctionId, ResolvedFunction> &resolved_functions;
    AIRBuildError errors;

    std::unordered_map<std::string, AIR::TyId> var_types; // variable name -> type
    std::unordered_map<std::string, VarId> var_ids;       // variable name -> varId for current scope
    AIR::TyId current_function_return_type;               // for checking return statements

    AIR::ExprPtr current_expr;
    AIR::StmtPtr current_stmt;

  public:
    AIRBuilder(AIR::TyTable &table,
               SymbolTable &symbols,
               const std::unordered_map<AIR::StructId, ResolvedStruct> &structs,
               const std::unordered_map<FunctionId, ResolvedFunction> &funcs)
        : ty_table(table), symbol_table(symbols),
          resolved_structs(structs), resolved_functions(funcs),
          current_function_return_type(AIR::TyIds::VOID) {}

    virtual ~AIRBuilder() = default;

    std::unique_ptr<AIR::Module> build(Program *program);

    const AIRBuildError &get_errors() const { return errors; }

    void visit(Number *node) override;
    void visit(Boolean *node) override;
    void visit(String *node) override;
    void visit(UnaryExpression *node) override;
    void visit(BinaryExpression *node) override;
    void visit(Identifier *node) override;
    void visit(Declaration *node) override;
    void visit(Assignment *node) override;
    void visit(FunctionCall *node) override;
    void visit(ReturnStatement *node) override;
    void visit(IfStatement *node) override;
    void visit(WhileLoop *node) override;
    void visit(ForLoop *node) override;
    void visit(Function *node) override;
    void visit(StructDecl *node) override;
    void visit(StructInstantiation *node) override;
    void visit(StructFieldAccess *node) override;
    void visit(StructFieldAssignment *node) override;
    void visit(Array *node) override;
    void visit(ExpressionStatement *node) override;
    void visit(StatementBlock *node) override;
    void visit(Program *node) override;
    void visit(Import *node) override;

  private:
    AIR::FunctionPtr lower_function(Function *func);
    AIR::StructDeclPtr lower_struct(StructDecl *struct_decl);

    AIR::ExprPtr lower_expr(Expression *expr);
    AIR::StmtPtr lower_stmt(Statement *stmt);
    std::vector<AIR::StmtPtr> lower_block(StatementBlock *block);

    bool check_types_compatible(AIR::TyId expected, AIR::TyId actual, Location loc,
                                const std::string &context);
    AIR::BinaryOpKind ast_op_to_air_binop(const std::string &op);
    AIR::UnaryOpKind ast_op_to_air_unop(const std::string &op);
    bool is_arithmetic_op(AIR::BinaryOpKind op);
    bool is_comparison_op(AIR::BinaryOpKind op);
    bool is_logical_op(AIR::BinaryOpKind op);

    void register_variable(const std::string &name, AIR::TyId type);
    void register_variable_id(const std::string &name, VarId id);
    std::optional<AIR::TyId> lookup_variable_type(const std::string &name);
    std::optional<VarId> lookup_variable_id(const std::string &name);

    const ResolvedStruct *lookup_resolved_struct(const std::string &name);
    const ResolvedStruct *lookup_resolved_struct_by_id(AIR::StructId struct_id);
  };

} // namespace aloha

#endif // SEMA_AIR_BUILDER_H_
