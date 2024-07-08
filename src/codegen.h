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

  bool generate_code(Aloha::Program *program);

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

  void visit(Aloha::Number *node) override;
  void visit(Aloha::Boolean *node) override;
  void visit(Aloha::String *node) override;
  void visit(Aloha::ExpressionStatement *node) override;
  void visit(Aloha::UnaryExpression *node) override;
  void visit(Aloha::BinaryExpression *node) override;
  void visit(Aloha::Identifier *node) override;
  void visit(Aloha::Declaration *node) override;
  void visit(Aloha::Assignment *node) override;
  void visit(Aloha::FunctionCall *node) override;
  void visit(Aloha::ReturnStatement *node) override;
  void visit(Aloha::IfStatement *node) override;
  void visit(Aloha::WhileLoop *node) override;
  void visit(Aloha::ForLoop *node) override;
  void visit(Aloha::Function *node) override;
  void visit(Aloha::StructDecl *node) override;
  void visit(Aloha::StructInstantiation *node) override;
  void visit(Aloha::StructFieldAccess *node) override;
  void visit(Aloha::StructFieldAssignment *node) override;
  void visit(Aloha::StatementList *node) override;
  void visit(Aloha::Program *node) override;
};

#endif // CODEGEN_H
