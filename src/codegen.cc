#include "codegen.h"
#include "ast.h"
#include "type.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_os_ostream.h>
#include <string>

CodeGen::CodeGen()
    : builder(context), current_val(nullptr), current_fn(nullptr) {
  module = std::make_unique<llvm::Module>("my_module", context);
}

bool CodeGen::generate_code(Program *program) {
  add_builtin_fns();
  program->accept(*this);
  auto status = llvm::verifyModule(*module, &llvm::errs());
  return status;
}

void CodeGen::visit(Number *node) {
  auto num = std::stod(node->value);
  current_val = llvm::ConstantFP::get(context, llvm::APFloat(num));
}

void CodeGen::visit(Boolean *node) {
  auto value = node->value;
  current_val = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), value);
}

void CodeGen::visit(AlohaString *node) {
  auto const &str = node->value;
  current_val = builder.CreateGlobalStringPtr(str, "str");
}

void CodeGen::visit(ExpressionStatement *node) { node->expr->accept(*this); }

void CodeGen::visit(UnaryExpression *node) {
  node->expr->accept(*this);
  if (node->op == "-") {
    current_val = builder.CreateFNeg(current_val, "negtmp");
  }
}

void CodeGen::visit(BinaryExpression *node) {
  node->left->accept(*this);
  llvm::Value *left = current_val;
  node->right->accept(*this);
  llvm::Value *right = current_val;

  if (node->op == "+") {
    current_val = builder.CreateFAdd(left, right, "addtmp");
  } else if (node->op == "-") {
    current_val = builder.CreateFSub(left, right, "subtmp");
  } else if (node->op == "*") {
    current_val = builder.CreateFMul(left, right, "multmp");
  } else if (node->op == "/") {
    current_val = builder.CreateFDiv(left, right, "divtmp");
  } else if (node->op == ">=") {
    current_val = builder.CreateFCmpOGE(left, right, "gtetmpe");
  } else if (node->op == "<=") {
    current_val = builder.CreateFCmpOLE(left, right, "ltetmp");
  } else if (node->op == "<") {
    current_val = builder.CreateFCmpOLT(left, right, "lttmp");
  } else if (node->op == ">") {
    current_val = builder.CreateFCmpOGT(left, right, "gttmp");
  } else if (node->op == "==") {
    current_val = builder.CreateFCmpOEQ(left, right, "eqtmp");
  } else if (node->op == "!=") {
    current_val = builder.CreateFCmpONE(left, right, "netmp");
  }
}

void CodeGen::visit(Identifier *node) {
  auto itr = named_values.find(node->name);
  if (itr == named_values.end()) {
    throw std::runtime_error("Unknown variable name: " + node->name);
  }

  auto *alloca = itr->second;

  if (!alloca) {
    throw std::runtime_error("Variable has no value: " + node->name);
  }
  current_val =
      builder.CreateLoad(alloca->getAllocatedType(), alloca, node->name);
}

void CodeGen::visit(Declaration *node) {
  llvm::Type *type = get_llvm_type(node->type.value());
  llvm::AllocaInst *alloca =
      builder.CreateAlloca(type, nullptr, node->variable_name);
  named_values[node->variable_name] = alloca;

  if (node->expression) {
    node->expression->accept(*this);
    builder.CreateStore(current_val, alloca);
  }
}
void CodeGen::visit(Assignment *node) {
  auto alloca = named_values[node->variable_name];
  node->expression->accept(*this);
  builder.CreateStore(current_val, alloca);
}
void CodeGen::visit(FunctionCall *node) {
  llvm::Function *callee_fn = module->getFunction(node->funcName->name);
  if (!callee_fn) {
    throw std::runtime_error("Unknown function referenced: " +
                             node->funcName->name);
  }
  std::vector<llvm::Value *> args;
  for (unsigned long i = 0, e = node->arguments.size(); i != e; ++i) {
    node->arguments[i]->accept(*this);
    args.push_back(current_val);
  }
  if (callee_fn->getReturnType()->isVoidTy()) {
    current_val = builder.CreateCall(callee_fn, args);
  } else
    current_val = builder.CreateCall(callee_fn, args, "calltmp");
}

void CodeGen::visit(ReturnStatement *node) {
  node->expression->accept(*this);
  builder.CreateRet(current_val);

  // Ensure no more code is added to the function after a return statement.
  llvm::BasicBlock *unreachable_block =
      llvm::BasicBlock::Create(context, "unreachable", current_fn);
  builder.SetInsertPoint(unreachable_block);
}

void CodeGen::visit(IfStatement *node) {
  llvm::Function *function = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *then_bb =
      llvm::BasicBlock::Create(context, "then", function);
  llvm::BasicBlock *else_bb = llvm::BasicBlock::Create(context, "else");
  llvm::BasicBlock *merge_bb = llvm::BasicBlock::Create(context, "ifcont");

  node->condition->accept(*this);
  llvm::Value *condValue = current_val;
  builder.CreateCondBr(condValue, then_bb, else_bb);

  builder.SetInsertPoint(then_bb);
  node->then_branch->accept(*this);
  if (!builder.GetInsertBlock()->getTerminator()) {
    builder.CreateBr(merge_bb);
  }

  function->insert(function->end(), else_bb);
  builder.SetInsertPoint(else_bb);
  if (node->else_branch) {
    node->else_branch->accept(*this);
  }
  if (!builder.GetInsertBlock()->getTerminator()) {
    builder.CreateBr(merge_bb);
  }

  function->insert(function->end(), merge_bb);
  builder.SetInsertPoint(merge_bb);
}

