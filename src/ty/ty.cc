#include "ty.h"
#include <stdexcept>

namespace aloha
{

  TyTable::TyTable()
      : next_ty_id(TyIds::USER_DEFINED_START), next_struct_id(0)
  {
    register_builtin("int", TyKind::INTEGER, TyIds::INTEGER);
    register_builtin("float", TyKind::FLOAT, TyIds::FLOAT);
    register_builtin("string", TyKind::STRING, TyIds::STRING);
    register_builtin("bool", TyKind::BOOL, TyIds::BOOL);
    register_builtin("void", TyKind::VOID, TyIds::VOID);
    register_builtin("error", TyKind::ERROR, TyIds::ERROR);
  }

  TyId TyTable::register_builtin(const std::string &name, TyKind kind, TyId id)
  {
    auto ty_info = std::make_unique<TyInfo>(id, kind, name);
    types[id] = std::move(ty_info);
    name_to_ty[name] = id;
    return id;
  }

  TyId TyTable::register_struct(const std::string &name, StructId struct_id)
  {
    if (auto existing = lookup_by_name(name))
    {
      return *existing;
    }

    TyId ty_id = next_ty_id++;
    auto ty_info = std::make_unique<TyInfo>(ty_id, TyKind::STRUCT, name, struct_id);
    types[ty_id] = std::move(ty_info);
    name_to_ty[name] = ty_id;
    return ty_id;
  }

  TyId TyTable::register_array(TyId element_type)
  {
    auto it = array_type_cache.find(element_type);
    if (it != array_type_cache.end())
    {
      return it->second;
    }

    TyId array_ty_id = next_ty_id++;
    std::string array_name = ty_name(element_type) + "[]";
    auto ty_info = std::make_unique<TyInfo>(array_ty_id, TyKind::ARRAY, array_name);
    ty_info->type_params.push_back(element_type);

    types[array_ty_id] = std::move(ty_info);
    array_type_cache[element_type] = array_ty_id;

    return array_ty_id;
  }

  std::optional<TyId> TyTable::lookup_by_name(const std::string &name) const
  {
    auto it = name_to_ty.find(name);
    if (it != name_to_ty.end())
    {
      return it->second;
    }
    return std::nullopt;
  }

  TyInfo *TyTable::get_ty_info(TyId id)
  {
    auto it = types.find(id);
    if (it != types.end())
    {
      return it->second.get();
    }
    return nullptr;
  }

  const TyInfo *TyTable::get_ty_info(TyId id) const
  {
    auto it = types.find(id);
    if (it != types.end())
    {
      return it->second.get();
    }
    return nullptr;
  }

  bool TyTable::has_ty(TyId id) const
  {
    return types.find(id) != types.end();
  }

  bool TyTable::has_ty_name(const std::string &name) const
  {
    return name_to_ty.find(name) != name_to_ty.end();
  }

  StructId TyTable::allocate_struct_id()
  {
    return next_struct_id++;
  }

  std::string TyTable::ty_name(TyId id) const
  {
    auto ty_info = get_ty_info(id);
    if (ty_info)
    {
      return ty_info->name;
    }
    return "<invalid type id>";
  }

  bool TyTable::is_numeric(TyId id) const
  {
    return id == TyIds::INTEGER || id == TyIds::FLOAT;
  }

  bool TyTable::is_bool(TyId id) const
  {
    return id == TyIds::BOOL;
  }

  bool TyTable::is_string(TyId id) const
  {
    return id == TyIds::STRING;
  }

  bool TyTable::is_void(TyId id) const
  {
    return id == TyIds::VOID;
  }

  bool TyTable::is_struct(TyId id) const
  {
    auto ty_info = get_ty_info(id);
    return ty_info && ty_info->kind == TyKind::STRUCT;
  }

  bool TyTable::is_array(TyId id) const
  {
    auto ty_info = get_ty_info(id);
    return ty_info && ty_info->kind == TyKind::ARRAY;
  }

  std::optional<TyId> TyTable::get_array_element_type(TyId array_ty) const
  {
    auto ty_info = get_ty_info(array_ty);
    if (ty_info && ty_info->kind == TyKind::ARRAY && !ty_info->type_params.empty())
    {
      return ty_info->type_params[0];
    }
    return std::nullopt;
  }

  bool TyTable::are_compatible(TyId lhs, TyId rhs) const
  {
    // for now, types must be exactly the same
    // todo: handle implicit conversions, subtyping, ...

    // Arrays are structurally typed and canonicalized,
    // so pointer equality suffices
    return lhs == rhs;
  }

} // namespace aloha