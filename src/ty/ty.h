#ifndef AIR_TY_H_
#define AIR_TY_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace aloha
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
    constexpr TyId NULL_TY = 6;
    constexpr TyId USER_DEFINED_START = 1000;
  }

  using StructId = uint32_t;
  using EnumId = uint32_t;
  using FunctionId = uint32_t;
  using VarId = uint32_t;

  enum class TyKind
  {
    ERROR,
    INTEGER,
    FLOAT,
    STRING,
    BOOL,
    VOID,
    NULL_TY,
    STRUCT,
    ENUM,
    OPAQUE,
    ARRAY,
    REF,
    // add more
  };

  struct TyInfo
  {
    TyId m_id;
    TyKind m_kind;
    std::string m_name;

    std::optional<StructId> m_struct_id;
    std::optional<EnumId> m_enum_id;

    std::vector<TyId> m_type_params;

    TyInfo(TyId id, TyKind kind, const std::string &name)
        : m_id(id), m_kind(kind), m_name(name), m_struct_id(std::nullopt), m_enum_id(std::nullopt) {}

    TyInfo(TyId id, TyKind kind, const std::string &name, StructId sid)
        : m_id(id), m_kind(kind), m_name(name), m_struct_id(sid), m_enum_id(std::nullopt) {}

    bool is_builtin() const
    {
      return m_kind == TyKind::INTEGER || m_kind == TyKind::FLOAT || m_kind == TyKind::STRING ||
             m_kind == TyKind::BOOL || m_kind == TyKind::VOID;
    }

    bool is_struct() const { return m_kind == TyKind::STRUCT; }

    bool is_enum() const { return m_kind == TyKind::ENUM; }

    bool is_opaque() const { return m_kind == TyKind::OPAQUE; }

    bool is_array() const { return m_kind == TyKind::ARRAY; }

    bool is_ref() const { return m_kind == TyKind::REF; }

    bool is_error() const { return m_kind == TyKind::ERROR; }
  };

  class TyTable
  {
  private:
    std::unordered_map<TyId, std::unique_ptr<TyInfo>> types;
    std::unordered_map<std::string, TyId> name_to_ty;
    std::unordered_map<TyId, TyId> array_type_cache; // element_type -> array_type
    std::unordered_map<TyId, TyId> ref_type_cache;   // pointee_type -> ref_type
    TyId next_ty_id;
    StructId next_struct_id;
    EnumId next_enum_id;

  public:
    TyTable();

    TyId register_builtin(const std::string &name, TyKind kind, TyId id);

    TyId register_struct(const std::string &name, StructId struct_id);

    TyId register_enum(const std::string &name, EnumId enum_id);

    TyId register_opaque(const std::string &name);

    TyId register_array(TyId element_type);

    TyId register_ref(TyId pointee_type);

    std::optional<TyId> lookup_by_name(const std::string &name) const;

    TyInfo *get_ty_info(TyId id);
    const TyInfo *get_ty_info(TyId id) const;

    bool has_ty(TyId id) const;
    bool has_ty_name(const std::string &name) const;

    StructId allocate_struct_id();
    EnumId allocate_enum_id();

    std::string ty_name(TyId id) const;

    bool is_numeric(TyId id) const;
    bool is_bool(TyId id) const;
    bool is_string(TyId id) const;
    bool is_void(TyId id) const;
    bool is_struct(TyId id) const;
    bool is_enum(TyId id) const;
    bool is_opaque(TyId id) const;
    bool is_array(TyId id) const;
    bool is_ref(TyId id) const;

    std::optional<TyId> get_array_element_type(TyId array_ty) const;
    std::optional<TyId> get_ref_pointee_type(TyId ref_ty) const;

    bool are_compatible(TyId lhs, TyId rhs) const;
  };

} // namespace air

#endif // AIR_TY_H_
