#ifndef SEMA_SYMBOL_TABLE_H_
#define SEMA_SYMBOL_TABLE_H_

#include "../ty/ty.h"
#include "../error/internal.h"
#include "../frontend/location.h"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace aloha
{

  using VarId = uint32_t;
  using FunctionId = uint32_t;

  struct VarSymbol
  {
    VarId id;
    std::string name;
    bool is_mutable;
    Location location;

    VarSymbol(VarId id, const std::string &name, bool is_mutable, Location loc)
        : id(id), name(name), is_mutable(is_mutable), location(loc) {}
  };

  struct FunctionSymbol
  {
    FunctionId id;
    std::string name;
    TyId return_type;
    std::vector<TyId> param_types;
    bool is_extern;
    Location location;

    FunctionSymbol(FunctionId id, const std::string &name, TyId ret_ty,
                   std::vector<TyId> params, bool is_extern, Location loc)
        : id(id), name(name), return_type(ret_ty), param_types(params),
          is_extern(is_extern), location(loc) {}
  };

  struct StructSymbol
  {
    StructId struct_id;
    TyId type_id;
    std::string name;
    Location location;

    StructSymbol(StructId sid, TyId tid, const std::string &name,
                 Location loc)
        : struct_id(sid), type_id(tid), name(name), location(loc) {}
  };

  struct EnumSymbol
  {
    EnumId enum_id;
    TyId type_id;
    std::string name;
    Location location;

    EnumSymbol(EnumId eid, TyId tid, const std::string &name, Location loc)
        : enum_id(eid), type_id(tid), name(name), location(loc) {}
  };

  struct OpaqueTypeSymbol
  {
    TyId type_id;
    std::string name;
    Location location;

    OpaqueTypeSymbol(TyId tid, const std::string &name, Location loc)
        : type_id(tid), name(name), location(loc) {}
  };

  struct EnumVariantSymbol
  {
    EnumId enum_id;
    TyId enum_type_id;
    std::string enum_name;
    std::string variant_name;
    uint32_t value;
    Location location;

    EnumVariantSymbol(EnumId eid, TyId tid, const std::string &enum_name,
                      const std::string &variant_name, uint32_t value,
                      Location loc)
        : enum_id(eid), enum_type_id(tid), enum_name(enum_name),
          variant_name(variant_name), value(value), location(loc) {}
  };

  class Scope
  {
  private:
    std::unordered_map<std::string, VarId> variables;
    Scope *parent;

  public:
    explicit Scope(Scope *parent = nullptr) : parent(parent) {}

    void add_variable(const std::string &name, VarId id)
    {
      variables[name] = id;
    }

    std::optional<VarId> lookup_variable(const std::string &name) const
    {
      auto it = variables.find(name);
      if (it != variables.end())
      {
        return it->second;
      }
      if (parent)
      {
        return parent->lookup_variable(name);
      }
      return std::nullopt;
    }

    bool has_variable_local(const std::string &name) const
    {
      return variables.find(name) != variables.end();
    }
  };

  class SymbolTable
  {
  public:
    std::unordered_map<std::string, FunctionSymbol> functions;

    std::unordered_map<std::string, StructSymbol> structs;
    std::unordered_map<std::string, EnumSymbol> enums;
    std::unordered_map<std::string, OpaqueTypeSymbol> opaque_types;
    std::unordered_map<std::string, EnumVariantSymbol> enum_variants;

    std::unordered_map<VarId, VarSymbol> variables;

    VarId next_var_id = 0;
    FunctionId next_func_id = 0;

    VarId allocate_var_id() { return next_var_id++; }

    FunctionId allocate_func_id() { return next_func_id++; }

    void register_variable(VarId id, const std::string &name, bool is_mutable,
                           Location loc)
    {
      variables.emplace(id, VarSymbol(id, name, is_mutable, loc));
    }

    void register_function(FunctionId id, const std::string &name,
                           TyId return_type,
                           std::vector<TyId> param_types, bool is_extern,
                           Location loc)
    {
      functions.emplace(name, FunctionSymbol(id, name, return_type, param_types,
                                             is_extern, loc));
    }

    void register_struct(const std::string &name, StructId struct_id,
                         TyId type_id, Location loc)
    {
      structs.emplace(name, StructSymbol(struct_id, type_id, name, loc));
    }

    void register_enum(const std::string &name, EnumId enum_id,
                       TyId type_id, Location loc)
    {
      enums.emplace(name, EnumSymbol(enum_id, type_id, name, loc));
    }

    void register_opaque_type(const std::string &name, TyId type_id, Location loc)
    {
      opaque_types.emplace(name, OpaqueTypeSymbol(type_id, name, loc));
    }

    void register_enum_variant(const std::string &enum_name,
                               const std::string &variant_name,
                               EnumId enum_id, TyId type_id, uint32_t value,
                               Location loc)
    {
      enum_variants.emplace(enum_name + "::" + variant_name,
                            EnumVariantSymbol(enum_id, type_id, enum_name,
                                              variant_name, value, loc));
    }

    std::optional<FunctionSymbol> lookup_function(const std::string &name) const
    {
      auto it = functions.find(name);
      if (it != functions.end())
      {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<StructSymbol> lookup_struct(const std::string &name) const
    {
      auto it = structs.find(name);
      if (it != structs.end())
      {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<EnumSymbol> lookup_enum(const std::string &name) const
    {
      auto it = enums.find(name);
      if (it != enums.end())
      {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<OpaqueTypeSymbol> lookup_opaque_type(const std::string &name) const
    {
      auto it = opaque_types.find(name);
      if (it != opaque_types.end())
      {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<EnumVariantSymbol> lookup_enum_variant(const std::string &enum_name,
                                                         const std::string &variant_name) const
    {
      auto it = enum_variants.find(enum_name + "::" + variant_name);
      if (it != enum_variants.end())
      {
        return it->second;
      }
      return std::nullopt;
    }

    std::vector<EnumVariantSymbol> get_enum_variants(const std::string &enum_name) const
    {
      std::vector<EnumVariantSymbol> variants;
      for (const auto &[key, variant] : enum_variants)
      {
        (void)key;
        if (variant.enum_name == enum_name)
        {
          variants.push_back(variant);
        }
      }
      return variants;
    }

    std::optional<VarSymbol> lookup_variable(VarId id) const
    {
      auto it = variables.find(id);
      if (it != variables.end())
      {
        return it->second;
      }
      return std::nullopt;
    }

    const std::unordered_map<std::string, FunctionSymbol> &get_all_functions() const
    {
      return functions;
    }

    const std::unordered_map<std::string, StructSymbol> &get_all_structs() const
    {
      return structs;
    }

    const std::unordered_map<std::string, EnumSymbol> &get_all_enums() const
    {
      return enums;
    }

    const std::unordered_map<std::string, OpaqueTypeSymbol> &get_all_opaque_types() const
    {
      return opaque_types;
    }
  };

} // namespace aloha
#endif // SEMA_SYMBOL_TABLE_H_
