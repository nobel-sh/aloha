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

  bool generateCode(Program *program);

  void visit(Number *node) override;
  void visit(Boolean *node) override;
  void visit(ExpressionStatement *node) override;
  void visit(UnaryExpression *node) override;
  void visit(BinaryExpression *node) override;
  void visit(Identifier *node) override;
  void visit(Declaration *node) override;
  void visit(FunctionCall *node) override;
  void visit(ReturnStatement *node) override;
  void visit(IfStatement *node) override;
  void visit(WhileLoop *node) override;
  void visit(ForLoop *node) override;
  void visit(Function *node) override;
  void visit(StatementList *node) override;
  void visit(Program *node) override;

  void dumpIR() const;
  void print_value() const;
  void print_value(llvm::Value *value) const;
  void print_llvm_type(llvm::Type *type) const;
  void dumpNamedValues() const;

  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;
  std::unordered_map<std::string, llvm::AllocaInst *> namedValues;

  void addBuiltinFunctions();

private:
  llvm::Value *currentValue;       // Current value during traversal
  llvm::Function *currentFunction; // Track the current function

  llvm::Type *
  getLLVMType(AlohaType::Type type); // Ensure this declaration exists
};

#endif // CODEGEN_H
