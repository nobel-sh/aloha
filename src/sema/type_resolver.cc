#include "type_resolver.h"
#include "../error/internal.h"
#include <algorithm>
#include <iostream>

namespace aloha
{

  bool TypeResolver::resolve(Program *program, const TySpecArena &type_arena)
  {
    if (!program)
    {
      ALOHA_ICE("Null program passed to TypeResolver::resolve");
    }

    for (const auto &node : program->m_nodes)
    {
      if (auto struct_decl = dynamic_cast<StructDecl *>(node.get()))
      {
        resolve_struct_fields(struct_decl, type_arena);
      }
    }

    for (const auto &node : program->m_nodes)
    {
      if (auto func = dynamic_cast<Function *>(node.get()))
      {
        resolve_function_signature(func, type_arena);
      }
    }

    if (!diagnostics.has_errors())
    {
      for (const auto &[struct_id, resolved_struct] : resolved_structs)
      {
        std::unordered_set<AIR::StructId> visiting;
        check_circular_dependency(struct_id, resolved_struct.name, visiting, resolved_struct.location);
      }
    }

    if (diagnostics.has_errors())
    {
      diagnostics.print_all();
      return false;
    }

    return true;
  }

  std::optional<AIR::TyId> TypeResolver::resolve_type_spec(TySpecId ty_spec_id, const TySpecArena &type_arena)
  {

    const TySpec &spec = type_arena[ty_spec_id];
    const Location loc = spec.loc;
    if (ty_spec_id >= type_arena.nodes.size())
    {
      ALOHA_ICE("TySpecId out of bound in TypeResolver::resolve_type_spec");
    }

    switch (spec.kind)
    {
    case TySpec::Kind::Builtin:
      switch (spec.builtin)
      {
      case TySpec::Builtin::Int:
        return AIR::TyIds::INTEGER;
      case TySpec::Builtin::Float:
        return AIR::TyIds::FLOAT;
      case TySpec::Builtin::String:
        return AIR::TyIds::STRING;
      case TySpec::Builtin::Bool:
        return AIR::TyIds::BOOL;
      case TySpec::Builtin::Void:
        return AIR::TyIds::VOID;
      }
      return std::nullopt;

    case TySpec::Kind::Named:
    {
      auto struct_opt = symbol_table.lookup_struct(spec.name);
      if (struct_opt.has_value())
      {
        return struct_opt->type_id;
      }

      std::string suggestion = suggest_type_name(spec.name);
      if (!suggestion.empty())
      {
        diagnostics.error(DiagnosticPhase::TypeResolution, loc, "Unknown type '" + spec.name + "'. Did you mean '" + suggestion + "'?");
      }
      else
      {
        diagnostics.error(DiagnosticPhase::TypeResolution, loc, "Unknown type '" + spec.name + "'");
      }
      return std::nullopt;
    }

    case TySpec::Kind::Array:
    {
      auto element_ty_opt = resolve_type_spec(spec.element, type_arena);
      if (!element_ty_opt.has_value())
      {
        return std::nullopt;
      }

      AIR::TyId array_ty = ty_table.register_array(element_ty_opt.value());
      return array_ty;
    }
    }

    return std::nullopt;
  }

  void TypeResolver::resolve_struct_fields(StructDecl *struct_decl, const TySpecArena &type_arena)
  {
    const std::string &struct_name = struct_decl->m_name;

    auto struct_opt = symbol_table.lookup_struct(struct_name);
    if (!struct_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::TypeResolution, struct_decl->loc(),
                        "Internal error: struct '" + struct_name + "' not in symbol table");
      return;
    }

    AIR::StructId struct_id = struct_opt->struct_id;
    AIR::TyId struct_type_id = struct_opt->type_id;

    ResolvedStruct resolved(struct_id, struct_type_id, struct_name, struct_decl->loc());

