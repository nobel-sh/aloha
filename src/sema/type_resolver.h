#ifndef SEMA_TYPE_RESOLVER_H_
#define SEMA_TYPE_RESOLVER_H_

#include "../ty/ty.h"
#include "../ast/ast.h"
#include "../frontend/location.h"
#include "symbol_binder.h"
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace aloha
{
  struct ResolvedField
  {
    std::string name;
    AIR::TyId type_id;
    Location location;

    ResolvedField(const std::string &n, AIR::TyId tid, Location loc)
        : name(n), type_id(tid), location(loc) {}
  };

  struct ResolvedStruct
  {
    AIR::StructId struct_id;
    AIR::TyId type_id;
    std::string name;
    std::vector<ResolvedField> fields;
    Location location;
    bool is_resolved;

    ResolvedStruct(AIR::StructId sid, AIR::TyId tid, const std::string &n, Location loc)
        : struct_id(sid), type_id(tid), name(n), location(loc), is_resolved(false) {}
  };

  struct ResolvedFunction
  {
    FunctionId id;
    std::string name;
    AIR::TyId return_type;
    std::vector<AIR::TyId> param_types;
    bool is_extern;
    Location location;

    ResolvedFunction(FunctionId fid, const std::string &n, AIR::TyId ret,
                     std::vector<AIR::TyId> params, bool ext, Location loc)
        : id(fid), name(n), return_type(ret), param_types(params), is_extern(ext), location(loc) {}
  };

  class TypeResolutionError
  {
  private:
    std::vector<std::string> errors;

  public:
    void add_error(Location loc, const std::string &message)
    {
      errors.push_back("[" + std::to_string(loc.line) + ":" +
                       std::to_string(loc.col) + "] " + message);
    }

    bool has_errors() const { return !errors.empty(); }

    void print_errors() const
    {
      for (const auto &error : errors)
      {
        std::cerr << "Type Resolution Error: " << error << std::endl;
      }
    }

    const std::vector<std::string> &get_errors() const { return errors; }
  };

  class TypeResolver
  {
  private:
    AIR::TyTable &ty_table;
    SymbolTable &symbol_table;
    TypeResolutionError errors;

    std::unordered_map<AIR::StructId, ResolvedStruct> resolved_structs;
    std::unordered_map<FunctionId, ResolvedFunction> resolved_functions;

    std::unordered_set<AIR::StructId> resolving_structs;

  public:
    TypeResolver(AIR::TyTable &table, SymbolTable &symbols)
        : ty_table(table), symbol_table(symbols) {}

    bool resolve(Program *program);

    // resolve a type name to tyId
    std::optional<AIR::TyId> resolve_type_name(const std::string &name, Location loc);

    const std::unordered_map<AIR::StructId, ResolvedStruct> &get_resolved_structs() const
    {
      return resolved_structs;
    }

    const std::unordered_map<FunctionId, ResolvedFunction> &get_resolved_functions() const
    {
      return resolved_functions;
    }

    const TypeResolutionError &get_errors() const { return errors; }

  private:
    void resolve_struct_fields(StructDecl *struct_decl);
    void resolve_function_signature(Function *func);

    // circular dependency detection for structs
    bool check_circular_dependency(AIR::StructId struct_id, const std::string &struct_name,
                                   std::unordered_set<AIR::StructId> &visiting, Location loc);

    std::string suggest_type_name(const std::string &name) const;
    bool is_primitive_type(const std::string &name) const;
  };

} // namespace aloha

#endif // SEMA_TYPE_RESOLVER_H_
