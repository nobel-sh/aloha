#include "objgen.h"
#include "codegen.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Vectorize.h>
#include <system_error>

using namespace llvm;

void optimize(CodeGen &codegen) {
  legacy::PassManager pass_manager;
  pass_manager.add(llvm::createLoopSimplifyPass());
  pass_manager.add(llvm::createLCSSAPass());
  pass_manager.add(llvm::createEarlyCSEPass());
  pass_manager.add(llvm::createLoopUnrollPass());
  pass_manager.add(llvm::createPromoteMemoryToRegisterPass());
  pass_manager.add(llvm::createInstructionCombiningPass());
  pass_manager.add(llvm::createCFGSimplificationPass());
  pass_manager.add(llvm::createDeadCodeEliminationPass());
  pass_manager.run(*codegen.module);
  verifyModule(*codegen.module);
}

void objgen(CodeGen &codegen, const std::string filename) {

  // InitializeAllTargetInfos();
  // InitializeAllTargets();
  // InitializeAllTargetMCs();
  // InitializeAllAsmParsers();
  // InitializeAllAsmPrinters();
  // InitializeNativeTarget();
  // InitializeNativeTargetAsmParser();
  // InitializeNativeTargetAsmPrinter();

  // only handle x86 targets at this state
  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86AsmPrinter();
  LLVMInitializeX86AsmParser();

  auto target_triple = sys::getDefaultTargetTriple();
  codegen.module->setTargetTriple(target_triple);
  std::string Error;
  auto target = TargetRegistry::lookupTarget(target_triple, Error);

  if (!target) {
    errs() << Error;
    return;
  }
  auto cpu = "generic";
  auto features = "";

  TargetOptions opt;
  auto TargetMachine = target->createTargetMachine(target_triple, cpu, features,
                                                   opt, Reloc::PIC_);
  codegen.module->setDataLayout(TargetMachine->createDataLayout());
  codegen.module->setTargetTriple(target_triple);

  std::error_code EC;
  raw_fd_ostream dest(filename.c_str(), EC, sys::fs::OF_None);

  if (EC) {
    errs() << "Could not open file:" << EC.message();
    return;
  }

  legacy::PassManager pass;
  auto filetype = CodeGenFileType::ObjectFile;

  if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, filetype)) {
    errs() << "theTargetMachine can't emit a file of this type";
    return;
  }

  pass.run(*codegen.module.get());
  dest.flush();
  outs() << "Object code wrote to " << filename.c_str() << "\n";

  return;
}
