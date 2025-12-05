#include "codegen.h"
#include "ast/ast.h"
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
#include <stdexcept>
#include <string>

CodeGen::CodeGen()
    : builder(context), current_val(nullptr), current_fn(nullptr)
{
  module = std::make_unique<llvm::Module>("my_module", context);
}

bool CodeGen::generate_code(aloha::Program *program)
{
  add_builtin_fns();
  program->accept(*this);
  auto status = llvm::verifyModule(*module, &llvm::errs());
  return status;
}

void CodeGen::visit(aloha::Number *node)
{
  auto num = std::stod(node->m_value);
  current_val = llvm::ConstantFP::get(context, llvm::APFloat(num));
}

void CodeGen::visit(aloha::Boolean *node)
{
  auto value = node->m_value;
  current_val = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), value);
}

void CodeGen::visit(aloha::String *node)
{
  auto const &str = node->m_value;
  current_val = builder.CreateGlobalStringPtr(str, "str");
}

void CodeGen::visit(aloha::ExpressionStatement *node)
{
  node->m_expr->accept(*this);
}

void CodeGen::visit(aloha::UnaryExpression *node)
{
  node->m_expr->accept(*this);
  if (node->m_op == "-")
  {
    current_val = builder.CreateFNeg(current_val, "negtmp");
  }
}

void CodeGen::visit(aloha::BinaryExpression *node)
{
  node->m_left->accept(*this);
  llvm::Value *left = current_val;
  node->m_right->accept(*this);
  llvm::Value *right = current_val;

  if (node->m_op == "+")
  {
    current_val = builder.CreateFAdd(left, right, "addtmp");
  }
  else if (node->m_op == "-")
  {
    current_val = builder.CreateFSub(left, right, "subtmp");
  }
  else if (node->m_op == "*")
  {
    current_val = builder.CreateFMul(left, right, "multmp");
  }
  else if (node->m_op == "/")
  {
    current_val = builder.CreateFDiv(left, right, "divtmp");
  }
  else if (node->m_op == "%")
  {
    current_val = builder.CreateFRem(left, right, "modtmp");
  }
  else if (node->m_op == ">=")
  {
    current_val = builder.CreateFCmpOGE(left, right, "gtetmpe");
  }
  else if (node->m_op == "<=")
  {
    current_val = builder.CreateFCmpOLE(left, right, "ltetmp");
  }
  else if (node->m_op == "<")
  {
    current_val = builder.CreateFCmpOLT(left, right, "lttmp");
  }
  else if (node->m_op == ">")
  {
    current_val = builder.CreateFCmpOGT(left, right, "gttmp");
  }
  else if (node->m_op == "==")
  {
    current_val = builder.CreateFCmpOEQ(left, right, "eqtmp");
  }
  else if (node->m_op == "!=")
  {
    current_val = builder.CreateFCmpONE(left, right, "netmp");
  }
}

void CodeGen::visit(aloha::Identifier *node)
{
  auto itr = named_values.find(node->m_name);
  if (itr == named_values.end())
  {
    throw std::runtime_error("Unknown variable name: " + node->m_name);
  }

  auto *alloca = itr->second;

  if (!alloca)
  {
    throw std::runtime_error("Variable has no value: " + node->m_name);
  }
  current_val =
      builder.CreateLoad(alloca->getAllocatedType(), alloca, node->m_name);
}

void CodeGen::visit(aloha::Declaration *node)
{
  llvm::Type *type = get_llvm_type(node->m_type.value());
  llvm::AllocaInst *alloca =
      builder.CreateAlloca(type, nullptr, node->m_variable_name);
  named_values[node->m_variable_name] = alloca;

  if (node->m_expression)
  {
    node->m_expression->accept(*this);
    builder.CreateStore(current_val, alloca);
  }
}

