#include "codegen.h"
#include "ast.h"
#include "type.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_os_ostream.h>
#include <string>

CodeGen::CodeGen()
    : builder(context), currentFunction(nullptr), currentValue(nullptr) {
  module = std::make_unique<llvm::Module>("my_module", context);
}

bool CodeGen::generateCode(Program *program) {
  program->accept(*this);
  auto status = llvm::verifyModule(*module, &llvm::errs());
  dumpIR();
  return status;
}

void CodeGen::visit(Number *node) {
  auto num = std::stod(node->value);
  currentValue = llvm::ConstantFP::get(context, llvm::APFloat(num));
}

void CodeGen::visit(Boolean *node) {
  auto value = node->value;
  currentValue = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), value);
}

void CodeGen::visit(UnaryExpression *node) {
  node->expr->accept(*this);
  if (node->op == "-") {
    currentValue = builder.CreateFNeg(currentValue, "negtmp");
  }
}

void CodeGen::visit(BinaryExpression *node) {
  node->left->accept(*this);
  llvm::Value *left = currentValue;
  node->right->accept(*this);
  llvm::Value *right = currentValue;

  if (node->op == "+") {
    currentValue = builder.CreateFAdd(left, right, "addtmp");
  } else if (node->op == "-") {
    currentValue = builder.CreateFSub(left, right, "subtmp");
  } else if (node->op == "*") {
    currentValue = builder.CreateFMul(left, right, "multmp");
  } else if (node->op == "/") {
    currentValue = builder.CreateFDiv(left, right, "divtmp");
  } else if (node->op == ">=") {
    currentValue = builder.CreateFCmpOGE(left, right, "gtetmpe");
  } else if (node->op == "<=") {
    currentValue = builder.CreateFCmpOLE(left, right, "ltetmp");
  } else if (node->op == "<") {
    currentValue = builder.CreateFCmpOLT(left, right, "lttmp");
  } else if (node->op == ">") {
    currentValue = builder.CreateFCmpOGT(left, right, "gttmp");
  } else if (node->op == "==") {
    currentValue = builder.CreateFCmpOEQ(left, right, "eqtmp");
  } else if (node->op == "!=") {
    currentValue = builder.CreateFCmpONE(left, right, "netmp");
  }
}

void CodeGen::visit(Identifier *node) {
  auto it = namedValues.find(node->name);
  if (it == namedValues.end()) {
    throw std::runtime_error("Unknown variable name: " + node->name);
  }

  auto *alloca = it->second;

  if (!alloca) {
    throw std::runtime_error("Variable has no value: " + node->name);
  }
  currentValue =
      builder.CreateLoad(alloca->getAllocatedType(), alloca, node->name);
}

void CodeGen::visit(Declaration *node) {
  llvm::Type *type = getLLVMType(node->type.value());
  llvm::AllocaInst *alloca =
      builder.CreateAlloca(type, nullptr, node->variableName);
  namedValues[node->variableName] = alloca;

  if (node->expression) {
    node->expression->accept(*this);
    builder.CreateStore(currentValue, alloca);
  }
}

void CodeGen::visit(FunctionCall *node) {
  llvm::Function *calleeF = module->getFunction(node->functionName);
  if (!calleeF) {
    throw std::runtime_error("Unknown function referenced: " +
                             node->functionName);
  }
  std::vector<llvm::Value *> argsV;
  for (unsigned i = 0, e = node->arguments.size(); i != e; ++i) {
    node->arguments[i]->accept(*this);
    argsV.push_back(currentValue);
  }
  currentValue = builder.CreateCall(calleeF, argsV, "calltmp");
}

void CodeGen::visit(ReturnStatement *node) {
  node->expression->accept(*this);
  builder.CreateRet(currentValue);

  // Ensure no more code is added to the function after a return statement.
  llvm::BasicBlock *unreachableBlock =
      llvm::BasicBlock::Create(context, "unreachable", currentFunction);
  builder.SetInsertPoint(unreachableBlock);
}

void CodeGen::visit(IfStatement *node) {
  llvm::Function *function = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *thenBB =
      llvm::BasicBlock::Create(context, "then", function);
  llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else");
  llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont");

  node->condition->accept(*this);
  llvm::Value *condValue = currentValue;
  builder.CreateCondBr(condValue, thenBB, elseBB);

  builder.SetInsertPoint(thenBB);
  node->thenBranch->accept(*this);
  if (!builder.GetInsertBlock()->getTerminator()) {
    builder.CreateBr(mergeBB);
  }

  function->insert(function->end(), elseBB);
  builder.SetInsertPoint(elseBB);
  if (node->elseBranch) {
    node->elseBranch->accept(*this);
  }
  if (!builder.GetInsertBlock()->getTerminator()) {
    builder.CreateBr(mergeBB);
  }

  function->insert(function->end(), mergeBB);
  builder.SetInsertPoint(mergeBB);
}

