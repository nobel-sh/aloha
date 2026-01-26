#ifndef CODEGEN_CODEGEN_H_
#define CODEGEN_CODEGEN_H_

#include "../air/air.h"
#include "../air/expr.h"
#include "../air/stmt.h"
#include "../error/diagnostic_engine.h"
#include "../ty/ty.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace aloha
{
  class CodeGenerator : public air::AIRVisitor
  {
  private:
    // LLVM infrastructure
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    // Type system bridge
    TyTable &ty_table;

    aloha::DiagnosticEngine &diagnostics;

    // Type mapping: TyId -> LLVM Type*
    std::unordered_map<TyId, llvm::Type *> type_map;

    // Struct mapping: StructId -> LLVM StructType*
    std::unordered_map<StructId, llvm::StructType *> struct_map;

    // Function mapping: FunctionId -> LLVM Function*
    std::unordered_map<FunctionId, llvm::Function *> function_map;

    // Variable mapping: VarId -> LLVM AllocaInst*
    std::unordered_map<VarId, llvm::AllocaInst *> variable_map;

    // Current codegen state
    llvm::Value *current_value;       // Result of expression codegen
    llvm::Function *current_function; // Currently generating function
    air::Module *current_air_module;  // Current AIR module being processed

  public:
    explicit CodeGenerator(TyTable &ty_table, aloha::DiagnosticEngine &diag);
    ~CodeGenerator() = default;

    // entry point
    std::unique_ptr<llvm::Module> generate(air::Module *air_module);

    bool has_errors() const { return diagnostics.has_errors(); }

    llvm::Module *get_module() const { return module.get(); }

  private:
    void generate_types();
    llvm::Type *get_llvm_type(TyId ty_id);
    void generate_struct_types();

    void declare_functions();
    llvm::FunctionType *get_function_type(air::Function *func);
    void generate_main_wrapper();

    void generate_function_bodies();
    void generate_function(air::Function *func);

    llvm::AllocaInst *create_entry_block_alloca(llvm::Function *func,
                                                const std::string &var_name,
                                                llvm::Type *type);

    // Expressions
    void visit(air::IntegerLiteral *node) override;
    void visit(air::FloatLiteral *node) override;
    void visit(air::StringLiteral *node) override;
    void visit(air::BoolLiteral *node) override;
    void visit(air::VarRef *node) override;
    void visit(air::BinaryOp *node) override;
    void visit(air::UnaryOp *node) override;
    void visit(air::Call *node) override;
    void visit(air::StructInstantiation *node) override;
    void visit(air::FieldAccess *node) override;
    void visit(air::ArrayExpr *node) override;
    void visit(air::ArrayAccess *node) override;

    // Statements
    void visit(air::VarDecl *node) override;
    void visit(air::Assignment *node) override;
    void visit(air::FieldAssignment *node) override;
    void visit(air::Return *node) override;
    void visit(air::If *node) override;
    void visit(air::ExprStmt *node) override;

    // Top-level declarations
    void visit(air::Function *node) override;
    void visit(air::StructDecl *node) override;
    void visit(air::Module *node) override;

    void report_error(const std::string &message, const Location &location)
    {
      diagnostics.error(aloha::DiagnosticPhase::Codegen, location, message);
    }
  };

} // namespace aloha

#endif // CODEGEN_CODEGEN_H_
