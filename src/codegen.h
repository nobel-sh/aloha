#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <unordered_map>

class CodeGen : public ASTVisitor {
public:
  CodeGen();

  bool generate_code(aloha::Program *program);

  void dump_ir() const;
  void print_value() const;
  void print_value(llvm::Value *value) const;
  void print_llvm_type(llvm::Type *type) const;
  void dump_named_values() const;
  void dump_struct_types() const;

  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;

private:
  void add_builtin_fns();
  std::unordered_map<std::string, llvm::AllocaInst *> named_values;
  std::unordered_map<std::string, llvm::StructType *> struct_types;
  std::unordered_map<AlohaType::Type, std::string> type_to_struct;
  llvm::Value *current_val;   // Current value during traversal
  llvm::Function *current_fn; // current function

  llvm::Type *get_llvm_type(AlohaType::Type type);

  void visit(aloha::Number *node) override;
  void visit(aloha::Boolean *node) override;
  void visit(aloha::String *node) override;
  void visit(aloha::ExpressionStatement *node) override;
  void visit(aloha::UnaryExpression *node) override;
  void visit(aloha::BinaryExpression *node) override;
  void visit(aloha::Identifier *node) override;
  void visit(aloha::Declaration *node) override;
  void visit(aloha::Assignment *node) override;
  void visit(aloha::FunctionCall *node) override;
  void visit(aloha::ReturnStatement *node) override;
  void visit(aloha::IfStatement *node) override;
  void visit(aloha::WhileLoop *node) override;
  void visit(aloha::ForLoop *node) override;
  void visit(aloha::Function *node) override;
  void visit(aloha::StructDecl *node) override;
  void visit(aloha::StructInstantiation *node) override;
  void visit(aloha::StructFieldAccess *node) override;
  void visit(aloha::StructFieldAssignment *node) override;
  void visit(aloha::Array *node) override;
  void visit(aloha::StatementBlock *node) override;
  void visit(aloha::Program *node) override;
};

#endif // CODEGEN_H
