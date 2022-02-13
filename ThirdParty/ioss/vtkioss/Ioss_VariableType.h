// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_Utils.h>
#include <cstring>    // for strncmp
#include <functional> // for less
#include <map>        // for map, map<>::value_compare
#include <string>     // for string, operator<
#include <vector>     // for vector

namespace Ioss {
  class VariableType;
} // namespace Ioss

namespace Ioss {
  using VariableTypeMap = std::map<std::string, VariableType *, std::less<std::string>>;
  using VTM_ValuePair   = VariableTypeMap::value_type;

  class Registry
  {
  public:
    void                      insert(const Ioss::VTM_ValuePair &value, bool delete_me);
    VariableTypeMap::iterator begin() { return m_registry.begin(); }
    VariableTypeMap::iterator end() { return m_registry.end(); }
    VariableTypeMap::iterator find(const std::string &type) { return m_registry.find(type); }

    ~Registry();
    std::map<std::string, std::string> customFieldTypes;

  private:
    Ioss::VariableTypeMap             m_registry;
    std::vector<Ioss::VariableType *> m_deleteThese;
  };

  struct Suffix
  {
    explicit Suffix(const char *new_data) : m_data(new_data) {}
    explicit Suffix(const std::string &new_data) : m_data(new_data) {}
    bool operator==(const std::string &str) const { return Utils::str_equal(m_data, str); }
    bool operator!=(const std::string &str) const { return !Utils::str_equal(m_data, str); }
    bool is_uppercase() const { return isalpha(m_data[0]) && isupper(m_data[0]); }

    std::string m_data{};
  };

  /** \brief A generic variable type
   */
  class VariableType
  {
  public:
    static void     alias(const std::string &base, const std::string &syn);
    static int      describe(NameList *names);
    static NameList describe();
    static bool     create_named_suffix_field_type(const std::string              &type_name,
                                                   const std::vector<std::string> &suffices);
    static bool     get_field_type_mapping(const std::string &field, std::string *type);
    static bool add_field_type_mapping(const std::string &raw_field, const std::string &raw_type);

    VariableType(const VariableType &) = delete;
    VariableType &operator=(const VariableType &) = delete;
    virtual ~VariableType();

    int component_count() const;

    // Override this function if the derived class has no suffices
    // For example, a 'vector_2d' has suffices "x" and "y"
    // A 'quad4' has no suffices...
    virtual int suffix_count() const;
    std::string name() const;

    static std::string  numeric_label(int which, int ncomp, const std::string &name);
    virtual std::string label(int which, const char suffix_sep = '_') const = 0;
    virtual std::string label_name(const std::string &base, int which, char suffix_sep = '_',
                                   bool suffices_uppercase = false) const;
    virtual bool        match(const std::vector<Suffix> &suffices) const;

    static const VariableType *factory(const std::string &raw_name, int copies = 1);
    static const VariableType *factory(const std::vector<Suffix> &suffices);

  protected:
    VariableType(const std::string &type, int comp_count, bool delete_me = false);
    static Registry &registry();

  private:
    const std::string name_;
    int               componentCount;

    static bool build_variable_type(const std::string &raw_type);
  };
} // namespace Ioss
inline std::string Ioss::VariableType::name() const { return name_; }

inline int Ioss::VariableType::component_count() const { return componentCount; }

inline int Ioss::VariableType::suffix_count() const { return componentCount; }
