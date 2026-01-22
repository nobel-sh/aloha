#include "driver.h"
#include "../air/printer.h"
#include "../codegen/objgen.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/raw_ostream.h>

#if defined(__linux__)
#include <linux/limits.h>
#include <unistd.h>
#else
#error "Unsupported platform. Currently only Linux is supported."
#endif

namespace AlohaPipeline
{

  CompilerDriver::CompilerDriver(const CompilerOptions &options)
      : options(options), has_compilation_errors(false)
  {
    ty_table = std::make_unique<AIR::TyTable>();
  }

  CompilerDriver::~CompilerDriver() = default;

  bool CompilerDriver::has_errors() const
  {
    return has_compilation_errors;
  }

  void CompilerDriver::print_errors() const
  {
    if (symbol_binder && symbol_binder->get_errors().has_errors())
    {
      symbol_binder->get_errors().print();
    }
    if (import_resolver && import_resolver->get_errors().has_errors())
    {
      import_resolver->get_errors().print();
    }
    if (type_resolver && type_resolver->get_errors().has_errors())
    {
      type_resolver->get_errors().print();
    }
    if (air_builder && air_builder->get_errors().has_errors())
    {
      air_builder->get_errors().print();
    }
    if (codegen && codegen->has_errors())
    {
      codegen->get_error_reporter().print();
    }
  }

  void CompilerDriver::log(const std::string &message) const
  {
    if (options.verbose)
    {
      std::cout << "[INFO] " << message << std::endl;
    }
  }

  void CompilerDriver::log_stage(const std::string &stage_name) const
  {
    std::cout << "Stage: " << stage_name << "..." << std::endl;
  }

  std::string CompilerDriver::get_base_name() const
  {
    std::filesystem::path p(options.input_file);
    return p.stem().string();
  }

  std::string CompilerDriver::get_output_name(const std::string &extension) const
  {
    if (!options.output_file.empty())
    {
      return options.output_file + extension;
    }
    return get_base_name() + extension;
  }

  std::string CompilerDriver::get_stdlib_path() const
  {
    if (const char *aloha_home = std::getenv("ALOHA_HOME"))
    {
      return std::string(aloha_home) + "/lib/libaloha_stdlib.a";
    }

    // try to find standard library relative to the compiler executable
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1)
    {
      exe_path[len] = '\0';
      std::filesystem::path exe_dir =
          std::filesystem::path(exe_path).parent_path();
      std::filesystem::path stdlib_path = exe_dir / "libaloha_stdlib.a";
      if (std::filesystem::exists(stdlib_path))
      {
        return stdlib_path.string();
      }
    }