void CodeGen::visit(aloha::Assignment *node)
{
  auto alloca = named_values[node->m_variable_name];
  node->m_expression->accept(*this);
  builder.CreateStore(current_val, alloca);
}
void CodeGen::visit(aloha::FunctionCall *node)
{
  llvm::Function *callee_fn = module->getFunction(node->m_func_name->m_name);
  if (!callee_fn)
  {
    throw std::runtime_error("Unknown function referenced: " +
                             node->m_func_name->m_name);
  }
  std::vector<llvm::Value *> args;
  for (unsigned long i = 0, e = node->m_arguments.size(); i != e; ++i)
  {
    node->m_arguments[i]->accept(*this);
    args.push_back(current_val);
  }
  if (callee_fn->getReturnType()->isVoidTy())
  {
    current_val = builder.CreateCall(callee_fn, args);
  }
  else
    current_val = builder.CreateCall(callee_fn, args, "calltmp");
}

void CodeGen::visit(aloha::ReturnStatement *node)
{
  bool is_main = current_fn->getName() == "main";

  // Handle void return
  if (!node->m_expression)
  {
    builder.CreateRetVoid();
  }
  else
  {
    node->m_expression->accept(*this);

    if (is_main && node->m_expression->get_type() == AlohaType::Type::NUMBER)
    {
      // Convert number to int32 for main's exit code
      llvm::Value *exit_code = builder.CreateFPToSI(
          current_val,
          llvm::Type::getInt32Ty(context),
          "exit_code");
      builder.CreateRet(exit_code);
    }
    else
    {
      builder.CreateRet(current_val);
    }
  }

  // Ensure no more code is added to the function after a return statement.
  llvm::BasicBlock *unreachable_block =
      llvm::BasicBlock::Create(context, "unreachable", current_fn);
  builder.SetInsertPoint(unreachable_block);
}

void CodeGen::visit(aloha::IfStatement *node)
{
  llvm::Function *function = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *then_bb =
      llvm::BasicBlock::Create(context, "then", function);
  llvm::BasicBlock *else_bb = llvm::BasicBlock::Create(context, "else");
  llvm::BasicBlock *merge_bb = llvm::BasicBlock::Create(context, "ifcont");

  node->m_condition->accept(*this);
  llvm::Value *condValue = current_val;
  builder.CreateCondBr(condValue, then_bb, else_bb);

  builder.SetInsertPoint(then_bb);
  node->m_then_branch->accept(*this);
  if (!builder.GetInsertBlock()->getTerminator())
  {
    builder.CreateBr(merge_bb);
  }

  function->insert(function->end(), else_bb);
  builder.SetInsertPoint(else_bb);
  if (node->m_else_branch)
  {
    node->m_else_branch->accept(*this);
  }
  if (!builder.GetInsertBlock()->getTerminator())
  {
    builder.CreateBr(merge_bb);
  }

  function->insert(function->end(), merge_bb);
  builder.SetInsertPoint(merge_bb);
}

void CodeGen::visit(aloha::WhileLoop *node)
{
  llvm::Function *function = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *cond_bb =
      llvm::BasicBlock::Create(context, "loopcond", function);
  llvm::BasicBlock *body_bb = llvm::BasicBlock::Create(context, "loopbody");
  llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(context, "afterloop");

  builder.CreateBr(cond_bb);
  builder.SetInsertPoint(cond_bb);

  node->m_condition->accept(*this);
  llvm::Value *condValue = current_val;
  builder.CreateCondBr(condValue, body_bb, after_bb);

  function->insert(function->end(), body_bb);

  builder.SetInsertPoint(body_bb);

  node->m_body->accept(*this);
  builder.CreateBr(cond_bb);

  function->insert(function->end(), after_bb);
  builder.SetInsertPoint(after_bb);
}

void CodeGen::visit(aloha::ForLoop *node)
{
  llvm::Function *function = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *preheader_bb = builder.GetInsertBlock();
  llvm::BasicBlock *loop_bb =
      llvm::BasicBlock::Create(context, "loop", function);
  llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(context, "afterloop");

  builder.CreateBr(loop_bb);
  builder.SetInsertPoint(loop_bb);

  node->m_initializer->accept(*this);
  node->m_condition->accept(*this);
  llvm::Value *condValue = current_val;
  builder.CreateCondBr(condValue, loop_bb, after_bb);

  for (auto &stmt : node->m_body)
  {
    stmt->accept(*this);
  }
  node->m_increment->accept(*this);
  builder.CreateBr(loop_bb);

  after_bb->insertInto(function);
  builder.SetInsertPoint(after_bb);
}