void CodeGen::visit(WhileLoop *node) {
  llvm::Function *function = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(context, "loopcond", function);
  llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "loopbody");
  llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "afterloop");

  builder.CreateBr(condBB);
  builder.SetInsertPoint(condBB);

  node->condition->accept(*this);
  llvm::Value *condValue = currentValue;
  builder.CreateCondBr(condValue, bodyBB, afterBB);

  function->insert(function->end(), bodyBB);

  builder.SetInsertPoint(bodyBB);

  node->body->accept(*this);
  builder.CreateBr(condBB);

  function->insert(function->end(), afterBB);
  builder.SetInsertPoint(afterBB);
}

void CodeGen::visit(ForLoop *node) {
  llvm::Function *function = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *preheaderBB = builder.GetInsertBlock();
  llvm::BasicBlock *loopBB =
      llvm::BasicBlock::Create(context, "loop", function);
  llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "afterloop");

  builder.CreateBr(loopBB);
  builder.SetInsertPoint(loopBB);

  node->initializer->accept(*this);
  node->condition->accept(*this);
  llvm::Value *condValue = currentValue;
  builder.CreateCondBr(condValue, loopBB, afterBB);

  for (auto &stmt : node->body) {
    stmt->accept(*this);
  }
  node->increment->accept(*this);
  builder.CreateBr(loopBB);

  afterBB->insertInto(function);
  builder.SetInsertPoint(afterBB);
}

void CodeGen::visit(Function *node) {
  std::vector<llvm::Type *> paramTypes;
  for (const auto &param : node->parameters) {
    paramTypes.push_back(getLLVMType(param.type));
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(getLLVMType(node->returnType), paramTypes, false);
  llvm::Function *function =
      llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                             node->name->name, module.get());

  unsigned idx = 0;
  for (auto &arg : function->args()) {
    arg.setName(node->parameters[idx++].name);
  }

  llvm::BasicBlock *basicBlock =
      llvm::BasicBlock::Create(context, "entry", function);
  builder.SetInsertPoint(basicBlock);

  namedValues.clear();
  for (auto &arg : function->args()) {
    llvm::AllocaInst *alloca =
        builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
    builder.CreateStore(&arg, alloca);
    namedValues[std::string(arg.getName())] = alloca;
  }

  currentFunction = function;
  node->body->accept(*this);

  if (node->returnType == AlohaType::Type::VOID) {
    builder.CreateRetVoid();
  } else if (!builder.GetInsertBlock()->getTerminator()) {
    builder.CreateRet(
        llvm::Constant::getNullValue(getLLVMType(node->returnType)));
  }

  llvm::verifyFunction(*function);
}

void CodeGen::visit(StatementList *node) {
  for (auto &stmt : node->statements) {
    stmt->accept(*this);
  }
}

void CodeGen::visit(Program *node) {
  for (auto &n : node->nodes) {
    n->accept(*this);
  }
}

llvm::Type *CodeGen::getLLVMType(AlohaType::Type type) {
  switch (type) {
  case AlohaType::Type::NUMBER:
    return llvm::Type::getDoubleTy(context);
  case AlohaType::Type::VOID:
    return llvm::Type::getVoidTy(context);
  case AlohaType::Type::BOOL:
    return llvm::Type::getInt1Ty(context);
  default:
    throw std::runtime_error(
        "Unknown type while converting from Aloha Type to LLVM Type");
  }
}

void CodeGen::dumpIR() const {
  module->print(llvm::outs(), nullptr);
  // dumpNamedValues();
}

void CodeGen::print_value() const { print_value(currentValue); }

void CodeGen::print_value(llvm::Value *value) const {
  std::string value_str;
  llvm::raw_string_ostream rso(value_str);
  value->print(rso);
  std::cout << rso.str() << std::endl;
}
void CodeGen::print_llvm_type(llvm::Type *type) const {
  std::string type_str;
  llvm::raw_string_ostream rso(type_str);
  type->print(rso);
  std::cout << rso.str() << std::endl;
}

void CodeGen::dumpNamedValues() const {
  std::cout << "DUMPING NAMED VALUES" << std::endl;
  for (const auto &pair : namedValues) {
    std::string name = pair.first;
    std::cout << "Variable: " << name << std::endl;
    std::cout << "Value: " << std::endl;
    print_value(pair.second);
    std::cout << "Type: ";
    // llvm::AllocaInst *ins = llvm::dyn_cast<llvm::AllocaInst>(pair.second);
    print_llvm_type(pair.second->getAllocatedType());
    std::cout << std::endl;
  }
}
