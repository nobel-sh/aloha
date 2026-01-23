#ifndef CODEGEN_CODEGEN_H_
#define CODEGEN_CODEGEN_H_

#include "../air/air.h"
#include "../air/expr.h"
#include "../air/stmt.h"
#include "../error/compiler_error.h"
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

namespace Codegen
{
  using CodegenErrorReporter = Aloha::CodegenError;

  class CodeGenerator : public AIR::AIRVisitor
  {
  private:
    // LLVM infrastructure
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    // Type system bridge
    AIR::TyTable &ty_table;

    CodegenErrorReporter error_reporter;

    // Type mapping: TyId -> LLVM Type*
    std::unordered_map<AIR::TyId, llvm::Type *> type_map;

    // Struct mapping: StructId -> LLVM StructType*
    std::unordered_map<AIR::StructId, llvm::StructType *> struct_map;

    // Function mapping: FunctionId -> LLVM Function*
    std::unordered_map<AIR::FunctionId, llvm::Function *> function_map;

    // Variable mapping: VarId -> LLVM AllocaInst*
    std::unordered_map<AIR::VarId, llvm::AllocaInst *> variable_map;

    // Current codegen state
    llvm::Value *current_value;       // Result of expression codegen
    llvm::Function *current_function; // Currently generating function
    AIR::Module *current_air_module;  // Current AIR module being processed

  public:
    explicit CodeGenerator(AIR::TyTable &ty_table);
    ~CodeGenerator() = default;

    // entry point
    std::unique_ptr<llvm::Module> generate(AIR::Module *air_module);

    bool has_errors() const { return error_reporter.has_errors(); }
    const CodegenErrorReporter &get_error_reporter() const { return error_reporter; }

    llvm::Module *get_module() const { return module.get(); }

  private:
    void generate_types();
    llvm::Type *get_llvm_type(AIR::TyId ty_id);
    void generate_struct_types();

    void declare_functions();
    llvm::FunctionType *get_function_type(AIR::Function *func);

    void generate_function_bodies();
    void generate_function(AIR::Function *func);

    llvm::AllocaInst *create_entry_block_alloca(llvm::Function *func,
                                                const std::string &var_name,
                                                llvm::Type *type);

    // Expressions
    void visit(AIR::IntegerLiteral *node) override;
    void visit(AIR::FloatLiteral *node) override;
    void visit(AIR::StringLiteral *node) override;
    void visit(AIR::BoolLiteral *node) override;
    void visit(AIR::VarRef *node) override;
    void visit(AIR::BinaryOp *node) override;
    void visit(AIR::UnaryOp *node) override;
    void visit(AIR::Call *node) override;
    void visit(AIR::StructInstantiation *node) override;
    void visit(AIR::FieldAccess *node) override;

    // Statements
    void visit(AIR::VarDecl *node) override;
    void visit(AIR::Assignment *node) override;
    void visit(AIR::FieldAssignment *node) override;
    void visit(AIR::Return *node) override;
    void visit(AIR::If *node) override;
    void visit(AIR::ExprStmt *node) override;

    // Top-level declarations
    void visit(AIR::Function *node) override;
    void visit(AIR::StructDecl *node) override;
    void visit(AIR::Module *node) override;

    void report_error(const std::string &message, const Location &location)
    {
      error_reporter.add_error(location, message);
    }
  };

} // namespace Codegen

#endif // CODEGEN_CODEGEN_H_
