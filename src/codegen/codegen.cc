#include "codegen.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <iostream>
#include "../error/internal.h"

namespace Codegen
{
  CodeGenerator::CodeGenerator(AIR::TyTable &ty_table, aloha::DiagnosticEngine &diag)
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

  std::unique_ptr<llvm::Module> CodeGenerator::generate(AIR::Module *air_module)
  {
    if (!air_module)
    {
      ALOHA_ICE("Null module passed to CodeGenerator::generate");
    }

    current_air_module = air_module;
    module->setSourceFileName(air_module->name);

    generate_types();

    // declare all functions for forward references
    declare_functions();

    generate_function_bodies();

    // wrap the main function with a custom entry point
    generate_main_wrapper();

    if (has_errors())
    {
      diagnostics.print_all();
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
    type_map[AIR::TyIds::INTEGER] = llvm::Type::getInt64Ty(*context);
    type_map[AIR::TyIds::FLOAT] = llvm::Type::getDoubleTy(*context);
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

    const AIR::TyInfo *ty_info = ty_table.get_ty_info(ty_id);
    if (!ty_info)
    {
      return nullptr;
    }

    if (ty_info->is_struct())
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
    else if (ty_info->is_array())
    {
      if (ty_info->type_params.empty())
      {
        return nullptr;
      }

      AIR::TyId element_ty_id = ty_info->type_params[0];
      llvm::Type *element_llvm_type = get_llvm_type(element_ty_id);
      if (!element_llvm_type)
      {
        return nullptr;
      }

      return element_llvm_type->getPointerTo();
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

      std::string llvm_name = func->name;
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

  void CodeGenerator::generate_main_wrapper()
  {
    auto it = std::find_if(
        current_air_module->functions.begin(),
        current_air_module->functions.end(),
        [](const std::unique_ptr<AIR::Function> &f)
        { return f->name == "main"; });

    if (it == current_air_module->functions.end())
    {
      return; // No main function to wrap
    }

    AIR::Function *main_func = it->get();

    llvm::FunctionType *main_wrapper_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context), false);
    llvm::Function *main_wrapper_func = llvm::Function::Create(
        main_wrapper_type,
        llvm::Function::ExternalLinkage,
        "main",
        module.get());

    llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(*context, "entry", main_wrapper_func);
    builder->SetInsertPoint(entry_block);

    llvm::Function *llvm_main_func = function_map[main_func->func_id];
    if (!llvm_main_func)
    {
      report_error("Function 'main' not declared", main_func->loc);
      return;
    }
    llvm::Value *ret_value = builder->CreateCall(llvm_main_func, {});

    if (main_func->return_ty == AIR::TyIds::VOID)
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

  void CodeGenerator::visit(AIR::IntegerLiteral *node)
  {
    uint64_t value = static_cast<uint64_t>(node->value);
    current_value = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), value, true);
  }

  void CodeGenerator::visit(AIR::FloatLiteral *node)
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

  enum class NumericKind
  {
    INTEGER,
    FLOAT,
    BOOL,
    OTHER
  };

  NumericKind get_numeric_kind(AIR::TyId ty_id)
  {
    if (ty_id == AIR::TyIds::INTEGER)
      return NumericKind::INTEGER;
    if (ty_id == AIR::TyIds::FLOAT)
      return NumericKind::FLOAT;
    if (ty_id == AIR::TyIds::BOOL)
      return NumericKind::BOOL;
    return NumericKind::OTHER;
  }

