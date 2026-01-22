#ifndef OBJGEN_H_
#define OBJGEN_H_

#include <llvm/IR/Module.h>
#include <string>

namespace llvm
{
  class Module;
}

// Emit object file from LLVM module
void emit_object_file(llvm::Module *module, const std::string &output_path);

// Apply optimization passes to module
void optimize_module(llvm::Module *module);

#endif // OBJGEN_H_