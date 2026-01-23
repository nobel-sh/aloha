#ifndef AIR_TY_H_
#define AIR_TY_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace AIR
{
  using TyId = uint32_t;

  // special type ids for built-in types
  namespace TyIds
  {
    constexpr TyId ERROR = 0; // tyId 0 is a resolved type id used as a sentinel/error value
    constexpr TyId INTEGER = 1;
    constexpr TyId FLOAT = 2;
    constexpr TyId STRING = 3;
    constexpr TyId BOOL = 4;
    constexpr TyId VOID = 5;
    constexpr TyId USER_DEFINED_START = 1000;
  }

  using StructId = uint32_t;

  enum class TyKind
  {
    ERROR,
    INTEGER,
    FLOAT,
    STRING,
    BOOL,
    VOID,
    STRUCT,
    // add more
  };

  struct TyInfo
  {
    TyId id;
    TyKind kind;
    std::string name;

    std::optional<StructId> struct_id;

    std::vector<TyId> type_params;

    TyInfo(TyId id, TyKind kind, const std::string &name)
        : id(id), kind(kind), name(name), struct_id(std::nullopt) {}

    TyInfo(TyId id, TyKind kind, const std::string &name, StructId sid)
        : id(id), kind(kind), name(name), struct_id(sid) {}

    bool is_builtin() const
    {
      return kind == TyKind::INTEGER || kind == TyKind::FLOAT || kind == TyKind::STRING ||
             kind == TyKind::BOOL || kind == TyKind::VOID;
    }

    bool is_struct() const { return kind == TyKind::STRUCT; }

    bool is_error() const { return kind == TyKind::ERROR; }
  };

  class TyTable
  {
  private:
    std::unordered_map<TyId, std::unique_ptr<TyInfo>> types;
    std::unordered_map<std::string, TyId> name_to_ty;
    TyId next_ty_id;
    StructId next_struct_id;

  public:
    TyTable();

    TyId register_builtin(const std::string &name, TyKind kind, TyId id);

    TyId register_struct(const std::string &name, StructId struct_id);

    std::optional<TyId> lookup_by_name(const std::string &name) const;

    TyInfo *get_ty_info(TyId id);
    const TyInfo *get_ty_info(TyId id) const;

    bool has_ty(TyId id) const;
    bool has_ty_name(const std::string &name) const;

    StructId allocate_struct_id();

    std::string ty_name(TyId id) const;

    bool is_numeric(TyId id) const;
    bool is_bool(TyId id) const;
    bool is_string(TyId id) const;
    bool is_void(TyId id) const;
    bool is_struct(TyId id) const;

    bool are_compatible(TyId lhs, TyId rhs) const;
  };

} // namespace AIR

#endif // AIR_TY_H_
