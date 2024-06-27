#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <unordered_map>

class CodeGen : public ASTVisitor {
public:
  CodeGen();

  bool generate_code(Program *program);

  void dump_ir() const;
  void print_value() const;
  void print_value(llvm::Value *value) const;
  void print_llvm_type(llvm::Type *type) const;
  void dump_named_values() const;

  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;
  std::unordered_map<std::string, llvm::AllocaInst *> named_values;

  void add_builtin_fns();

private:
  llvm::Value *current_val;   // Current value during traversal
  llvm::Function *current_fn; // current function

  llvm::Type *get_llvm_type(AlohaType::Type type);

  void visit(Number *node) override;
  void visit(Boolean *node) override;
  void visit(AlohaString *node) override;
  void visit(ExpressionStatement *node) override;
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
  void visit(StatementList *node) override;
  void visit(Program *node) override;
};

#endif // CODEGEN_H
