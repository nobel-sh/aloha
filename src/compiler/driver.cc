#include "driver.h"
#include "../air/printer.h"
#include "../codegen/objgen.h"
#include "../utils/paths.h"
#include <cstdlib>
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

namespace aloha
{

  CompilerDriver::CompilerDriver(const CompilerOptions &options)
      : options(options), has_compilation_errors(false)
  {
    ty_table = std::make_unique<TyTable>();
  }

  CompilerDriver::~CompilerDriver() = default;

  bool CompilerDriver::has_errors() const
  {
    return diagnostics.has_errors();
  }

  void CompilerDriver::print_errors() const
  {
    diagnostics.print_all();
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

  std::string CompilerDriver::get_stdlib_archive_path() const
  {
    return aloha::utils::get_stdlib_archive();
  }

  Location CompilerDriver::input_location() const
  {
    return Location(1, 1, options.input_file);
  }

  bool CompilerDriver::fail_with_diagnostic(DiagnosticPhase phase,
                                            const std::string &message,
                                            bool mark_compilation_error)
  {
    diagnostics.error(phase, input_location(), message);
    diagnostics.print_all();
    if (mark_compilation_error)
    {
      has_compilation_errors = true;
    }
    return false;
  }

  bool CompilerDriver::fail_after_diagnostics(bool mark_compilation_error)
  {
    diagnostics.print_all();
    if (mark_compilation_error)
    {
      has_compilation_errors = true;
    }
    return false;
  }

  bool CompilerDriver::fail_stage_or_diagnostics(DiagnosticPhase phase,
                                                 const std::string &message,
                                                 bool mark_compilation_error)
  {
    if (diagnostics.has_errors())
    {
      return fail_after_diagnostics(mark_compilation_error);
    }

    return fail_with_diagnostic(phase, message, mark_compilation_error);
  }

  void CompilerDriver::dump_ast() const
  {
    if (!options.dump_ast || !ast || !parser)
      return;

    std::cout << "\n========================================\n";
    std::cout << "UNTYPED AST\n";
    std::cout << "========================================\n";
    parser->dump(ast.get(), type_arena);
    std::cout << "========================================\n\n";
  }

  void CompilerDriver::dump_air() const
  {
    if (!options.dump_air || !air_module)
      return;

    std::cout << "\n========================================\n";
    std::cout << "AIR MODULE\n";
    std::cout << "========================================\n";
    air::Printer printer(std::cout, ty_table.get());
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
        return fail_with_diagnostic(DiagnosticPhase::Driver,
                                    "Could not open file: " + options.input_file,
                                    false);
      }
      std::string source((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
      file.close();

      if (source.empty())
      {
        return fail_with_diagnostic(DiagnosticPhase::Driver,
                                    "File is empty: " + options.input_file,
                                    false);
      }

      lexer = std::make_unique<Lexer>(source, options.input_file);
      parser = std::make_unique<Parser>(*lexer, type_arena, diagnostics);

      ast = parser->parse();
      if (!ast || diagnostics.has_errors())
      {
        return fail_stage_or_diagnostics(DiagnosticPhase::Parser, "Parsing failed", false);
      }

      log("Parsed successfully");
      dump_ast();
      return true;
    }
    catch (const std::exception &e)
    {
      return fail_with_diagnostic(DiagnosticPhase::Parser,
                                  "Parsing exception: " + std::string(e.what()),
                                  false);
    }
  }

  bool CompilerDriver::stage_symbol_binding()
  {
    log_stage("Definition Collection");

    try
    {
      symbol_binder = std::make_unique<aloha::SymbolBinder>(*ty_table, diagnostics);

      if (!symbol_binder->bind(ast.get(), type_arena) || diagnostics.has_errors())
      {
        return fail_stage_or_diagnostics(DiagnosticPhase::SymbolBinding,
                                         "Symbol binding failed");
      }

      log("Definition collection completed successfully");

      return true;
    }
    catch (const std::exception &e)
    {
      return fail_with_diagnostic(DiagnosticPhase::SymbolBinding,
                                  "Definition collection exception: " + std::string(e.what()));
    }
  }

  bool CompilerDriver::stage_import_resolution()
  {
    log_stage("Import Resolution");

    try
    {
      import_resolver = std::make_unique<aloha::ImportResolver>(
          *ty_table, symbol_binder->get_symbol_table(), type_arena, diagnostics, options.input_file);

      if (!import_resolver->resolve_imports(ast.get()) || diagnostics.has_errors())
      {
        return fail_stage_or_diagnostics(DiagnosticPhase::ImportResolution,
                                         "Import resolution failed");
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
      return fail_with_diagnostic(DiagnosticPhase::ImportResolution,
                                  "Import resolution exception: " + std::string(e.what()));
    }
  }

  bool CompilerDriver::stage_type_resolution()
  {
    log_stage("Type Resolution");

    try
    {
      type_resolver = std::make_unique<aloha::TypeResolver>(
          *ty_table, symbol_binder->get_symbol_table(), diagnostics);

      if (!type_resolver->resolve(ast.get(), type_arena) || diagnostics.has_errors())
      {
        return fail_stage_or_diagnostics(DiagnosticPhase::TypeResolution,
                                         "Type resolution failed");
      }

      if (import_resolver)
      {
        const auto &imported_asts = import_resolver->get_imported_asts();
        for (const auto &imported_ast : imported_asts)
        {
          if (!type_resolver->resolve(imported_ast.get(), type_arena) || diagnostics.has_errors())
          {
            return fail_stage_or_diagnostics(DiagnosticPhase::TypeResolution,
                                             "Type resolution failed in imported file");
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
      return fail_with_diagnostic(DiagnosticPhase::TypeResolution,
                                  "Type resolution exception: " + std::string(e.what()));
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
          type_resolver->get_resolved_functions(),
          type_arena,
          *type_resolver,
          diagnostics);

      air_module = air_builder->build(ast.get());
      if (!air_module || diagnostics.has_errors())
      {
        return fail_stage_or_diagnostics(DiagnosticPhase::AIRBuilding,
                                         "AIR building failed");
      }

      if (import_resolver)
      {
        const auto &imported_asts = import_resolver->get_imported_asts();
        for (const auto &imported_ast : imported_asts)
        {
          auto imported_module = air_builder->build(imported_ast.get());
          if (!imported_module || diagnostics.has_errors())
          {
            return fail_stage_or_diagnostics(DiagnosticPhase::AIRBuilding,
                                             "AIR building failed in imported file");
          }

          for (auto &func : imported_module->m_functions)
          {
            air_module->m_functions.push_back(std::move(func));
          }
          for (auto &struct_decl : imported_module->m_structs)
          {
            air_module->m_structs.push_back(std::move(struct_decl));
          }
        }
      }

      log("AIR building completed successfully");
      dump_air();
      return true;
    }
    catch (const std::exception &e)
    {
      return fail_with_diagnostic(DiagnosticPhase::AIRBuilding,
                                  "AIR building exception: " + std::string(e.what()));
    }
  }

  bool CompilerDriver::stage_codegen()
  {
    log_stage("Code Generation");

    try
    {
      codegen = std::make_unique<CodeGenerator>(*ty_table, diagnostics);

      llvm_module = codegen->generate(air_module.get());
      if (!llvm_module || diagnostics.has_errors())
      {
        return fail_stage_or_diagnostics(DiagnosticPhase::Codegen,
                                         "Code generation failed");
      }

      log("Code generation completed successfully");
      dump_llvm_ir();
      return true;
    }
    catch (const std::exception &e)
    {
      return fail_with_diagnostic(DiagnosticPhase::Codegen,
                                  "Code generation exception: " + std::string(e.what()));
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
      return fail_with_diagnostic(DiagnosticPhase::Optimization,
                                  "Optimization exception: " + std::string(e.what()),
                                  false);
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
        return fail_with_diagnostic(DiagnosticPhase::Emission,
                                    "Could not open file " + ir_file + ": " + ec.message(),
                                    false);
      }

      llvm_module->print(out, nullptr);
      out.close();

      std::cout << "LLVM IR written to: " << ir_file << std::endl;
      return true;
    }
    catch (const std::exception &e)
    {
      return fail_with_diagnostic(DiagnosticPhase::Emission,
                                  "LLVM IR emission exception: " + std::string(e.what()),
                                  false);
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
      return fail_with_diagnostic(DiagnosticPhase::Emission,
                                  "Object file emission exception: " + std::string(e.what()),
                                  false);
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
      std::string stdlib_path = get_stdlib_archive_path();

      std::string link_driver;

      if (const char *env_cc = std::getenv("ALOHA_CC"))
      {
        link_driver = env_cc;
      }
      else
      {
        const char *link_driver_candidates[] = {"cc", "clang", "gcc"};

        for (const char *candidate : link_driver_candidates)
        {
          if (auto driver_path = llvm::sys::findProgramByName(candidate))
          {
            link_driver = driver_path.get();
            break;
          }
        }
      }

      if (link_driver.empty())
      {
        return fail_with_diagnostic(DiagnosticPhase::Linking,
                                    "No C compiler found for linking (tried: ALOHA_CC, cc, clang, gcc)",
                                    false);
      }

      log("Using C compiler for linking: " + link_driver);

      std::vector<llvm::StringRef> args = {
          link_driver,
          obj_file,
          stdlib_path,
          "-no-pie",
          "-o",
          exe_file};

      std::string error_msg;
      int result = llvm::sys::ExecuteAndWait(link_driver, args, std::nullopt, {}, 0, 0,
                                             &error_msg);

      if (result == 0)
      {
        std::cout << "Linking successful: " << exe_file << std::endl;
        return true;
      }
      else
      {
        std::string message = "Linking failed with code " + std::to_string(result);
        if (!error_msg.empty())
        {
          message += ": " + error_msg;
        }
        return fail_with_diagnostic(DiagnosticPhase::Linking, message, false);
      }
    }
    catch (const std::exception &e)
    {
      return fail_with_diagnostic(DiagnosticPhase::Linking,
                                  "Linking exception: " + std::string(e.what()),
                                  false);
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

} // namespace aloha
