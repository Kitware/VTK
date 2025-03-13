// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include "Ioss_Utils.h"
#include <cstring>    // for strncmp
#include <functional> // for less
#include <map>        // for map, map<>::value_compare
#include <string>     // for string, operator<
#include <vector>     // for vector

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  struct Basis;
  struct QuadraturePoint;
  class VariableType;
} // namespace Ioss

namespace Ioss {
  using VariableTypeMap = std::map<std::string, VariableType *, std::less<>>;
  using VTM_ValuePair   = VariableTypeMap::value_type;

  class IOSS_EXPORT Registry
  {
    friend class VariableType;

  public:
    void           insert(const Ioss::VTM_ValuePair &value, bool delete_me);
    IOSS_NODISCARD VariableTypeMap::iterator begin() { return m_registry.begin(); }
    IOSS_NODISCARD VariableTypeMap::iterator end() { return m_registry.end(); }
    IOSS_NODISCARD VariableTypeMap::iterator find(const std::string &type)
    {
      return m_registry.find(type);
    }

    ~Registry();

  private:
    std::map<std::string, std::string> customFieldTypes;
    Ioss::VariableTypeMap              m_registry;
    std::vector<Ioss::VariableType *>  m_deleteThese;
  };

  struct IOSS_EXPORT Suffix
  {
    explicit Suffix(const char *new_data) : m_data(new_data) {}
    explicit Suffix(std::string new_data) : m_data(std::move(new_data)) {}
    IOSS_NODISCARD bool operator==(const std::string &str) const
    {
      return Utils::str_equal(m_data, str);
    }
    IOSS_NODISCARD bool operator!=(const std::string &str) const
    {
      return !Utils::str_equal(m_data, str);
    }
    IOSS_NODISCARD bool is_uppercase() const { return isalpha(m_data[0]) && isupper(m_data[0]); }

    std::string m_data{};
  };

  /** \brief A generic variable type
   */
  class IOSS_EXPORT VariableType
  {
  public:
    enum class Type {
      UNKNOWN,
      SCALAR,
      STANDARD,
      COMPOSED,
      COMPOSITE,
      CONSTRUCTED,
      ELEMENT,
      NAMED_SUFFIX,
      BASIS,
      QUADRATURE
    };

    static void                    alias(const std::string &base, const std::string &syn);
    static int                     describe(NameList *names);
    IOSS_NODISCARD static NameList describe();

    // Return a list of types that have been defined
    // externally... Basically a subset of the types in the
    // `Registry::m_deleteThese` list...
    IOSS_NODISCARD static std::vector<Ioss::VariableType *>
    external_types(Ioss::VariableType::Type type);

    static bool create_named_suffix_type(const std::string    &type_name,
                                         const Ioss::NameList &suffices);

    // Backward compatibility...
    [[deprecated("Use create_named_suffix_type")]] static bool
    create_named_suffix_field_type(const std::string &type_name, const Ioss::NameList &suffices)
    {
      return create_named_suffix_type(type_name, suffices);
    }

    static bool create_basis_type(const std::string &type_name, const Ioss::Basis &basis);
    static bool create_quadrature_type(const std::string                        &type_name,
                                       const std::vector<Ioss::QuadraturePoint> &quad_points);
    static bool get_field_type_mapping(const std::string &field, std::string *type);
    static bool add_field_type_mapping(const std::string &raw_field, const std::string &raw_type);

    VariableType(const VariableType &)            = delete;
    VariableType &operator=(const VariableType &) = delete;
    virtual ~VariableType()                       = default;

    virtual void print() const;

    IOSS_NODISCARD int component_count() const;

    // Override this function if the derived class has no suffices
    // For example, a 'vector_2d' has suffices "x" and "y"
    // A 'quad4' has no suffices...
    IOSS_NODISCARD virtual int suffix_count() const;
    IOSS_NODISCARD std::string         name() const;
    IOSS_NODISCARD virtual Type        type() const        = 0;
    IOSS_NODISCARD virtual std::string type_string() const = 0;

    IOSS_NODISCARD static std::string  numeric_label(int which, int ncomp, const std::string &name);
    IOSS_NODISCARD virtual std::string label(int which, char suffix_sep = '_') const = 0;
    IOSS_NODISCARD virtual std::string label_name(const std::string &base, int which,
                                                  char suffix_sep1 = '_', char suffix_sep2 = '_',
                                                  bool suffices_uppercase = false) const;
    IOSS_NODISCARD virtual bool        match(const std::vector<Suffix> &suffices) const;

    IOSS_NODISCARD static const VariableType *factory(const std::string &raw_name, int copies = 1);
    IOSS_NODISCARD static const VariableType *factory(const std::string &raw_name,
                                                      const std::string &secondary);
    IOSS_NODISCARD static const VariableType *factory(const std::vector<Suffix> &suffices,
                                                      bool ignore_realn_fields = false);

    static Registry &registry();

  protected:
    VariableType(const std::string &type, int comp_count, bool delete_me = false);

  private:
    const std::string name_{};
    int               componentCount{};

    static bool build_variable_type(const std::string &raw_type);
  };
} // namespace Ioss
inline std::string Ioss::VariableType::name() const { return name_; }

inline int Ioss::VariableType::component_count() const { return componentCount; }

inline int Ioss::VariableType::suffix_count() const { return componentCount; }