void CodeGen::visit(aloha::Function *node)
{
  std::vector<llvm::Type *> param_types;
  for (const auto &param : node->m_parameters)
  {
    param_types.push_back(get_llvm_type(param.m_type));
  }

  // Special handling for main: always return int32
  bool is_main = node->m_name->m_name == "main";
  llvm::Type *return_type;

  if (is_main)
  {
    return_type = llvm::Type::getInt32Ty(context);
  }
  else
  {
    return_type = get_llvm_type(node->m_return_type);
  }

  llvm::FunctionType *func_type = llvm::FunctionType::get(
      return_type, param_types, false);
  llvm::Function *function =
      llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                             node->m_name->m_name, module.get());

  unsigned idx = 0;
  for (auto &arg : function->args())
  {
    arg.setName(node->m_parameters[idx++].m_name);
  }

  // For extern functions, we don't generate a body
  if (node->m_is_extern)
  {
    return;
  }

  llvm::BasicBlock *basic_block =
      llvm::BasicBlock::Create(context, "entry", function);
  builder.SetInsertPoint(basic_block);

  named_values.clear();
  for (auto &arg : function->args())
  {
    llvm::AllocaInst *alloca =
        builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
    builder.CreateStore(&arg, alloca);
    named_values[std::string(arg.getName())] = alloca;
  }

  current_fn = function;
  node->m_body->accept(*this);

  // Add default return if function doesn't have a terminator
  if (!builder.GetInsertBlock()->getTerminator())
  {
    if (is_main)
    {
      // Main function returns 0 by default
      llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
      builder.CreateRet(zero);
    }
    else if (node->m_return_type == AlohaType::Type::VOID)
    {
      builder.CreateRetVoid();
    }
    else
    {
      builder.CreateRet(
          llvm::Constant::getNullValue(get_llvm_type(node->m_return_type)));
    }
  }

  llvm::verifyFunction(*function);
}

void CodeGen::visit(aloha::StructDecl *node)
{
  std::string struct_name = node->m_name;
  AlohaType::Type struct_type = AlohaType::create_struct_type(
      (int)struct_types.size() /
      2); // divide by 2 because we are inserting twice in struct_types

  type_to_struct[struct_type] = struct_name;
  if (llvm::StructType::getTypeByName(context, struct_name))
  {
    throw std::runtime_error("Struct with name " + struct_name +
                             " already exists");
  }

  std::vector<llvm::Type *> field_types;
  std::vector<std::string> field_names;
  for (const auto &field : node->m_fields)
  {
    field_types.push_back(get_llvm_type(field.m_type));
    field_names.push_back(field.m_name);
  }

  llvm::StructType *llvm_struct_type =
      llvm::StructType::create(context, field_types, struct_name);

  struct_types[struct_name] = llvm_struct_type;
  struct_types[AlohaType::to_string(struct_type)] = llvm_struct_type;
  struct_field_names[struct_name] = field_names;

  std::vector<llvm::Type *> ctor_param_types = field_types;
  llvm::FunctionType *ctor_type = llvm::FunctionType::get(
      llvm::PointerType::getUnqual(llvm_struct_type), ctor_param_types, false);

  llvm::Function *ctor_fn =
      llvm::Function::Create(ctor_type, llvm::Function::ExternalLinkage,
                             "create_" + node->m_name, module.get());
  llvm::BasicBlock *ctor_bb =
      llvm::BasicBlock::Create(context, "entry", ctor_fn);
  builder.SetInsertPoint(ctor_bb);

  llvm::AllocaInst *struct_alloca =
      builder.CreateAlloca(llvm_struct_type, nullptr, "struct_instance");

  auto arg_it = ctor_fn->arg_begin();
  for (unsigned i = 0; i < node->m_fields.size(); ++i, ++arg_it)
  {
    llvm::Value *field_ptr =
        builder.CreateStructGEP(llvm_struct_type, struct_alloca, i);
    builder.CreateStore(arg_it, field_ptr);
  }

  builder.CreateRet(struct_alloca);
  std::cout << "passed struct" << std::endl;
}