void CodeGen::visit(WhileLoop *node) {
  llvm::Function *function = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *cond_bb =
      llvm::BasicBlock::Create(context, "loopcond", function);
  llvm::BasicBlock *body_bb = llvm::BasicBlock::Create(context, "loopbody");
  llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(context, "afterloop");

  builder.CreateBr(cond_bb);
  builder.SetInsertPoint(cond_bb);

  node->condition->accept(*this);
  llvm::Value *condValue = current_val;
  builder.CreateCondBr(condValue, body_bb, after_bb);

  function->insert(function->end(), body_bb);

  builder.SetInsertPoint(body_bb);

  node->body->accept(*this);
  builder.CreateBr(cond_bb);

  function->insert(function->end(), after_bb);
  builder.SetInsertPoint(after_bb);
}

void CodeGen::visit(ForLoop *node) {
  llvm::Function *function = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *preheader_bb = builder.GetInsertBlock();
  llvm::BasicBlock *loop_bb =
      llvm::BasicBlock::Create(context, "loop", function);
  llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(context, "afterloop");

  builder.CreateBr(loop_bb);
  builder.SetInsertPoint(loop_bb);

  node->initializer->accept(*this);
  node->condition->accept(*this);
  llvm::Value *condValue = current_val;
  builder.CreateCondBr(condValue, loop_bb, after_bb);

  for (auto &stmt : node->body) {
    stmt->accept(*this);
  }
  node->increment->accept(*this);
  builder.CreateBr(loop_bb);

  after_bb->insertInto(function);
  builder.SetInsertPoint(after_bb);
}

void CodeGen::visit(Function *node) {
  std::vector<llvm::Type *> param_types;
  for (const auto &param : node->parameters) {
    param_types.push_back(get_llvm_type(param.type));
  }

  llvm::FunctionType *func_type = llvm::FunctionType::get(
      get_llvm_type(node->return_type), param_types, false);
  llvm::Function *function =
      llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                             node->name->name, module.get());

  unsigned idx = 0;
  for (auto &arg : function->args()) {
    arg.setName(node->parameters[idx++].name);
  }

  llvm::BasicBlock *basic_block =
      llvm::BasicBlock::Create(context, "entry", function);
  builder.SetInsertPoint(basic_block);

  named_values.clear();
  for (auto &arg : function->args()) {
    llvm::AllocaInst *alloca =
        builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
    builder.CreateStore(&arg, alloca);
    named_values[std::string(arg.getName())] = alloca;
  }

  current_fn = function;
  node->body->accept(*this);

  if (node->return_type == AlohaType::Type::VOID) {
    builder.CreateRetVoid();
  } else if (!builder.GetInsertBlock()->getTerminator()) {
    builder.CreateRet(
        llvm::Constant::getNullValue(get_llvm_type(node->return_type)));
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

llvm::Type *CodeGen::get_llvm_type(AlohaType::Type type) {
  switch (type) {
  case AlohaType::Type::NUMBER:
    return llvm::Type::getDoubleTy(context);
  case AlohaType::Type::VOID:
    return llvm::Type::getVoidTy(context);
  case AlohaType::Type::BOOL:
    return llvm::Type::getInt1Ty(context);
  case AlohaType::Type::STRING:
    return llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
  default:
    throw std::runtime_error(
        "Unknown type while converting from Aloha Type to LLVM Type");
  }
}

void CodeGen::dump_ir() const {
  module->print(llvm::outs(), nullptr);
  // dumpNamedValues();
}

void CodeGen::print_value() const { print_value(current_val); }

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

void CodeGen::dump_named_values() const {
  std::cout << "DUMPING NAMED VALUES" << std::endl;
  for (const auto &pair : named_values) {
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

void CodeGen::add_builtin_fns() {
  llvm::FunctionType *print_type =
      llvm::FunctionType::get(llvm::Type::getVoidTy(context),
                              {llvm::PointerType::getUnqual(context)}, false);
  llvm::FunctionType *print_num_type =
      llvm::FunctionType::get(llvm::Type::getVoidTy(context),
                              {llvm::Type::getDoubleTy(context)}, false);
  llvm::PointerType *char_type =
      llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);

  llvm::FunctionType *input_type = llvm::FunctionType::get(
      char_type, {llvm::Type::getVoidTy(context)}, false);

  module->getOrInsertFunction("print", print_type);
  module->getOrInsertFunction("println", print_type);
  module->getOrInsertFunction("printNum", print_num_type);
  module->getOrInsertFunction("printlnNum", print_num_type);
  module->getOrInsertFunction("input", input_type);
}