    for (const auto &field : struct_decl->m_fields)
    {
      auto ty_id_opt = resolve_type_spec(field.m_type, type_arena);
      if (!ty_id_opt.has_value())
      {
        continue;
      }

      resolved.fields.emplace_back(field.m_name, ty_id_opt.value(), struct_decl->loc());
    }

    if (!diagnostics.has_errors())
    {
      resolved.is_resolved = true;
    }

    resolved_structs.insert({struct_id, std::move(resolved)});
  }

  void TypeResolver::resolve_function_signature(Function *func, const TySpecArena &type_arena)
  {
    const std::string &func_name = func->m_name->m_name;

    auto func_opt = symbol_table.lookup_function(func_name);
    if (!func_opt.has_value())
    {
      diagnostics.error(DiagnosticPhase::TypeResolution, func->loc(),
                        "Internal error: function '" + func_name + "' not in symbol table");
      return;
    }

    FunctionId func_id = func_opt->id;

    auto return_ty_id_opt = resolve_type_spec(func->m_return_type, type_arena);
    if (!return_ty_id_opt.has_value())
    {
      return;
    }

    std::vector<AIR::TyId> param_types;
    for (const auto &param : func->m_parameters)
    {
      auto ty_id_opt = resolve_type_spec(param.m_type, type_arena);
      if (!ty_id_opt.has_value())
      {
        return;
      }
      param_types.push_back(ty_id_opt.value());
    }

    ResolvedFunction resolved(func_id, func_name, return_ty_id_opt.value(),
                              param_types, func->m_is_extern, func->loc());

    resolved_functions.insert({func_id, std::move(resolved)});
  }

  bool TypeResolver::check_circular_dependency(AIR::StructId struct_id,
                                               const std::string &struct_name,
                                               std::unordered_set<AIR::StructId> &visiting,
                                               Location loc)
  {
    // check if we're already visiting this struct
    if (visiting.find(struct_id) != visiting.end())
    {
      diagnostics.error(DiagnosticPhase::TypeResolution, loc, "Circular dependency detected in struct '" + struct_name + "'");
      return true;
    }

    // look up resolved struct
    auto it = resolved_structs.find(struct_id);
    if (it == resolved_structs.end())
    {
      return false; // struct not resolved yet, skip
    }

    const ResolvedStruct &resolved = it->second;

    visiting.insert(struct_id);

    for (const auto &field : resolved.fields)
    {
      auto ty_info = ty_table.get_ty_info(field.type_id);
      if (ty_info && ty_info->kind == AIR::TyKind::STRUCT)
      {
        // recursively check for circular dependency
        if (ty_info->struct_id.has_value())
        {
          AIR::StructId field_struct_id = ty_info->struct_id.value();
          if (check_circular_dependency(field_struct_id, field.name, visiting, field.location))
          {
            return true;
          }
        }
      }
    }

    // remove from visiting set (backtrack)
    visiting.erase(struct_id);
    return false;
  }

  // suggest the closest type name for an unknown type using Levenstein distance
  std::string TypeResolver::suggest_type_name(const std::string &name) const
  {
    std::vector<std::string> candidates = {"int", "float", "string", "bool", "void"};

    for (const auto &[struct_name, struct_symbol] : symbol_table.structs)
    {
      candidates.push_back(struct_name);
    }

    std::string best_match;
    int best_score = 999;

    for (const auto &candidate : candidates)
    {
      // length difference + first character match
      int score = std::abs(static_cast<int>(name.length()) - static_cast<int>(candidate.length()));
      if (!name.empty() && !candidate.empty() && name[0] == candidate[0])
      {
        score -= 2; // bonus for matching first character
      }

      if (score < best_score && score < 3)
      {
        best_score = score;
        best_match = candidate;
      }
    }

    return best_match;
  }

  bool TypeResolver::is_primitive_type(const std::string &name) const
  {
    return name == "int" || name == "float" || name == "string" || name == "bool" || name == "void";
  }

} // namespace aloha