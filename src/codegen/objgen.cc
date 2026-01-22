#include "objgen.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>
#include <system_error>

using namespace llvm;

void optimize_module(llvm::Module *module)
{
  legacy::PassManager pass_manager;

  // Memory to register promotion (critical for removing alloca/load/store)
  pass_manager.add(createPromoteMemoryToRegisterPass());

  // Instruction combining and simplification
  pass_manager.add(createInstructionCombiningPass());

  // Control flow simplification
  pass_manager.add(createCFGSimplificationPass());

  // Dead code elimination
  pass_manager.add(createDeadCodeEliminationPass());

  // Global Value Numbering (common subexpression elimination)
  pass_manager.add(createGVNPass());

  // Run the passes
  pass_manager.run(*module);

  // Verify the module after optimization
  verifyModule(*module);
}

void emit_object_file(llvm::Module *module, const std::string &output_path)
{
  InitializeNativeTarget();
  InitializeNativeTargetAsmParser();
  InitializeNativeTargetAsmPrinter();

  std::string target_triple = sys::getDefaultTargetTriple();
  module->setTargetTriple(target_triple);

  std::string error;
  auto target = TargetRegistry::lookupTarget(target_triple, error);
  if (!target)
  {
    throw std::runtime_error("Failed to lookup target: " + error);
  }

  // create target machine
  auto CPU = "generic";
  auto features = "";
  TargetOptions opt;
  auto RM = std::optional<Reloc::Model>();
  auto target_machine = target->createTargetMachine(
      target_triple, CPU, features, opt, RM);

  module->setDataLayout(target_machine->createDataLayout());

  std::error_code EC;
  raw_fd_ostream dest(output_path, EC, sys::fs::OF_None);
  if (EC)
  {
    throw std::runtime_error("Could not open file: " + EC.message());
  }

  legacy::PassManager pass;
  auto file_type = CodeGenFileType::ObjectFile;

  if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type))
  {
    throw std::runtime_error("TargetMachine can't emit a file of this type");
  }

  pass.run(*module);
  dest.flush();
}