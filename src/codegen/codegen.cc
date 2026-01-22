#include "codegen.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <iostream>

namespace Codegen
{
  CodeGenerator::CodeGenerator(AIR::TyTable &ty_table)
      : context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>("aloha_module", *context)),
        builder(std::make_unique<llvm::IRBuilder<>>(*context)),
        ty_table(ty_table),
        current_value(nullptr),
        current_function(nullptr),
        current_air_module(nullptr)
  {
  }

  std::unique_ptr<llvm::Module> CodeGenerator::generate(AIR::Module *air_module)
  {
    if (!air_module)
    {
      report_error("Cannot generate code for null module", Location{0, 0});
      return nullptr;
    }

    current_air_module = air_module;
    module->setSourceFileName(air_module->name);

    generate_types();

    // declare all functions for forward references
    declare_functions();

    generate_function_bodies();

    if (has_errors())
    {
      error_reporter.print();
      return nullptr;
    }

    std::string verify_error;
    llvm::raw_string_ostream error_stream(verify_error);
    if (llvm::verifyModule(*module, &error_stream))
    {
      std::cerr << "LLVM Module verification failed:\n"
                << verify_error << std::endl;
      // return module so we can inspect IR
    }

    return std::move(module);
  }

  void CodeGenerator::generate_types()
  {
    // primitive types
    type_map[AIR::TyIds::NUMBER] = llvm::Type::getDoubleTy(*context);
    type_map[AIR::TyIds::BOOL] = llvm::Type::getInt1Ty(*context);
    type_map[AIR::TyIds::VOID] = llvm::Type::getVoidTy(*context);
    type_map[AIR::TyIds::STRING] = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
    type_map[AIR::TyIds::ERROR] = llvm::Type::getVoidTy(*context); // Placeholder

    generate_struct_types();
  }

  void CodeGenerator::generate_struct_types()
  {
    if (!current_air_module)
      return;

    // First pass: Create opaque struct types for forward references
    for (const auto &struct_decl : current_air_module->structs)
    {
      auto *struct_type = llvm::StructType::create(*context, struct_decl->name);
      struct_map[struct_decl->struct_id] = struct_type;
      type_map[struct_decl->ty_id] = struct_type;
    }

    // Second pass: Fill in struct bodies
    for (const auto &struct_decl : current_air_module->structs)
    {
      std::vector<llvm::Type *> field_types;
      for (const auto &field : struct_decl->fields)
      {
        llvm::Type *field_type = get_llvm_type(field.ty);
        if (!field_type)
        {
          report_error("Cannot resolve field type for '" + field.name + "'",
                       field.loc);
          field_type = llvm::Type::getInt32Ty(*context); // Fallback
        }
        field_types.push_back(field_type);
      }

      auto *struct_type = struct_map[struct_decl->struct_id];
      struct_type->setBody(field_types);
    }
  }

  llvm::Type *CodeGenerator::get_llvm_type(AIR::TyId ty_id)
  {
    auto it = type_map.find(ty_id);
    if (it != type_map.end())
    {
      return it->second;
    }

    // Check if it's a struct type we haven't mapped yet
    const AIR::TyInfo *ty_info = ty_table.get_ty_info(ty_id);
    if (ty_info && ty_info->kind == AIR::TyKind::STRUCT)
    {
      if (ty_info->struct_id.has_value())
      {
        auto struct_it = struct_map.find(ty_info->struct_id.value());
        if (struct_it != struct_map.end())
        {
          type_map[ty_id] = struct_it->second;
          return struct_it->second;
        }
      }
    }

    return nullptr;
  }

  void CodeGenerator::declare_functions()
  {
    if (!current_air_module)
      return;

    for (const auto &func : current_air_module->functions)
    {
      llvm::FunctionType *func_type = get_function_type(func.get());
      if (!func_type)
      {
        report_error("Cannot create function type for '" + func->name + "'", func->loc);
        continue;
      }

      llvm::Function::LinkageTypes linkage = func->is_extern
                                                 ? llvm::Function::ExternalLinkage
                                                 : llvm::Function::ExternalLinkage;

      llvm::Function *llvm_func = llvm::Function::Create(
          func_type, linkage, func->name, module.get());

      // Set parameter names
      unsigned idx = 0;
      for (auto &arg : llvm_func->args())
      {
        if (idx < func->params.size())
        {
          arg.setName(func->params[idx].name);
        }
        idx++;
      }

      function_map[func->func_id] = llvm_func;
    }
  }

  llvm::FunctionType *CodeGenerator::get_function_type(AIR::Function *func)
  {
    // Get return type
    llvm::Type *return_type = get_llvm_type(func->return_ty);
    if (!return_type)
    {
      report_error("Cannot resolve return type for function '" + func->name + "'",
                   func->loc);
      return nullptr;
    }

    // Get parameter types
    std::vector<llvm::Type *> param_types;
    for (const auto &param : func->params)
    {
      llvm::Type *param_type = get_llvm_type(param.ty);
      if (!param_type)
      {
        report_error("Cannot resolve parameter type for '" + param.name + "'",
                     param.loc);
        return nullptr;
      }
      param_types.push_back(param_type);
    }

    return llvm::FunctionType::get(return_type, param_types, false);
  }

  void CodeGenerator::generate_function_bodies()
  {
    if (!current_air_module)
      return;

    for (const auto &func : current_air_module->functions)
    {
      if (!func->is_extern)
      {
        generate_function(func.get());
      }
    }
  }

  void CodeGenerator::generate_function(AIR::Function *func)
  {
    llvm::Function *llvm_func = function_map[func->func_id];
    if (!llvm_func)
    {
      report_error("Function '" + func->name + "' not declared", func->loc);
      return;
    }

    current_function = llvm_func;
    variable_map.clear();

    // Create entry block
    llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(*context, "entry", llvm_func);
    builder->SetInsertPoint(entry_block);

    // Allocate space for parameters and store them
    unsigned idx = 0;
    for (auto &arg : llvm_func->args())
    {
      if (idx < func->params.size())
      {
        const auto &param = func->params[idx];
        llvm::AllocaInst *alloca = create_entry_block_alloca(
            llvm_func, param.name, arg.getType());
        builder->CreateStore(&arg, alloca);
        variable_map[param.var_id] = alloca;
      }
      idx++;
    }

    // Generate function body statements
    for (const auto &stmt : func->body)
    {
      stmt->accept(*this);
    }

    // Add return if current block doesn't have a terminator
    llvm::BasicBlock *current_block = builder->GetInsertBlock();
    if (current_block && !current_block->getTerminator())
    {
      if (func->return_ty == AIR::TyIds::VOID)
      {
        builder->CreateRetVoid();
      }
      else
      {
        // For non-void functions, if there's no terminator, it's an error
        // (should have been caught by semantic analysis)
        report_error("Function '" + func->name + "' missing return statement", func->loc);
        // Add a dummy return to prevent LLVM errors
        llvm::Type *ret_type = get_llvm_type(func->return_ty);
        if (ret_type->isDoubleTy())
        {
          builder->CreateRet(llvm::ConstantFP::get(ret_type, 0.0));
        }
        else if (ret_type->isIntegerTy(1))
        {
          builder->CreateRet(llvm::ConstantInt::get(ret_type, 0));
        }
        else
        {
          builder->CreateRet(llvm::Constant::getNullValue(ret_type));
        }
      }
    }

    current_function = nullptr;
  }

  llvm::AllocaInst *CodeGenerator::create_entry_block_alloca(
      llvm::Function *func, const std::string &var_name, llvm::Type *type)
  {
    llvm::IRBuilder<> tmp_builder(&func->getEntryBlock(),
                                  func->getEntryBlock().begin());
    return tmp_builder.CreateAlloca(type, nullptr, var_name);
  }

  void CodeGenerator::visit(AIR::NumberLiteral *node)
  {
    current_value = llvm::ConstantFP::get(*context, llvm::APFloat(node->value));
  }

  void CodeGenerator::visit(AIR::StringLiteral *node)
  {
    llvm::Constant *str_constant = llvm::ConstantDataArray::getString(*context, node->value);
    llvm::GlobalVariable *global_str = new llvm::GlobalVariable(
        *module,
        str_constant->getType(),
        true, // is constant
        llvm::GlobalValue::PrivateLinkage,
        str_constant,
        ".str");

    llvm::Constant *zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0);
    std::vector<llvm::Constant *> indices = {zero, zero};
    current_value = llvm::ConstantExpr::getGetElementPtr(
        str_constant->getType(), global_str, indices);
  }

  void CodeGenerator::visit(AIR::BoolLiteral *node)
  {
    current_value = llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), node->value ? 1 : 0);
  }

  void CodeGenerator::visit(AIR::VarRef *node)
  {
    auto it = variable_map.find(node->var_id);
    if (it == variable_map.end())
    {
      report_error("Undefined variable: '" + node->name + "'", node->loc);
      current_value = nullptr;
      return;
    }

    llvm::AllocaInst *alloca = it->second;
    current_value = builder->CreateLoad(alloca->getAllocatedType(), alloca, node->name);
  }

  void CodeGenerator::visit(AIR::BinaryOp *node)
  {
    node->left->accept(*this);
    llvm::Value *left = current_value;

    node->right->accept(*this);
    llvm::Value *right = current_value;

    if (!left || !right)
    {
      report_error("Failed to generate binary operation operands", node->loc);
      current_value = nullptr;
      return;
    }

    switch (node->op)
    {
    case AIR::BinaryOpKind::ADD:
      current_value = builder->CreateFAdd(left, right, "addtmp");
      break;
    case AIR::BinaryOpKind::SUB:
      current_value = builder->CreateFSub(left, right, "subtmp");
      break;
    case AIR::BinaryOpKind::MUL:
      current_value = builder->CreateFMul(left, right, "multmp");
      break;
    case AIR::BinaryOpKind::DIV:
      current_value = builder->CreateFDiv(left, right, "divtmp");
      break;
    case AIR::BinaryOpKind::MOD:
      current_value = builder->CreateFRem(left, right, "modtmp");
      break;

    case AIR::BinaryOpKind::EQ:
      if (node->left->ty == AIR::TyIds::NUMBER)
      {
        current_value = builder->CreateFCmpOEQ(left, right, "eqtmp");
      }
      else if (node->left->ty == AIR::TyIds::BOOL)
      {
        current_value = builder->CreateICmpEQ(left, right, "eqtmp");
      }
      else
      {
        report_error("Unsupported type for equality comparison", node->loc);
        current_value = nullptr;
      }
      break;

    case AIR::BinaryOpKind::NE:
      if (node->left->ty == AIR::TyIds::NUMBER)
      {
        current_value = builder->CreateFCmpONE(left, right, "netmp");
      }
      else if (node->left->ty == AIR::TyIds::BOOL)
      {
        current_value = builder->CreateICmpNE(left, right, "netmp");
      }
      else
      {
        report_error("Unsupported type for inequality comparison", node->loc);
        current_value = nullptr;
      }
      break;

    case AIR::BinaryOpKind::LT:
      current_value = builder->CreateFCmpOLT(left, right, "lttmp");
      break;
    case AIR::BinaryOpKind::LE:
      current_value = builder->CreateFCmpOLE(left, right, "letmp");
      break;
    case AIR::BinaryOpKind::GT:
      current_value = builder->CreateFCmpOGT(left, right, "gttmp");
      break;
    case AIR::BinaryOpKind::GE:
      current_value = builder->CreateFCmpOGE(left, right, "getmp");
      break;

    case AIR::BinaryOpKind::AND:
      current_value = builder->CreateAnd(left, right, "andtmp");
      break;
    case AIR::BinaryOpKind::OR:
      current_value = builder->CreateOr(left, right, "ortmp");
      break;

    default:
      report_error("Unknown binary operation", node->loc);
      current_value = nullptr;
      break;
    }
  }

  void CodeGenerator::visit(AIR::UnaryOp *node)
  {
    node->operand->accept(*this);
    llvm::Value *operand = current_value;

    if (!operand)
    {
      report_error("Failed to generate unary operation operand", node->loc);
      current_value = nullptr;
      return;
    }

    switch (node->op)
    {
    case AIR::UnaryOpKind::NEG:
      current_value = builder->CreateFNeg(operand, "negtmp");
      break;
    case AIR::UnaryOpKind::NOT:
      current_value = builder->CreateNot(operand, "nottmp");
      break;
    default:
      report_error("Unknown unary operation", node->loc);
      current_value = nullptr;
      break;
    }
  }

  void CodeGenerator::visit(AIR::Call *node)
  {
    llvm::Function *callee = function_map[node->func_id];
    if (!callee)
    {
      report_error("Undefined function: '" + node->function_name + "'", node->loc);
      current_value = nullptr;
      return;
    }

    std::vector<llvm::Value *> args;
    for (const auto &arg : node->arguments)
    {
      arg->accept(*this);
      if (!current_value)
      {
        report_error("Failed to generate function argument", arg->loc);
        return;
      }
      args.push_back(current_value);
    }

    if (callee->getReturnType()->isVoidTy())
    {
      current_value = builder->CreateCall(callee, args);
    }
    else
    {
      current_value = builder->CreateCall(callee, args, "calltmp");
    }
  }

  void CodeGenerator::visit(AIR::StructInstantiation *node)
  {
    llvm::StructType *struct_type = struct_map[node->struct_id];
    if (!struct_type)
    {
      report_error("Undefined struct: '" + node->struct_name + "'", node->loc);
      current_value = nullptr;
      return;
    }

    llvm::AllocaInst *struct_alloca = builder->CreateAlloca(struct_type, nullptr, "struct_tmp");

    for (size_t i = 0; i < node->field_values.size(); ++i)
    {
      node->field_values[i]->accept(*this);
      if (!current_value)
      {
        report_error("Failed to generate struct field value", node->field_values[i]->loc);
        return;
      }

      llvm::Value *field_ptr = builder->CreateStructGEP(
          struct_type, struct_alloca, static_cast<unsigned>(i), "field_ptr");

      builder->CreateStore(current_value, field_ptr);
    }

    current_value = builder->CreateLoad(struct_type, struct_alloca, "struct_val");
  }

  void CodeGenerator::visit(AIR::FieldAccess *node)
  {
    node->object->accept(*this);
    llvm::Value *object = current_value;

    if (!object)
    {
      report_error("Failed to generate object for field access", node->loc);
      current_value = nullptr;
      return;
    }

    llvm::Type *obj_type = object->getType();
    llvm::StructType *struct_type = nullptr;

    if (obj_type->isStructTy())
    {
      struct_type = llvm::cast<llvm::StructType>(obj_type);
    }
    else if (obj_type->isPointerTy())
    {
      // From LLVM 18, opaque pointers don't have element type
      // We need to track the struct type separately
      // For now, try to extract from the alloca if available
      struct_type = nullptr; // FIXME: needs proper handling
    }

    if (!struct_type)
    {
      report_error("Field access on non-struct type", node->loc);
      current_value = nullptr;
      return;
    }

    llvm::Value *struct_ptr;
    if (obj_type->isStructTy())
    {
      llvm::AllocaInst *tmp_alloca = builder->CreateAlloca(struct_type, nullptr, "tmp_struct");
      builder->CreateStore(object, tmp_alloca);
      struct_ptr = tmp_alloca;
    }
    else
    {
      struct_ptr = object;
    }

    llvm::Value *field_ptr = builder->CreateStructGEP(
        struct_type, struct_ptr, node->field_index, "field_ptr");

    llvm::Type *field_type = get_llvm_type(node->ty);
    current_value = builder->CreateLoad(field_type, field_ptr, node->field_name);
  }

  void CodeGenerator::visit(AIR::VarDecl *node)
  {
    if (node->initializer)
    {
      node->initializer->accept(*this);
      llvm::Value *init_value = current_value;

      if (!init_value)
      {
        report_error("Failed to generate variable initializer", node->loc);
        return;
      }

      llvm::Type *var_type = get_llvm_type(node->var_ty);
      if (!var_type)
      {
        report_error("Cannot resolve variable type", node->loc);
        return;
      }

      llvm::AllocaInst *alloca = create_entry_block_alloca(
          current_function, node->name, var_type);

      builder->CreateStore(init_value, alloca);

      // register variable
      variable_map[node->var_id] = alloca;
    }
    else
    {
      report_error("Variable declaration without initializer", node->loc);
    }
  }

  void CodeGenerator::visit(AIR::Assignment *node)
  {
    node->value->accept(*this);
    llvm::Value *value = current_value;

    if (!value)
    {
      report_error("Failed to generate assignment value", node->loc);
      return;
    }

    auto it = variable_map.find(node->var_id);
    if (it == variable_map.end())
    {
      report_error("Assignment to undefined variable: '" + node->var_name + "'", node->loc);
      return;
    }

    builder->CreateStore(value, it->second);
  }

  void CodeGenerator::visit(AIR::FieldAssignment *node)
  {
    node->object->accept(*this);
    llvm::Value *object = current_value;

    if (!object)
    {
      report_error("Failed to generate object for field assignment", node->loc);
      return;
    }

    node->value->accept(*this);
    llvm::Value *value = current_value;

    if (!value)
    {
      report_error("Failed to generate field assignment value", node->loc);
      return;
    }

    llvm::Type *obj_type = object->getType();
    llvm::StructType *struct_type = nullptr;
    llvm::Value *struct_ptr = nullptr;

    if (obj_type->isStructTy())
    {
      struct_type = llvm::cast<llvm::StructType>(obj_type);
      llvm::AllocaInst *tmp_alloca = builder->CreateAlloca(struct_type, nullptr, "tmp_struct");
      builder->CreateStore(object, tmp_alloca);
      struct_ptr = tmp_alloca;
    }
    else if (obj_type->isPointerTy())
    {
      // In LLVM 18 with opaque pointers, we need to handle this differently
      // For now, assume it's already a pointer to a struct
      struct_ptr = object;
      struct_type = nullptr; // FIXME: needs proper type tracking
    }

    if (!struct_type || !struct_ptr)
    {
      report_error("Field assignment on non-struct type", node->loc);
      return;
    }

    llvm::Value *field_ptr = builder->CreateStructGEP(
        struct_type, struct_ptr, node->field_index, "field_ptr");
    builder->CreateStore(value, field_ptr);
  }

  void CodeGenerator::visit(AIR::Return *node)
  {
    if (node->value)
    {
      node->value->accept(*this);
      if (!current_value)
      {
        report_error("Failed to generate return value", node->loc);
        return;
      }
      builder->CreateRet(current_value);
    }
    else
    {
      builder->CreateRetVoid();
    }
  }

  void CodeGenerator::visit(AIR::If *node)
  {
    node->condition->accept(*this);
    llvm::Value *cond = current_value;

    if (!cond)
    {
      report_error("Failed to generate if condition", node->loc);
      return;
    }

    llvm::BasicBlock *then_block = llvm::BasicBlock::Create(*context, "then", current_function);
    llvm::BasicBlock *else_block = llvm::BasicBlock::Create(*context, "else");
    llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(*context, "ifcont");

    if (node->else_branch.empty())
    {
      builder->CreateCondBr(cond, then_block, merge_block);
    }
    else
    {
      builder->CreateCondBr(cond, then_block, else_block);
    }

    builder->SetInsertPoint(then_block);
    for (const auto &stmt : node->then_branch)
    {
      stmt->accept(*this);
    }
    bool then_has_terminator = builder->GetInsertBlock()->getTerminator() != nullptr;
    if (!then_has_terminator)
    {
      builder->CreateBr(merge_block);
    }

    bool else_has_terminator = false;
    if (!node->else_branch.empty())
    {
      else_block->insertInto(current_function);
      builder->SetInsertPoint(else_block);
      for (const auto &stmt : node->else_branch)
      {
        stmt->accept(*this);
      }
      else_has_terminator = builder->GetInsertBlock()->getTerminator() != nullptr;
      if (!else_has_terminator)
      {
        builder->CreateBr(merge_block);
      }
    }

    bool merge_reachable = !then_has_terminator || !else_has_terminator || node->else_branch.empty();
    if (merge_reachable)
    {
      merge_block->insertInto(current_function);
      builder->SetInsertPoint(merge_block);
    }
    else
    {
      // merge block is unreachable, delete it
      delete merge_block;

      // set insert point to null to indicate unreachable code
      builder->ClearInsertionPoint();
    }
  }

  void CodeGenerator::visit(AIR::ExprStmt *node)
  {
    node->expression->accept(*this);
    // result is discarded for expression statements
  }

  void CodeGenerator::visit(AIR::Function *node)
  {
    // Functions are generated in generate_function_bodies()
    // This visitor method is not used directly
  }

  void CodeGenerator::visit(AIR::StructDecl *node)
  {
    // Structs are generated in generate_struct_types()
    // This visitor method is not used directly
  }

  void CodeGenerator::visit(AIR::Module *node)
  {
    // Module traversal is handled in generate()
    // This visitor method is not used directly
  }

} // namespace Codegen