    // fallback: look in build directory
    return "../build/libaloha_stdlib.a";
  }

  void CompilerDriver::dump_ast() const
  {
    if (!options.dump_ast || !ast || !parser)
      return;

    std::cout << "\n========================================\n";
    std::cout << "UNTYPED AST\n";
    std::cout << "========================================\n";
    parser->dump(ast.get());
    std::cout << "========================================\n\n";
  }

  void CompilerDriver::dump_air() const
  {
    if (!options.dump_air || !air_module)
      return;

    std::cout << "\n========================================\n";
    std::cout << "AIR MODULE\n";
    std::cout << "========================================\n";
    AIR::Printer printer(std::cout, ty_table.get());
    printer.print(air_module.get());
    std::cout << "========================================\n\n";
  }

  void CompilerDriver::dump_llvm_ir() const
  {
    if (!options.dump_ir || !llvm_module)
      return;

    std::cout << "\n========================================\n";
    std::cout << "LLVM IR\n";
    std::cout << "========================================\n";
    llvm_module->print(llvm::outs(), nullptr);
    std::cout << "========================================\n\n";
  }

  bool CompilerDriver::stage_parse()
  {
    log_stage("Parsing");

    try
    {
      std::ifstream file(options.input_file);
      if (!file.is_open())
      {
        std::cerr << "Error: Could not open file: " << options.input_file << std::endl;
        return false;
      }
      std::string source((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
      file.close();

      if (source.empty())
      {
        std::cerr << "Error: File is empty: " << options.input_file << std::endl;
        return false;
      }

      lexer = std::make_unique<Lexer>(source, options.input_file);
      parser = std::make_unique<Parser>(*lexer);

      ast = parser->parse();
      if (!ast)
      {
        std::cerr << "Error: Parsing failed" << std::endl;
        return false;
      }

      log("Parsed successfully");
      dump_ast();
      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: Parsing exception: " << e.what() << std::endl;
      return false;
    }
  }

  bool CompilerDriver::stage_symbol_binding()
  {
    log_stage("Definition Collection");

    try
    {
      symbol_binder = std::make_unique<aloha::SymbolBinder>(*ty_table);

      if (!symbol_binder->bind(ast.get()))
      {
        std::cerr << "Error: Symbol binding failed" << std::endl;
        symbol_binder->get_errors().print();
        has_compilation_errors = true;
        return false;
      }

      log("Definition collection completed successfully");

      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: Definition collection exception: " << e.what() << std::endl;
      has_compilation_errors = true;
      return false;
    }
  }

  bool CompilerDriver::stage_import_resolution()
  {
    log_stage("Import Resolution");

    try
    {
      import_resolver = std::make_unique<aloha::ImportResolver>(
          *ty_table, symbol_binder->get_symbol_table(), options.input_file);

      if (!import_resolver->resolve_imports(ast.get()))
      {
        std::cerr << "Error: Import resolution failed" << std::endl;
        import_resolver->get_errors().print();
        has_compilation_errors = true;
        return false;
      }

      log("Import resolution completed successfully");
      auto import_paths = import_resolver->get_import_paths();
      if (!import_paths.empty())
      {
        log("  Imports resolved: " + std::to_string(import_paths.size()));
        for (const auto &path : import_paths)
        {
          log("    - " + path);
        }
      }

      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: Import resolution exception: " << e.what() << std::endl;
      has_compilation_errors = true;
      return false;
    }
  }

  bool CompilerDriver::stage_type_resolution()
  {
    log_stage("Type Resolution");

    try
    {
      type_resolver = std::make_unique<aloha::TypeResolver>(
          *ty_table, symbol_binder->get_symbol_table());

      if (!type_resolver->resolve(ast.get()))
      {
        std::cerr << "Error: Type resolution failed" << std::endl;
        type_resolver->get_errors().print();
        has_compilation_errors = true;
        return false;
      }

      if (import_resolver)
      {
        const auto &imported_asts = import_resolver->get_imported_asts();
        for (const auto &imported_ast : imported_asts)
        {
          if (!type_resolver->resolve(imported_ast.get()))
          {
            std::cerr << "Error: Type resolution failed in imported file" << std::endl;
            type_resolver->get_errors().print();
            has_compilation_errors = true;
            return false;
          }
        }
      }

      log("Type resolution completed successfully");
      log("  Structs resolved: " +
          std::to_string(type_resolver->get_resolved_structs().size()));
      log("  Functions resolved: " +
          std::to_string(type_resolver->get_resolved_functions().size()));

      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: Type resolution exception: " << e.what() << std::endl;
      has_compilation_errors = true;
      return false;
    }
  }

  bool CompilerDriver::stage_air_building()
  {
    log_stage("AIR Building");

    try
    {
      air_builder = std::make_unique<aloha::AIRBuilder>(
          *ty_table,
          symbol_binder->get_symbol_table(),
          type_resolver->get_resolved_structs(),
          type_resolver->get_resolved_functions());

      air_module = air_builder->build(ast.get());
      if (!air_module)
      {
        std::cerr << "Error: AIR building failed" << std::endl;
        air_builder->get_errors().print();
        has_compilation_errors = true;
        return false;
      }

      if (import_resolver)
      {
        const auto &imported_asts = import_resolver->get_imported_asts();
        for (const auto &imported_ast : imported_asts)
        {
          auto imported_module = air_builder->build(imported_ast.get());
          if (!imported_module)
          {
            std::cerr << "Error: AIR building failed in imported file" << std::endl;
            air_builder->get_errors().print();
            has_compilation_errors = true;
            return false;
          }

          for (auto &func : imported_module->functions)
          {
            air_module->functions.push_back(std::move(func));
          }
          for (auto &struct_decl : imported_module->structs)
          {
            air_module->structs.push_back(std::move(struct_decl));
          }
        }
      }

      log("AIR building completed successfully");
      dump_air();
      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: AIR building exception: " << e.what() << std::endl;
      has_compilation_errors = true;
      return false;
    }
  }

  bool CompilerDriver::stage_codegen()
  {
    log_stage("Code Generation");

    try
    {
      codegen = std::make_unique<Codegen::CodeGenerator>(*ty_table);

      llvm_module = codegen->generate(air_module.get());
      if (!llvm_module)
      {
        std::cerr << "Error: Code generation failed" << std::endl;
        codegen->get_error_reporter().print();
        has_compilation_errors = true;
        return false;
      }

      log("Code generation completed successfully");
      dump_llvm_ir();
      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: Code generation exception: " << e.what() << std::endl;
      has_compilation_errors = true;
      return false;
    }
  }

  bool CompilerDriver::stage_optimize()
  {
    if (!options.enable_optimization)
    {
      log("Optimization: disabled");
      return true;
    }

    log_stage("Optimization");

    try
    {
      optimize_module(llvm_module.get());
      log("Optimization passes completed");
      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: Optimization exception: " << e.what() << std::endl;
      return false;
    }
  }

  bool CompilerDriver::stage_emit_llvm_ir()
  {
    if (!options.emit_llvm)
    {
      return true;
    }

    log_stage("Emitting LLVM IR");

    try
    {
      std::string ir_file = get_output_name(".ll");
      std::error_code ec;
      llvm::raw_fd_ostream out(ir_file, ec, llvm::sys::fs::OF_Text);

      if (ec)
      {
        std::cerr << "Error: Could not open file " << ir_file << ": "
                  << ec.message() << std::endl;
        return false;
      }

      llvm_module->print(out, nullptr);
      out.close();

      std::cout << "LLVM IR written to: " << ir_file << std::endl;
      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: LLVM IR emission exception: " << e.what() << std::endl;
      return false;
    }
  }

  bool CompilerDriver::stage_emit_object()
  {
    if (!options.emit_object && !options.emit_executable)
    {
      return true;
    }

    log_stage("Emitting Object File");

    try
    {
      std::string obj_file = get_output_name(".o");

      emit_object_file(llvm_module.get(), obj_file);

      std::cout << "Object file written to: " << obj_file << std::endl;
      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: Object file emission exception: " << e.what() << std::endl;
      return false;
    }
  }

  bool CompilerDriver::stage_link_executable()
  {
    if (!options.emit_executable)
    {
      return true;
    }

    log_stage("Linking Executable");

    try
    {
      std::string obj_file = get_output_name(".o");
      std::string exe_file = get_output_name(".out");
      std::string stdlib_path = get_stdlib_path();

      const char *linker_candidates[] = {"ld.lld", "ld", "lld"};
      std::string linker;

      for (const char *candidate : linker_candidates)
      {
        if (auto linker_path = llvm::sys::findProgramByName(candidate))
        {
          linker = linker_path.get();
          break;
        }
      }

      if (linker.empty())
      {
        std::cerr << "Error: No linker found (tried: ld.lld, ld, lld)" << std::endl;
        return false;
      }

      log("Using linker: " + linker);

      std::vector<llvm::StringRef> args = {
          linker,
          "-o",
          exe_file,
          "/usr/lib/x86_64-linux-gnu/crt1.o",
          "/usr/lib/x86_64-linux-gnu/crti.o",
          obj_file,
          stdlib_path,
          "/usr/lib/x86_64-linux-gnu/crtn.o",
          "-L/usr/lib/x86_64-linux-gnu",
          "-L/usr/lib",
          "-L/lib/x86_64-linux-gnu",
          "-L/lib",
          "-lc",
          "-dynamic-linker",
          "/lib64/ld-linux-x86-64.so.2"};

      std::string error_msg;
      int result = llvm::sys::ExecuteAndWait(linker, args, std::nullopt, {}, 0, 0,
                                             &error_msg);

      if (result == 0)
      {
        std::cout << "Linking successful: " << exe_file << std::endl;
        return true;
      }
      else
      {
        std::cerr << "Error: Linking failed with code " << result << std::endl;
        if (!error_msg.empty())
        {
          std::cerr << "Error: " << error_msg << std::endl;
        }
        return false;
      }
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error: Linking exception: " << e.what() << std::endl;
      return false;
    }
  }

  int CompilerDriver::compile()
  {
    std::cout << "========================================\n";
    std::cout << "            Aloha Compiler \n";
    std::cout << "========================================\n";
    std::cout << "Input: " << options.input_file << std::endl;
    std::cout << "\n";

    if (!stage_parse())
      return 1;

    if (!stage_symbol_binding())
      return 1;

    if (!stage_import_resolution())
      return 1;

    if (!stage_type_resolution())
      return 1;

    if (!stage_air_building())
      return 1;

    if (!stage_codegen())
      return 1;

    if (!stage_optimize())
      return 1;

    if (!stage_emit_llvm_ir())
      return 1;

    if (!stage_emit_object())
      return 1;

    if (!stage_link_executable())
      return 1;

    std::cout << "\n========================================\n";
    std::cout << "Compilation successful!\n";
    std::cout << "========================================\n";

    return 0;
  }

} // namespace CompilerPipeline