void CodeGen::visit(aloha::StructInstantiation *node)
{
  AlohaType::Type struct_type = node->m_type;
  std::string struct_name = type_to_struct[struct_type];

  auto struct_type_it = struct_types.find(struct_name);
  if (struct_type_it == struct_types.end())
  {
    throw std::runtime_error("Unknown struct type: " + node->m_struct_name);
  }

  llvm::StructType *llvm_struct_type = struct_type_it->second;

  // Prepare arguments for the constructor call
  std::vector<llvm::Value *> ctor_args;
  for (const auto &arg : node->m_field_values)
  {
    arg->accept(*this);
    ctor_args.push_back(current_val);
  }

  // Call the constructor function
  llvm::Function *ctor_function = module->getFunction("create_" + struct_name);
  if (!ctor_function)
  {
    throw std::runtime_error("Constructor function not found for struct: " +
                             struct_name);
  }

  current_val = builder.CreateCall(ctor_function, ctor_args, "struct_instance");
}

void CodeGen::visit(aloha::StructFieldAccess *node)
{
  node->m_struct_expr->accept(*this);
  llvm::Value *struct_ptr = current_val;

  AlohaType::Type struct_type = node->m_struct_expr->get_type();
  std::string struct_name = type_to_struct[struct_type];

  auto struct_type_it = struct_types.find(struct_name);
  if (struct_type_it == struct_types.end())
  {
    throw std::runtime_error("Unknown struct type: " + struct_name);
  }

  llvm::StructType *llvm_struct_type = struct_type_it->second;

  // Find the field index by name
  auto field_names_it = struct_field_names.find(struct_name);
  if (field_names_it == struct_field_names.end())
  {
    throw std::runtime_error("Struct field names not found for: " + struct_name);
  }

  const auto &field_names = field_names_it->second;
  unsigned int field_idx = 0;
  bool found = false;
  for (unsigned int i = 0; i < field_names.size(); ++i)
  {
    if (field_names[i] == node->m_field_name)
    {
      field_idx = i;
      found = true;
      break;
    }
  }

  if (!found)
  {
    throw std::runtime_error("Field '" + node->m_field_name +
                             "' not found in struct " + struct_name);
  }

  llvm::Value *field_ptr =
      builder.CreateStructGEP(llvm_struct_type, struct_ptr, field_idx);
  current_val = builder.CreateLoad(get_llvm_type(node->get_type()), field_ptr,
                                   "fieldptr");
}

void CodeGen::visit(aloha::StructFieldAssignment *node)
{
  node->m_struct_expr->accept(*this);
  llvm::Value *struct_ptr = current_val;

  AlohaType::Type struct_type = node->m_struct_expr->get_type();
  std::string struct_name = type_to_struct[struct_type];

  auto struct_type_it = struct_types.find(struct_name);
  if (struct_type_it == struct_types.end())
  {
    throw std::runtime_error("Unknown struct type: " + struct_name);
  }

  node->m_value->accept(*this);
  if (!current_val)
  {
    std::runtime_error("Cannot codegen rvalue of struct");
  }

  llvm::StructType *llvm_struct_type = struct_type_it->second;

  // Find the field index by name
  auto field_names_it = struct_field_names.find(struct_name);
  if (field_names_it == struct_field_names.end())
  {
    throw std::runtime_error("Struct field names not found for: " + struct_name);
  }

  const auto &field_names = field_names_it->second;
  unsigned int field_idx = 0;
  bool found = false;
  for (unsigned int i = 0; i < field_names.size(); ++i)
  {
    if (field_names[i] == node->m_field_name)
    {
      field_idx = i;
      found = true;
      break;
    }
  }

  if (!found)
  {
    throw std::runtime_error("Field '" + node->m_field_name +
                             "' not found in struct " + struct_name);
  }

  llvm::Value *field_ptr = builder.CreateStructGEP(llvm_struct_type, struct_ptr,
                                                   field_idx, "fieldptr");
  builder.CreateStore(current_val, field_ptr);
}

