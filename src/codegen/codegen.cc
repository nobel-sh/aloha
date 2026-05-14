#include "codegen.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <iostream>
#include "../error/internal.h"

namespace aloha
{
  CodeGenerator::CodeGenerator(TyTable &ty_table, aloha::DiagnosticEngine &diag)
      : context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>("aloha_module", *context)),
        builder(std::make_unique<llvm::IRBuilder<>>(*context)),
        ty_table(ty_table),
        diagnostics(diag),
        current_value(nullptr),
        current_function(nullptr),
        current_air_module(nullptr)
  {
  }

  std::unique_ptr<llvm::Module> CodeGenerator::generate(air::Module *air_module)
  {
    if (!air_module)
    {
      ALOHA_ICE("Null module passed to CodeGenerator::generate");
    }

    current_air_module = air_module;
    module->setSourceFileName(air_module->m_name);

    generate_types();

    // declare all functions for forward references
    declare_functions();

    generate_function_bodies();

    // wrap the main function with a custom entry point
    generate_main_wrapper();

    if (has_errors())
    {
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
    type_map[TyIds::INTEGER] = llvm::Type::getInt64Ty(*context);
    type_map[TyIds::FLOAT] = llvm::Type::getDoubleTy(*context);
    type_map[TyIds::BOOL] = llvm::Type::getInt1Ty(*context);
    type_map[TyIds::VOID] = llvm::Type::getVoidTy(*context);
    type_map[TyIds::STRING] = llvm::PointerType::get(*context, 0);
    type_map[TyIds::ERROR] = llvm::Type::getVoidTy(*context); // Placeholder

    generate_struct_types();
  }

  void CodeGenerator::generate_struct_types()
  {
    if (!current_air_module)
      return;

    // First pass: Create opaque struct types for forward references
    for (const auto &struct_decl : current_air_module->m_structs)
    {
      auto *struct_type = llvm::StructType::create(*context, struct_decl->m_name);
      struct_map[struct_decl->m_struct_id] = struct_type;
      type_map[struct_decl->m_ty_id] = struct_type;
    }

    // Second pass: Fill in struct bodies
    for (const auto &struct_decl : current_air_module->m_structs)
    {
      std::vector<llvm::Type *> field_types;
      for (const auto &field : struct_decl->m_fields)
      {
        llvm::Type *field_type = get_llvm_type(field.m_ty);
        if (!field_type)
        {
          report_error("Cannot resolve field type for '" + field.m_name + "'",
                       field.m_loc);
          field_type = llvm::Type::getInt32Ty(*context); // Fallback
        }
        field_types.push_back(field_type);
      }

      auto *struct_type = struct_map[struct_decl->m_struct_id];
      struct_type->setBody(field_types);
    }
  }

  llvm::Type *CodeGenerator::get_llvm_type(TyId ty_id)
  {
    auto it = type_map.find(ty_id);
    if (it != type_map.end())
    {
      return it->second;
    }

    const TyInfo *ty_info = ty_table.get_ty_info(ty_id);
    if (!ty_info)
    {
      return nullptr;
    }

    if (ty_info->is_struct())
    {
      if (ty_info->m_struct_id.has_value())
      {
        auto struct_it = struct_map.find(ty_info->m_struct_id.value());
        if (struct_it != struct_map.end())
        {
          type_map[ty_id] = struct_it->second;
          return struct_it->second;
        }
      }
    }
    else if (ty_info->is_array())
    {
      if (ty_info->m_type_params.empty())
      {
        return nullptr;
      }

      TyId element_ty_id = ty_info->m_type_params[0];
      llvm::Type *element_llvm_type = get_llvm_type(element_ty_id);
      if (!element_llvm_type)
      {
        return nullptr;
      }

      return llvm::PointerType::get(*context, 0);
    }

    return nullptr;
  }

  void CodeGenerator::declare_functions()
  {
    if (!current_air_module)
      return;

    for (const auto &func : current_air_module->m_functions)
    {
      llvm::FunctionType *func_type = get_function_type(func.get());
      if (!func_type)
      {
        report_error("Cannot create function type for '" + func->m_name + "'", func->m_loc);
        continue;
      }

      llvm::Function::LinkageTypes linkage = func->m_is_extern
                                                 ? llvm::Function::ExternalLinkage
                                                 : llvm::Function::ExternalLinkage;

      std::string llvm_name = func->m_name;
      if (llvm_name == "main")
      {
        llvm_name = "__aloha_main";
      }

      llvm::Function *llvm_func = llvm::Function::Create(
          func_type, linkage, llvm_name, module.get());

      // Set parameter names
      unsigned idx = 0;
      for (auto &arg : llvm_func->args())
      {
        if (idx < func->m_params.size())
        {
          arg.setName(func->m_params[idx].m_name);
        }
        idx++;
      }

      function_map[func->m_func_id] = llvm_func;
    }
  }

  llvm::FunctionType *CodeGenerator::get_function_type(air::Function *func)
  {
    // Get return type
    llvm::Type *return_type = get_llvm_type(func->m_return_ty);
    if (!return_type)
    {
      report_error("Cannot resolve return type for function '" + func->m_name + "'",
                   func->m_loc);
      return nullptr;
    }

    // Get parameter types
    std::vector<llvm::Type *> param_types;
    for (const auto &param : func->m_params)
    {
      llvm::Type *param_type = get_llvm_type(param.m_ty);
      if (!param_type)
      {
        report_error("Cannot resolve parameter type for '" + param.m_name + "'",
                     param.m_loc);
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

    for (const auto &func : current_air_module->m_functions)
    {
      if (!func->m_is_extern)
      {
        generate_function(func.get());
      }
    }
  }

  void CodeGenerator::generate_function(air::Function *func)
  {
    llvm::Function *llvm_func = function_map[func->m_func_id];
    if (!llvm_func)
    {
      report_error("Function '" + func->m_name + "' not declared", func->m_loc);
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
      if (idx < func->m_params.size())
      {
        const auto &param = func->m_params[idx];
        llvm::AllocaInst *alloca = create_entry_block_alloca(
            llvm_func, param.m_name, arg.getType());
        builder->CreateStore(&arg, alloca);
        variable_map[param.m_var_id] = alloca;
      }
      idx++;
    }

    // Generate function body statements
    for (const auto &stmt : func->m_body)
    {
      llvm::BasicBlock *block = builder->GetInsertBlock();
      if (!block || block->getTerminator())
      {
        break;
      }
      stmt->accept(*this);
    }

    // Add return if current block doesn't have a terminator
    llvm::BasicBlock *current_block = builder->GetInsertBlock();
    if (current_block && !current_block->getTerminator())
    {
      if (func->m_return_ty == TyIds::VOID)
      {
        builder->CreateRetVoid();
      }
      else
      {
        // For non-void functions, if there's no terminator, it's an error
        report_error("Function '" + func->m_name + "' missing return statement", func->m_loc);
        // Add a dummy return to prevent LLVM errors
        llvm::Type *ret_type = get_llvm_type(func->m_return_ty);
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

  void CodeGenerator::generate_main_wrapper()
  {
    auto it = std::find_if(
        current_air_module->m_functions.begin(),
        current_air_module->m_functions.end(),
        [](const std::unique_ptr<air::Function> &f)
        { return f->m_name == "main"; });

    if (it == current_air_module->m_functions.end())
    {
      return; // No main function to wrap
    }

    air::Function *main_func = it->get();

    llvm::FunctionType *main_wrapper_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context), false);
    llvm::Function *main_wrapper_func = llvm::Function::Create(
        main_wrapper_type,
        llvm::Function::ExternalLinkage,
        "main",
        module.get());

    llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(*context, "entry", main_wrapper_func);
    builder->SetInsertPoint(entry_block);

    llvm::Function *llvm_main_func = function_map[main_func->m_func_id];
    if (!llvm_main_func)
    {
      report_error("Function 'main' not declared", main_func->m_loc);
      return;
    }
    llvm::Value *ret_value = builder->CreateCall(llvm_main_func, {});

    if (main_func->m_return_ty == TyIds::VOID)
    {
      builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
    }
    else
    {
      builder->CreateRet(ret_value);
    }
  }

  llvm::AllocaInst *CodeGenerator::create_entry_block_alloca(
      llvm::Function *func, const std::string &var_name, llvm::Type *type)
  {
    if (func->empty())
    {
      return nullptr; // No entry block yet
    }
    llvm::IRBuilder<> tmp_builder(&func->getEntryBlock(),
                                  func->getEntryBlock().begin());
    return tmp_builder.CreateAlloca(type, nullptr, var_name);
  }

  void CodeGenerator::visit(air::IntegerLiteral *node)
  {
    uint64_t value = static_cast<uint64_t>(node->m_value);
    current_value = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), value, true);
  }

  void CodeGenerator::visit(air::FloatLiteral *node)
  {
    current_value = llvm::ConstantFP::get(*context, llvm::APFloat(node->m_value));
  }

  void CodeGenerator::visit(air::StringLiteral *node)
  {
    llvm::Constant *str_constant = llvm::ConstantDataArray::getString(*context, node->m_value);
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

  void CodeGenerator::visit(air::BoolLiteral *node)
  {
    current_value = llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), node->m_value ? 1 : 0);
  }

  void CodeGenerator::visit(air::VarRef *node)
  {
    auto it = variable_map.find(node->m_var_id);
    if (it == variable_map.end())
    {
      report_error("Undefined variable: '" + node->m_name + "'", node->m_loc);
      current_value = nullptr;
      return;
    }

    llvm::AllocaInst *alloca = it->second;
    current_value = builder->CreateLoad(alloca->getAllocatedType(), alloca, node->m_name);
  }

  enum class NumericKind
  {
    INTEGER,
    FLOAT,
    BOOL,
    OTHER
  };

  NumericKind get_numeric_kind(TyId ty_id)
  {
    if (ty_id == TyIds::INTEGER)
      return NumericKind::INTEGER;
    if (ty_id == TyIds::FLOAT)
      return NumericKind::FLOAT;
    if (ty_id == TyIds::BOOL)
      return NumericKind::BOOL;
    return NumericKind::OTHER;
  }

  void CodeGenerator::visit(air::BinaryOp *node)
  {
    current_value = nullptr; // set on success

    node->m_left->accept(*this);
    llvm::Value *left = current_value;

    if (node->m_op == air::BinaryOpKind::LOGICAL_AND || node->m_op == air::BinaryOpKind::LOGICAL_OR)
    {
      if (!left)
      {
        report_error("Failed to generate left operand for short-circuit operation", node->m_loc);
        return;
      }

      llvm::BasicBlock *left_block = builder->GetInsertBlock();
      llvm::BasicBlock *right_block =
          llvm::BasicBlock::Create(*context, node->m_op == air::BinaryOpKind::LOGICAL_AND ? "and.rhs" : "or.rhs", current_function);
      llvm::BasicBlock *merge_block =
          llvm::BasicBlock::Create(*context, node->m_op == air::BinaryOpKind::LOGICAL_AND ? "and.end" : "or.end");

      if (node->m_op == air::BinaryOpKind::LOGICAL_AND)
      {
        builder->CreateCondBr(left, right_block, merge_block);
      }
      else
      {
        builder->CreateCondBr(left, merge_block, right_block);
      }

      builder->SetInsertPoint(right_block);
      node->m_right->accept(*this);
      llvm::Value *right = current_value;
      if (!right)
      {
        report_error("Failed to generate right operand for short-circuit operation", node->m_loc);
        return;
      }

      llvm::BasicBlock *right_end_block = builder->GetInsertBlock();
      if (right_end_block && !right_end_block->getTerminator())
      {
        builder->CreateBr(merge_block);
      }

      merge_block->insertInto(current_function);
      builder->SetInsertPoint(merge_block);

      llvm::PHINode *phi = builder->CreatePHI(llvm::Type::getInt1Ty(*context), 2,
                                              node->m_op == air::BinaryOpKind::LOGICAL_AND ? "andtmp" : "ortmp");
      if (node->m_op == air::BinaryOpKind::LOGICAL_AND)
      {
        phi->addIncoming(llvm::ConstantInt::getFalse(*context), left_block);
      }
      else
      {
        phi->addIncoming(llvm::ConstantInt::getTrue(*context), left_block);
      }
      phi->addIncoming(right, right_end_block);
      current_value = phi;
      return;
    }

    node->m_right->accept(*this);
    llvm::Value *right = current_value;

    if (!left || !right)
    {
      report_error("Failed to generate binary operation operands", node->m_loc);
      return;
    }

    NumericKind kind = get_numeric_kind(node->m_left->m_ty);

    switch (node->m_op)
    {
    case air::BinaryOpKind::ADD:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateAdd(left, right, "addtmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFAdd(left, right, "addtmp");
      else
        report_error("Unsupported type for addition", node->m_loc);
      break;

    case air::BinaryOpKind::SUB:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateSub(left, right, "subtmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFSub(left, right, "subtmp");
      else
        report_error("Unsupported type for subtraction", node->m_loc);
      break;

    case air::BinaryOpKind::MUL:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateMul(left, right, "multmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFMul(left, right, "multmp");
      else
        report_error("Unsupported type for multiplication", node->m_loc);
      break;

    case air::BinaryOpKind::DIV:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateSDiv(left, right, "divtmp"); // signed division
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFDiv(left, right, "divtmp");
      else
        report_error("Unsupported type for division", node->m_loc);
      break;

    case air::BinaryOpKind::MOD:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateSRem(left, right, "modtmp"); // signed remainder
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFRem(left, right, "modtmp");
      else
        report_error("Unsupported type for modulo", node->m_loc);
      break;

    case air::BinaryOpKind::EQ:
      if (kind == NumericKind::INTEGER || kind == NumericKind::BOOL)
        current_value = builder->CreateICmpEQ(left, right, "eqtmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOEQ(left, right, "eqtmp");
      else
        report_error("Unsupported type for equality comparison", node->m_loc);
      break;

    case air::BinaryOpKind::NE:
      if (kind == NumericKind::INTEGER || kind == NumericKind::BOOL)
        current_value = builder->CreateICmpNE(left, right, "netmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpONE(left, right, "netmp");
      else
        report_error("Unsupported type for inequality comparison", node->m_loc);
      break;

    case air::BinaryOpKind::LT:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateICmpSLT(left, right, "lttmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOLT(left, right, "lttmp");
      else
        report_error("Unsupported type for less-than comparison", node->m_loc);
      break;

    case air::BinaryOpKind::LE:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateICmpSLE(left, right, "letmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOLE(left, right, "letmp");
      else
        report_error("Unsupported type for less-equal comparison", node->m_loc);
      break;

    case air::BinaryOpKind::GT:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateICmpSGT(left, right, "gttmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOGT(left, right, "gttmp");
      else
        report_error("Unsupported type for greater-than comparison", node->m_loc);
      break;

    case air::BinaryOpKind::GE:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateICmpSGE(left, right, "getmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOGE(left, right, "getmp");
      else
        report_error("Unsupported type for greater-equal comparison", node->m_loc);
      break;

    case air::BinaryOpKind::LOGICAL_AND:
      current_value = builder->CreateAnd(left, right, "andtmp");
      break;

    case air::BinaryOpKind::LOGICAL_OR:
      current_value = builder->CreateOr(left, right, "ortmp");
      break;

    default:
      report_error("Unknown binary operation", node->m_loc);
      break;
    }
  }

  void CodeGenerator::visit(air::UnaryOp *node)
  {
    current_value = nullptr; // set on success

    node->m_operand->accept(*this);
    llvm::Value *operand = current_value;

    if (!operand)
    {
      report_error("Failed to generate unary operation operand", node->m_loc);
      return;
    }

    NumericKind kind = get_numeric_kind(node->m_operand->m_ty);

    switch (node->m_op)
    {
    case air::UnaryOpKind::NEG:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateNeg(operand, "negtmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFNeg(operand, "negtmp");
      else
        report_error("Unsupported type for negation", node->m_loc);
      break;

    case air::UnaryOpKind::NOT:
      current_value = builder->CreateNot(operand, "nottmp");
      break;

    default:
      report_error("Unknown unary operation", node->m_loc);
      break;
    }
  }

  void CodeGenerator::visit(air::Call *node)
  {
    llvm::Function *callee = function_map[node->m_func_id];
    if (!callee)
    {
      report_error("Undefined function: '" + node->m_function_name + "'", node->m_loc);
      current_value = nullptr;
      return;
    }

    std::vector<llvm::Value *> args;
    for (const auto &arg : node->m_arguments)
    {
      arg->accept(*this);
      if (!current_value)
      {
        report_error("Failed to generate function argument", arg->m_loc);
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

  void CodeGenerator::visit(air::StructInstantiation *node)
  {
    llvm::StructType *struct_type = struct_map[node->m_struct_id];
    if (!struct_type)
    {
      report_error("Undefined struct: '" + node->m_struct_name + "'", node->m_loc);
      current_value = nullptr;
      return;
    }

    llvm::AllocaInst *struct_alloca = builder->CreateAlloca(struct_type, nullptr, "struct_tmp");

    for (size_t i = 0; i < node->m_field_values.size(); ++i)
    {
      node->m_field_values[i]->accept(*this);
      if (!current_value)
      {
        report_error("Failed to generate struct field value", node->m_field_values[i]->m_loc);
        return;
      }

      llvm::Value *field_ptr = builder->CreateStructGEP(
          struct_type, struct_alloca, static_cast<unsigned>(i), "field_ptr");

      builder->CreateStore(current_value, field_ptr);
    }

    current_value = builder->CreateLoad(struct_type, struct_alloca, "struct_val");
  }

  void CodeGenerator::visit(air::FieldAccess *node)
  {
    node->m_object->accept(*this);
    llvm::Value *object = current_value;

    if (!object)
    {
      report_error("Failed to generate object for field access", node->m_loc);
      current_value = nullptr;
      return;
    }

    const TyInfo *obj_ty_info = ty_table.get_ty_info(node->m_object->m_ty);
    if (!obj_ty_info || !obj_ty_info->is_struct())
    {
      report_error("Field access on non-struct type", node->m_loc);
      current_value = nullptr;
      return;
    }

    llvm::StructType *struct_type = struct_map[obj_ty_info->m_struct_id.value()];
    if (!struct_type)
    {
      report_error("Struct type not found in Struct mapping from AirTy -> LLVM Type", node->m_loc);
      current_value = nullptr;
      return;
    }

    llvm::Type *obj_llvm_type = object->getType();
    if (!obj_llvm_type->isStructTy())
    {
      report_error("Expected struct value for field access", node->m_loc);
      current_value = nullptr;
      return;
    }

    llvm::AllocaInst *tmp_alloca = builder->CreateAlloca(struct_type, nullptr, "tmp_struct");
    builder->CreateStore(object, tmp_alloca);
    llvm::Value *struct_ptr = tmp_alloca;

    llvm::Value *field_ptr = builder->CreateStructGEP(
        struct_type, struct_ptr, node->m_field_index, "field_ptr");

    llvm::Type *field_type = get_llvm_type(node->m_ty);
    current_value = builder->CreateLoad(field_type, field_ptr, node->m_field_name);
  }

  void CodeGenerator::visit(air::ArrayExpr *node)
  {
    if (node->m_elements.empty())
    {
      report_error("Empty arrays not yet supported", node->m_loc);
      current_value = nullptr;
      return;
    }

    if (!current_function)
    {
      report_error("Array literals only supported inside functions", node->m_loc);
      current_value = nullptr;
      return;
    }

    node->m_elements[0]->accept(*this);
    if (!current_value)
    {
      report_error("Failed to generate first array element", node->m_loc);
      return;
    }

    llvm::Type *element_type = current_value->getType();
    size_t array_size = node->m_elements.size();

    llvm::ArrayType *array_type = llvm::ArrayType::get(element_type, array_size);

    llvm::AllocaInst *array_alloca = create_entry_block_alloca(
        current_function, "array_tmp", array_type);

    if (!array_alloca)
    {
      report_error("Failed to allocate array storage", node->m_loc);
      current_value = nullptr;
      return;
    }

    size_t index = 0;
    for (const auto &element : node->m_elements)
    {
      element->accept(*this);
      if (!current_value)
      {
        report_error("Failed to generate array element at index " + std::to_string(index), element->m_loc);
        return;
      }

      llvm::Value *index_val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), index);
      llvm::Value *element_ptr = builder->CreateGEP(
          array_type, array_alloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0), index_val},
          "element_ptr");

      builder->CreateStore(current_value, element_ptr);
      index++;
    }

    current_value = array_alloca;
  }

  void CodeGenerator::visit(air::ArrayAccess *node)
  {
    node->m_array_expr->accept(*this);
    llvm::Value *array = current_value;

    if (!array)
    {
      report_error("Failed to generate array for access", node->m_loc);
      current_value = nullptr;
      return;
    }

    node->m_index_expr->accept(*this);
    llvm::Value *index = current_value;

    if (!index)
    {
      report_error("Failed to generate index for array access", node->m_loc);
      current_value = nullptr;
      return;
    }

    llvm::Type *element_type = get_llvm_type(node->m_ty);
    if (!element_type)
    {
      report_error("Cannot resolve array element type", node->m_loc);
      current_value = nullptr;
      return;
    }

    llvm::Value *element_ptr = builder->CreateGEP(
        element_type, array, index, "element_ptr");

    current_value = builder->CreateLoad(element_type, element_ptr, "array_elem");
  }

  void CodeGenerator::visit(air::VarDecl *node)
  {
    if (node->m_initializer)
    {
      node->m_initializer->accept(*this);
      llvm::Value *init_value = current_value;

      if (!init_value)
      {
        report_error("Failed to generate variable initializer", node->m_loc);
        return;
      }

      // For arrays or if type is ERROR, use the value's type directly
      llvm::Type *var_type = nullptr;
      if (node->m_var_ty == TyIds::ERROR)
      {
        var_type = init_value->getType();
      }
      else
      {
        var_type = get_llvm_type(node->m_var_ty);
      }

      if (!var_type)
      {
        report_error("Cannot resolve variable type", node->m_loc);
        return;
      }

      llvm::AllocaInst *alloca = create_entry_block_alloca(
          current_function, node->m_name, var_type);

      if (!alloca)
      {
        report_error("Failed to create variable storage", node->m_loc);
        return;
      }

      builder->CreateStore(init_value, alloca);

      // register variable
      variable_map[node->m_var_id] = alloca;
    }
    else
    {
      report_error("Variable declaration without initializer", node->m_loc);
    }
  }

  void CodeGenerator::visit(air::Assignment *node)
  {
    node->m_value->accept(*this);
    llvm::Value *value = current_value;

    if (!value)
    {
      report_error("Failed to generate assignment value", node->m_loc);
      return;
    }

    auto it = variable_map.find(node->m_var_id);
    if (it == variable_map.end())
    {
      report_error("Assignment to undefined variable: '" + node->m_var_name + "'", node->m_loc);
      return;
    }

    builder->CreateStore(value, it->second);
  }

  void CodeGenerator::visit(air::ArrayAssignment *node)
  {
    auto array_it = variable_map.find(node->m_array_var_id);
    if (array_it == variable_map.end())
    {
      report_error("Array assignment to undefined variable: '" + node->m_array_name + "'", node->m_loc);
      return;
    }

    node->m_index->accept(*this);
    llvm::Value *index = current_value;
    if (!index)
    {
      report_error("Failed to generate index for array assignment", node->m_loc);
      return;
    }

    node->m_value->accept(*this);
    llvm::Value *value = current_value;
    if (!value)
    {
      report_error("Failed to generate value for array assignment", node->m_loc);
      return;
    }

    llvm::Type *element_type = get_llvm_type(node->m_element_ty);
    if (!element_type)
    {
      report_error("Cannot resolve array element type", node->m_loc);
      return;
    }

    llvm::Value *array_ptr = builder->CreateLoad(array_it->second->getAllocatedType(),
                                                 array_it->second, "array_ptr");
    llvm::Value *element_ptr = builder->CreateGEP(
        element_type, array_ptr, index, "element_ptr");

    builder->CreateStore(value, element_ptr);
  }

  void CodeGenerator::visit(air::FieldAssignment *node)
  {
    node->m_object->accept(*this);
    llvm::Value *object = current_value;

    if (!object)
    {
      report_error("Failed to generate object for field assignment", node->m_loc);
      return;
    }

    node->m_value->accept(*this);
    llvm::Value *value = current_value;

    if (!value)
    {
      report_error("Failed to generate field assignment value", node->m_loc);
      return;
    }

    const TyInfo *obj_ty_info = ty_table.get_ty_info(node->m_object->m_ty);
    if (!obj_ty_info || !obj_ty_info->is_struct())
    {
      report_error("Field assignment on non-struct type", node->m_loc);
      return;
    }

    llvm::StructType *struct_type = struct_map[obj_ty_info->m_struct_id.value()];
    if (!struct_type)
    {
      report_error("Struct type not found in codegen", node->m_loc);
      return;
    }

    llvm::Type *obj_llvm_type = object->getType();
    if (!obj_llvm_type->isStructTy())
    {
      report_error("Expected struct value for field assignment", node->m_loc);
      return;
    }

    llvm::AllocaInst *tmp_alloca = builder->CreateAlloca(struct_type, nullptr, "tmp_struct");
    builder->CreateStore(object, tmp_alloca);
    llvm::Value *struct_ptr = tmp_alloca;

    llvm::Value *field_ptr = builder->CreateStructGEP(
        struct_type, struct_ptr, node->m_field_index, "field_ptr");
    builder->CreateStore(value, field_ptr);
  }

  void CodeGenerator::visit(air::Return *node)
  {
    if (node->m_value)
    {
      node->m_value->accept(*this);
      if (!current_value)
      {
        report_error("Failed to generate return value", node->m_loc);
        return;
      }
      builder->CreateRet(current_value);
    }
    else
    {
      builder->CreateRetVoid();
    }
  }

  void CodeGenerator::visit(air::If *node)
  {
    node->m_condition->accept(*this);
    llvm::Value *cond = current_value;

    if (!cond)
    {
      report_error("Failed to generate if condition", node->m_loc);
      return;
    }

    llvm::BasicBlock *then_block = llvm::BasicBlock::Create(*context, "then", current_function);
    llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(*context, "ifcont");
    llvm::BasicBlock *else_block = nullptr;

    if (node->m_else_branch.empty())
    {
      builder->CreateCondBr(cond, then_block, merge_block);
    }
    else
    {
      else_block = llvm::BasicBlock::Create(*context, "else");
      builder->CreateCondBr(cond, then_block, else_block);
    }

    builder->SetInsertPoint(then_block);
    for (const auto &stmt : node->m_then_branch)
    {
      llvm::BasicBlock *block = builder->GetInsertBlock();
      if (!block || block->getTerminator())
      {
        break;
      }
      stmt->accept(*this);
    }
    llvm::BasicBlock *then_end_block = builder->GetInsertBlock();
    // If insertion point is cleared (null), it means all paths in the branch have terminated
    bool then_has_terminator = !then_end_block || (then_end_block && then_end_block->getTerminator() != nullptr);
    if (!then_has_terminator && then_end_block)
    {
      builder->CreateBr(merge_block);
    }

    bool else_has_terminator = false;
    if (!node->m_else_branch.empty())
    {
      else_block->insertInto(current_function);
      builder->SetInsertPoint(else_block);
      for (const auto &stmt : node->m_else_branch)
      {
        llvm::BasicBlock *block = builder->GetInsertBlock();
        if (!block || block->getTerminator())
        {
          break;
        }
        stmt->accept(*this);
      }
      llvm::BasicBlock *else_end_block = builder->GetInsertBlock();
      // If insertion point is cleared (null), it means all paths in the branch have terminated
      else_has_terminator = !else_end_block || (else_end_block && else_end_block->getTerminator() != nullptr);
      if (!else_has_terminator && else_end_block)
      {
        builder->CreateBr(merge_block);
      }
    }

    bool merge_reachable = !then_has_terminator || !else_has_terminator || node->m_else_branch.empty();
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

  void CodeGenerator::visit(air::Break *node)
  {
    if (break_blocks.empty())
    {
      report_error("'break' can only be used inside a loop", node->m_loc);
      return;
    }

    builder->CreateBr(break_blocks.back());
  }

  void CodeGenerator::visit(air::Continue *node)
  {
    if (continue_blocks.empty())
    {
      report_error("'continue' can only be used inside a loop", node->m_loc);
      return;
    }

    builder->CreateBr(continue_blocks.back());
  }

  void CodeGenerator::visit(air::While *node)
  {
    llvm::BasicBlock *condition_block =
        llvm::BasicBlock::Create(*context, "while.cond", current_function);
    llvm::BasicBlock *body_block =
        llvm::BasicBlock::Create(*context, "while.body");
    llvm::BasicBlock *after_block =
        llvm::BasicBlock::Create(*context, "while.end");

    builder->CreateBr(condition_block);

    builder->SetInsertPoint(condition_block);
    node->m_condition->accept(*this);
    llvm::Value *cond = current_value;
    if (!cond)
    {
      report_error("Failed to generate while condition", node->m_loc);
      return;
    }
    builder->CreateCondBr(cond, body_block, after_block);

    body_block->insertInto(current_function);
    builder->SetInsertPoint(body_block);
    break_blocks.push_back(after_block);
    continue_blocks.push_back(condition_block);

    for (const auto &stmt : node->m_body)
    {
      llvm::BasicBlock *block = builder->GetInsertBlock();
      if (!block || block->getTerminator())
      {
        break;
      }
      stmt->accept(*this);
    }

    break_blocks.pop_back();
    continue_blocks.pop_back();

    llvm::BasicBlock *body_end_block = builder->GetInsertBlock();
    if (body_end_block && !body_end_block->getTerminator())
    {
      builder->CreateBr(condition_block);
    }

    after_block->insertInto(current_function);
    builder->SetInsertPoint(after_block);
  }

  void CodeGenerator::visit(air::ExprStmt *node)
  {
    node->m_expression->accept(*this);
    // result is discarded for expression statements
  }

  void CodeGenerator::visit(air::Function *node)
  {
    // Functions are generated in generate_function_bodies()
    // This visitor method is not used directly
  }

  void CodeGenerator::visit(air::StructDecl *node)
  {
    // Structs are generated in generate_struct_types()
    // This visitor method is not used directly
  }

  void CodeGenerator::visit(air::Module *node)
  {
    // Module traversal is handled in generate()
    // This visitor method is not used directly
  }
} // namespace aloha
