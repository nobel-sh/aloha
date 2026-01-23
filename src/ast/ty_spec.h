#ifndef ALOHA_TY_SPEC_H_
#define ALOHA_TY_SPEC_H_

#include "../frontend/location.h"
#include <optional>
#include <string>
#include <vector>

namespace aloha
{

  using TySpecId = uint64_t;

  struct TySpec
  {
    enum class Kind
    {
      Builtin,
      Named,
      Array
    };

    Kind kind;
    Location loc;

    enum class Builtin
    {
      Int,
      Float,
      Bool,
      String,
      Void
    };

    Builtin builtin;
    std::string name;
    TySpecId element = 0;
    std::optional<uint64_t> size;
  };

  struct TySpecArena
  {
    std::vector<TySpec> nodes;

    TySpecId builtin(Location loc, TySpec::Builtin b)
    {
      TySpec t{};
      t.kind = TySpec::Kind::Builtin;
      t.loc = loc;
      t.builtin = b;
      return add(std::move(t));
    }

    TySpecId named(Location loc, std::string n)
    {
      TySpec t{};
      t.kind = TySpec::Kind::Named;
      t.loc = loc;
      t.name = std::move(n);
      return add(std::move(t));
    }

    TySpecId array(Location loc, TySpecId elem, std::optional<uint64_t> sz)
    {
      TySpec t{};
      t.kind = TySpec::Kind::Array;
      t.loc = loc;
      t.element = elem;
      t.size = sz;
      return add(std::move(t));
    }

    std::string to_string(TySpecId id) const
    {
      if (id >= nodes.size())
        return "invalid";

      const auto &spec = nodes[id];
      switch (spec.kind)
      {
      case TySpec::Kind::Builtin:
        switch (spec.builtin)
        {
        case TySpec::Builtin::Int:
          return "int";
        case TySpec::Builtin::Float:
          return "float";
        case TySpec::Builtin::Bool:
          return "bool";
        case TySpec::Builtin::String:
          return "string";
        case TySpec::Builtin::Void:
          return "void";
        }
        return "unknown_builtin";

      case TySpec::Kind::Named:
        return spec.name;

      case TySpec::Kind::Array:
        if (spec.size.has_value())
        {
          return to_string(spec.element) + "[" + std::to_string(*spec.size) + "]";
        }
        return to_string(spec.element) + "[]";
      }
      return "unknown";
    }

    TySpec &operator[](TySpecId id) { return nodes[id]; }
    const TySpec &operator[](TySpecId id) const { return nodes[id]; }

  private:
    TySpecId add(TySpec t)
    {
      nodes.push_back(std::move(t));
      return nodes.size() - 1;
    }
  };

} // namespace aloha

#endif // ALOHA_TY_SPEC_H_