void CodeGen::visit(aloha::StatementBlock *node)
{
  for (auto &stmt : node->m_statements)
  {
    stmt->accept(*this);
  }
}

void CodeGen::visit(aloha::Array *node)
{
  llvm::Type *elementType = get_llvm_type(node->m_type);
  llvm::ArrayType *arrayType = llvm::ArrayType::get(elementType, node->m_size);
  llvm::AllocaInst *arrayAlloca =
      builder.CreateAlloca(arrayType, nullptr, "array");
  for (size_t i = 0; i < node->m_members.size(); ++i)
  {
    node->m_members[i]->accept(*this);
    llvm::Value *memberValue = current_val;

    std::vector<llvm::Value *> indices = {
        llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
        llvm::ConstantInt::get(context, llvm::APInt(32, i))};
    llvm::Value *elementPtr = builder.CreateInBoundsGEP(
        arrayType, arrayAlloca, indices, "array_element_ptr");
    builder.CreateStore(memberValue, elementPtr);
  }
  current_val = arrayAlloca;
}

void CodeGen::visit(aloha::Program *node)
{
  for (auto &n : node->m_nodes)
  {
    n->accept(*this);
  }
}

void CodeGen::visit(aloha::Import *node)
{
}

llvm::Type *CodeGen::get_llvm_type(AlohaType::Type type)
{
  switch (type)
  {
  case AlohaType::Type::NUMBER:
    return llvm::Type::getDoubleTy(context);
  case AlohaType::Type::VOID:
    return llvm::Type::getVoidTy(context);
  case AlohaType::Type::BOOL:
    return llvm::Type::getInt1Ty(context);
  case AlohaType::Type::STRING:
    return llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
  default:
    if (AlohaType::is_struct_type(type))
    {
      auto it = type_to_struct.find(type);
      if (it != type_to_struct.end())
      {
        std::string struct_name = it->second;
        auto struct_it = struct_types.find(struct_name);
        if (struct_it != struct_types.end())
        {
          return struct_it->second;
        }
      }
      throw std::runtime_error("Unknown struct type: " +
                               AlohaType::to_string(type));
    }
    throw std::runtime_error(
        "Unknown type while converting from aloha Type to LLVM Type");
  }
}

void CodeGen::dump_ir() const
{
  module->print(llvm::outs(), nullptr);
  // dumpNamedValues();
}

void CodeGen::print_value() const { print_value(current_val); }

void CodeGen::print_value(llvm::Value *value) const
{
  std::string value_str;
  llvm::raw_string_ostream rso(value_str);
  value->print(rso);
  std::cout << rso.str() << std::endl;
}
void CodeGen::print_llvm_type(llvm::Type *type) const
{
  std::string type_str;
  llvm::raw_string_ostream rso(type_str);
  type->print(rso);
  std::cout << rso.str() << std::endl;
}

void CodeGen::dump_struct_types() const
{
  std::cout << "DUMPING STRUCT TYPES" << std::endl;
  for (const auto &pair : struct_types)
  {
    std::string name = pair.first;
    std::cout << "Struct: " << name << std::endl;
    std::cout << "Type: ";
    print_llvm_type(pair.second);
    std::cout << std::endl;
  }
}

void CodeGen::dump_named_values() const
{
  std::cout << "DUMPING NAMED VALUES" << std::endl;
  for (const auto &pair : named_values)
  {
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

void CodeGen::declare_fn_external(const std::string &name,
                                  const FunctionInfo &info)
{
  // dont redeclare if already declared
  if (module->getFunction(name))
  {
    return;
  }

  std::vector<llvm::Type *> param_types;
  for (const auto &param_type : info.param_types)
  {
    param_types.push_back(get_llvm_type(param_type));
  }

  llvm::Type *return_type = get_llvm_type(info.return_type);
  llvm::FunctionType *fn_type = llvm::FunctionType::get(
      return_type, param_types, false);

  // declare as external
  llvm::Function::Create(fn_type, llvm::Function::ExternalLinkage,
                         name, module.get());
}

void CodeGen::add_builtin_fns()
{
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