  void CodeGenerator::visit(AIR::BinaryOp *node)
  {
    current_value = nullptr; // set on success

    node->left->accept(*this);
    llvm::Value *left = current_value;

    node->right->accept(*this);
    llvm::Value *right = current_value;

    if (!left || !right)
    {
      report_error("Failed to generate binary operation operands", node->loc);
      return;
    }

    NumericKind kind = get_numeric_kind(node->left->ty);

    switch (node->op)
    {
    case AIR::BinaryOpKind::ADD:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateAdd(left, right, "addtmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFAdd(left, right, "addtmp");
      else
        report_error("Unsupported type for addition", node->loc);
      break;

    case AIR::BinaryOpKind::SUB:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateSub(left, right, "subtmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFSub(left, right, "subtmp");
      else
        report_error("Unsupported type for subtraction", node->loc);
      break;

    case AIR::BinaryOpKind::MUL:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateMul(left, right, "multmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFMul(left, right, "multmp");
      else
        report_error("Unsupported type for multiplication", node->loc);
      break;

    case AIR::BinaryOpKind::DIV:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateSDiv(left, right, "divtmp"); // signed division
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFDiv(left, right, "divtmp");
      else
        report_error("Unsupported type for division", node->loc);
      break;

    case AIR::BinaryOpKind::MOD:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateSRem(left, right, "modtmp"); // signed remainder
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFRem(left, right, "modtmp");
      else
        report_error("Unsupported type for modulo", node->loc);
      break;

    case AIR::BinaryOpKind::EQ:
      if (kind == NumericKind::INTEGER || kind == NumericKind::BOOL)
        current_value = builder->CreateICmpEQ(left, right, "eqtmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOEQ(left, right, "eqtmp");
      else
        report_error("Unsupported type for equality comparison", node->loc);
      break;

    case AIR::BinaryOpKind::NE:
      if (kind == NumericKind::INTEGER || kind == NumericKind::BOOL)
        current_value = builder->CreateICmpNE(left, right, "netmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpONE(left, right, "netmp");
      else
        report_error("Unsupported type for inequality comparison", node->loc);
      break;

    case AIR::BinaryOpKind::LT:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateICmpSLT(left, right, "lttmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOLT(left, right, "lttmp");
      else
        report_error("Unsupported type for less-than comparison", node->loc);
      break;

    case AIR::BinaryOpKind::LE:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateICmpSLE(left, right, "letmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOLE(left, right, "letmp");
      else
        report_error("Unsupported type for less-equal comparison", node->loc);
      break;

    case AIR::BinaryOpKind::GT:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateICmpSGT(left, right, "gttmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOGT(left, right, "gttmp");
      else
        report_error("Unsupported type for greater-than comparison", node->loc);
      break;

    case AIR::BinaryOpKind::GE:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateICmpSGE(left, right, "getmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFCmpOGE(left, right, "getmp");
      else
        report_error("Unsupported type for greater-equal comparison", node->loc);
      break;

    case AIR::BinaryOpKind::AND:
      current_value = builder->CreateAnd(left, right, "andtmp");
      break;

    case AIR::BinaryOpKind::OR:
      current_value = builder->CreateOr(left, right, "ortmp");
      break;

    default:
      report_error("Unknown binary operation", node->loc);
      break;
    }
  }

  void CodeGenerator::visit(AIR::UnaryOp *node)
  {
    current_value = nullptr; // set on success

    node->operand->accept(*this);
    llvm::Value *operand = current_value;

    if (!operand)
    {
      report_error("Failed to generate unary operation operand", node->loc);
      return;
    }

    NumericKind kind = get_numeric_kind(node->operand->ty);

    switch (node->op)
    {
    case AIR::UnaryOpKind::NEG:
      if (kind == NumericKind::INTEGER)
        current_value = builder->CreateNeg(operand, "negtmp");
      else if (kind == NumericKind::FLOAT)
        current_value = builder->CreateFNeg(operand, "negtmp");
      else
        report_error("Unsupported type for negation", node->loc);
      break;

    case AIR::UnaryOpKind::NOT:
      current_value = builder->CreateNot(operand, "nottmp");
      break;

    default:
      report_error("Unknown unary operation", node->loc);
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

    const AIR::TyInfo *obj_ty_info = ty_table.get_ty_info(node->object->ty);
    if (!obj_ty_info || !obj_ty_info->is_struct())
    {
      report_error("Field access on non-struct type", node->loc);
      current_value = nullptr;
      return;
    }

    llvm::StructType *struct_type = struct_map[obj_ty_info->struct_id.value()];
    if (!struct_type)
    {
      report_error("Struct type not found in Struct mapping from AirTy -> LLVM Type", node->loc);
      current_value = nullptr;
      return;
    }

    llvm::Type *obj_llvm_type = object->getType();
    if (!obj_llvm_type->isStructTy())
    {
      report_error("Expected struct value for field access", node->loc);
      current_value = nullptr;
      return;
    }

    llvm::AllocaInst *tmp_alloca = builder->CreateAlloca(struct_type, nullptr, "tmp_struct");
    builder->CreateStore(object, tmp_alloca);
    llvm::Value *struct_ptr = tmp_alloca;

    llvm::Value *field_ptr = builder->CreateStructGEP(
        struct_type, struct_ptr, node->field_index, "field_ptr");

    llvm::Type *field_type = get_llvm_type(node->ty);
    current_value = builder->CreateLoad(field_type, field_ptr, node->field_name);
  }

  void CodeGenerator::visit(AIR::ArrayExpr *node)
  {
    if (node->elements.empty())
    {
      report_error("Empty arrays not yet supported", node->loc);
      current_value = nullptr;
      return;
    }

    if (!current_function)
    {
      report_error("Array literals only supported inside functions", node->loc);
      current_value = nullptr;
      return;
    }

    node->elements[0]->accept(*this);
    if (!current_value)
    {
      report_error("Failed to generate first array element", node->loc);
      return;
    }

    llvm::Type *element_type = current_value->getType();
    size_t array_size = node->elements.size();

    llvm::ArrayType *array_type = llvm::ArrayType::get(element_type, array_size);

    llvm::AllocaInst *array_alloca = create_entry_block_alloca(
        current_function, "array_tmp", array_type);

    if (!array_alloca)
    {
      report_error("Failed to allocate array storage", node->loc);
      current_value = nullptr;
      return;
    }

    size_t index = 0;
    for (const auto &element : node->elements)
    {
      element->accept(*this);
      if (!current_value)
      {
        report_error("Failed to generate array element at index " + std::to_string(index), element->loc);
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

  void CodeGenerator::visit(AIR::ArrayAccess *node)
  {
    node->array_expr->accept(*this);
    llvm::Value *array = current_value;

    if (!array)
    {
      report_error("Failed to generate array for access", node->loc);
      current_value = nullptr;
      return;
    }

    node->index_expr->accept(*this);
    llvm::Value *index = current_value;

    if (!index)
    {
      report_error("Failed to generate index for array access", node->loc);
      current_value = nullptr;
      return;
    }

    llvm::Type *element_type = get_llvm_type(node->ty);
    if (!element_type)
    {
      report_error("Cannot resolve array element type", node->loc);
      current_value = nullptr;
      return;
    }

    llvm::Value *element_ptr = builder->CreateGEP(
        element_type, array, index, "element_ptr");

    current_value = builder->CreateLoad(element_type, element_ptr, "array_elem");
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

      // For arrays or if type is ERROR, use the value's type directly
      llvm::Type *var_type = nullptr;
      if (node->var_ty == AIR::TyIds::ERROR)
      {
        var_type = init_value->getType();
      }
      else
      {
        var_type = get_llvm_type(node->var_ty);
      }

      if (!var_type)
      {
        report_error("Cannot resolve variable type", node->loc);
        return;
      }

      llvm::AllocaInst *alloca = create_entry_block_alloca(
          current_function, node->name, var_type);

      if (!alloca)
      {
        report_error("Failed to create variable storage", node->loc);
        return;
      }

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

    const AIR::TyInfo *obj_ty_info = ty_table.get_ty_info(node->object->ty);
    if (!obj_ty_info || !obj_ty_info->is_struct())
    {
      report_error("Field assignment on non-struct type", node->loc);
      return;
    }

    llvm::StructType *struct_type = struct_map[obj_ty_info->struct_id.value()];
    if (!struct_type)
    {
      report_error("Struct type not found in codegen", node->loc);
      return;
    }

    llvm::Type *obj_llvm_type = object->getType();
    if (!obj_llvm_type->isStructTy())
    {
      report_error("Expected struct value for field assignment", node->loc);
      return;
    }

    llvm::AllocaInst *tmp_alloca = builder->CreateAlloca(struct_type, nullptr, "tmp_struct");
    builder->CreateStore(object, tmp_alloca);
    llvm::Value *struct_ptr = tmp_alloca;

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
    llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(*context, "ifcont");
    llvm::BasicBlock *else_block = nullptr;

    if (node->else_branch.empty())
    {
      builder->CreateCondBr(cond, then_block, merge_block);
    }
    else
    {
      else_block = llvm::BasicBlock::Create(*context, "else");
      builder->CreateCondBr(cond, then_block, else_block);
    }

    builder->SetInsertPoint(then_block);
    for (const auto &stmt : node->then_branch)
    {
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
    if (!node->else_branch.empty())
    {
      else_block->insertInto(current_function);
      builder->SetInsertPoint(else_block);
      for (const auto &stmt : node->else_branch)
      {
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
