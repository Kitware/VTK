// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_BasisVariableType.h"
#include "Ioss_ComposedVariableType.h"
#include "Ioss_CompositeVariableType.h"
#include "Ioss_ConstructedVariableType.h"
#include "Ioss_NamedSuffixVariableType.h"
#include "Ioss_QuadratureVariableType.h"
#include "Ioss_Utils.h"
#include "Ioss_VariableType.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Ioss_CodeTypes.h"

namespace Ioss {
  void Registry::insert(const VTM_ValuePair &value, bool delete_me)
  {
    m_registry.insert(value);
    if (delete_me) {
      m_deleteThese.push_back(value.second);
    }
  }

  Registry::~Registry()
  {
    for (const auto &entry : m_deleteThese) {
      delete entry;
    }
  }

  VariableType::VariableType(const std::string &type, int comp_count, bool delete_me)
      : name_(type), componentCount(comp_count)
  {
    std::string low_type = Utils::lowercase(type);
    registry().insert(VTM_ValuePair(low_type, this), delete_me);
  }

  VariableType::Type VariableType::type() const { return Type::UNKNOWN; }
  std::string        VariableType::type_string() const { return "Unknown"; }
  std::string        CompositeVariableType::type_string() const
  {
    return fmt::format("Composite: {}*{}", baseType->type_string(), copies_);
  }
  std::string ComposedVariableType::type_string() const
  {
    return fmt::format("Composed: {}*{}", baseType->type_string(), secondaryType->type_string());
  }

  void VariableType::print() const
  {
    fmt::print("\tVariableType '{}' of type '{}' with {} components.\n\n", name(), type_string(),
               component_count());
  }

  void BasisVariableType::print() const
  {
    fmt::print("\tVariableType '{}' of type '{}' with {} components\n\t\tordinal  subc: _dim, "
               "_ordinal, _dof_ordinal, _num_dof\t    xi     eta    zeta\n",
               name(), type_string(), component_count());
    for (int i = 0; i < component_count(); i++) {
      auto basis = get_basis_component(i + 1);
      fmt::print("\t\t {:6}\t\t{:6}\t{:6}\t{:6}\t{:6}\t\t{:6.3}\t{:6.3}\t{:6.3}\n", i + 1,
                 basis.subc_dim, basis.subc_ordinal, basis.subc_dof_ordinal, basis.subc_num_dof,
                 basis.xi, basis.eta, basis.zeta);
    }
    fmt::print("\n");
  }

  void QuadratureVariableType::print() const
  {
    fmt::print("\tVariableType '{}' of type '{}' with {} components\n\t\t\t    xi     eta    zeta  "
               "weight\n",
               name(), type_string(), component_count());
    for (int i = 0; i < component_count(); i++) {
      auto quad = get_quadrature_component(i + 1);
      fmt::print("\t\t{}\t{:6.3}\t{:6.3}\t{:6.3}\t{:6.3}\n", i + 1, quad.xi, quad.eta, quad.zeta,
                 quad.weight);
    }
    fmt::print("\n");
  }

  void NamedSuffixVariableType::print() const
  {
    fmt::print("\tVariableType '{}' of type '{}' with {} components\n\t\tSuffices: {}\n\n", name(),
               type_string(), component_count(), fmt::join(suffixList, ", "));
  }

  void VariableType::alias(const std::string &base, const std::string &syn)
  {
    registry().insert(
        VTM_ValuePair(Utils::lowercase(syn), const_cast<VariableType *>(factory(base))), false);
  }

  Registry &VariableType::registry()
  {
    static Registry registry_;
    return registry_;
  }

  /** \brief Get the names of variable types known to IOSS.
   *
   *  \param[out] names The list of known variable type names.
   *  \returns The number of known variable types.
   */
  int VariableType::describe(NameList *names)
  {
    int count = 0;
    for (auto &entry : registry()) {
      names->push_back(entry.first);
      count++;
    }
    return count;
  }

  /** \brief Get the names of variable types known to IOSS.
   *
   *  \returns The list of known variable type names.
   */
  Ioss::NameList VariableType::describe()
  {
    Ioss::NameList names;
    describe(&names);
    return names;
  }

  std::vector<Ioss::VariableType *> VariableType::external_types(Ioss::VariableType::Type type)
  {
    auto vars = registry().m_deleteThese;

    std::vector<Ioss::VariableType *> user_vars;
    if (type == Ioss::VariableType::Type::UNKNOWN) {
      user_vars = vars;
    }
    else {
      for (auto *var : vars) {
        if (var->type() == type) {
          user_vars.push_back(var);
        }
      }
    }
    return user_vars;
  }

  bool VariableType::add_field_type_mapping(const std::string &raw_field,
                                            const std::string &raw_type)
  {
    // See if storage type 'type' exists...
    std::string field = Utils::lowercase(raw_field);
    std::string type  = Utils::lowercase(raw_type);
    if (registry().find(type) == registry().end()) {
      return false;
    }

    // Add mapping.
    return registry().customFieldTypes.insert(std::make_pair(field, type)).second;
  }

  bool VariableType::create_named_suffix_type(const std::string    &type_name,
                                              const Ioss::NameList &suffices)
  {
    size_t count = suffices.size();
    if (count < 1) {
      return false;
    }

    std::string low_name = Utils::lowercase(type_name);
    // See if the variable already exists...
    if (registry().find(low_name) != registry().end()) {
      return false;
    }

    // Create the variable.  Note that the 'true' argument means Ioss will delete
    // the pointer.
    auto *var_type = new NamedSuffixVariableType(low_name, count, true);

    for (size_t i = 0; i < count; i++) {
      var_type->add_suffix(i + 1, suffices[i]);
    }
    return true;
  }

  bool VariableType::create_basis_type(const std::string &type_name, const Ioss::Basis &basis)
  {
    // See if the variable already exists...
    std::string basis_name = Utils::lowercase(type_name);
    if (registry().find(basis_name) != registry().end()) {
      return false;
    }

    // Create the variable.  Note that the 'true' argument means Ioss will delete
    // the pointer.
    new BasisVariableType(type_name, basis, true);
    return true;
  }

  bool VariableType::create_quadrature_type(const std::string                        &type_name,
                                            const std::vector<Ioss::QuadraturePoint> &quad_points)
  {
    size_t count = quad_points.size();
    if (count < 1) {
      return false;
    }

    // See if the variable already exists...
    std::string quad_name = Utils::lowercase(type_name);
    if (registry().find(quad_name) != registry().end()) {
      return false;
    }

    // Create the variable.  Note that the 'true' argument means Ioss will delete
    // the pointer.
    auto *var_type = new QuadratureVariableType(type_name, quad_points, true);
    return (var_type != nullptr);
  }

  bool VariableType::get_field_type_mapping(const std::string &field, std::string *type)
  {
    // Returns true if a mapping exists, 'type' contains the mapped type.
    // Returns false if no custom mapping exists for this field.
    std::string low_field = Utils::lowercase(field);

    if (registry().customFieldTypes.find(low_field) == registry().customFieldTypes.end()) {
      return false;
    }

    *type = registry().customFieldTypes.find(low_field)->second;
    return true;
  }

  const VariableType *VariableType::factory(const std::string &raw_name, int copies)
  {
    VariableType *inst    = nullptr;
    std::string   lc_name = Utils::lowercase(raw_name);
    auto          iter    = registry().find(lc_name);
    if (iter == registry().end()) {
      bool can_construct = build_variable_type(lc_name);
      if (can_construct) {
        iter = registry().find(lc_name);
        assert(iter != registry().end());
        inst = (*iter).second;
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: The variable type '{}' is not supported.\n", raw_name);
        IOSS_ERROR(errmsg);
      }
    }
    else {
      inst = (*iter).second;
    }

    if (copies != 1) {
      inst = CompositeVariableType::composite_variable_type(inst, copies);
    }
    assert(inst != nullptr);
    return inst;
  }

  const VariableType *VariableType::factory(const std::string &raw_name,
                                            const std::string &secondary)
  {
    VariableType *inst    = nullptr;
    std::string   lc_name = Utils::lowercase(raw_name);
    auto          iter    = registry().find(lc_name);
    if (iter == registry().end()) {
      bool can_construct = build_variable_type(lc_name);
      if (can_construct) {
        iter = registry().find(lc_name);
        assert(iter != registry().end());
        inst = (*iter).second;
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: The variable type '{}' is not supported.\n", raw_name);
        IOSS_ERROR(errmsg);
      }
    }
    else {
      inst = (*iter).second;
    }

    if (!secondary.empty()) {
      auto *sec_type = factory(secondary);
      if (sec_type != nullptr) {
        inst = ComposedVariableType::composed_variable_type(inst, sec_type);
      }
    }
    assert(inst != nullptr);
    return inst;
  }

  const VariableType *VariableType::factory(const std::vector<Suffix> &suffices,
                                            bool                       ignore_realn_fields)
  {
    size_t              size = suffices.size();
    const VariableType *ivt  = nullptr;
    if (size <= 1) {
      return nullptr; // All storage types must have at least 2 components.
    }

    bool matches = false;
    for (const auto &vtype : registry()) {
      auto *tst_ivt = vtype.second;
      if (ignore_realn_fields && Ioss::Utils::substr_equal("Real", tst_ivt->name())) {
        continue;
      }
      if (tst_ivt->suffix_count() == static_cast<int>(size)) {
        if (tst_ivt->match(suffices)) {
          ivt     = tst_ivt;
          matches = true;
          break;
        }
      }
    }

    if (!matches && !ignore_realn_fields) {
      matches = true;
      // Check if the suffices form a sequence (1,2,3,...,N)
      // This indicates a "component" variable type that is
      // constructed "on-the-fly" for use in Sierra
      //
      size_t width = Ioss::Utils::number_width(size);
      for (size_t i = 0; i < size; i++) {
        std::string digits = fmt::format("{:0{}}", i + 1, width);
        if (!Ioss::Utils::str_equal(suffices[i].m_data, digits)) {
          matches = false;
          break;
        }
      }
      if (matches) {
        // Create a new type.  For now, assume that the base type is
        // "Real" (Sierra type).  The name of the new type is
        // "Real[component_count]"
        // Note that this type has not yet been constructed since
        // it would have been found above.
        ivt = new ConstructedVariableType(size, true);
      }
    }
    return ivt;
  }

  bool VariableType::match(const std::vector<Suffix> &suffices) const
  {
    bool result = true;
    if (static_cast<int>(suffices.size()) == suffix_count()) {
      for (int i = 0; i < suffix_count(); i++) {
        if (suffices[i] != label(i + 1)) {
          result = false;
          break;
        }
      }
    }
    else {
      result = false;
    }
    return result;
  }

  std::string VariableType::label_name(const std::string &base, int which, const char suffix_sep1,
                                       const char suffix_sep2, bool suffices_uppercase) const
  {
    std::string my_name = base;
    std::string suffix  = label(which, suffix_sep2);
    if (!suffix.empty()) {
      if (suffix_sep1 != 0) {
        my_name += suffix_sep1;
      }
      if (suffices_uppercase) {
        my_name += Ioss::Utils::uppercase(suffix);
      }
      else {
        my_name += suffix;
      }
    }
    return my_name;
  }

  bool VariableType::build_variable_type(const std::string &raw_type)
  {
    // See if this is a multi-component instance of a base type.
    // An example would be REAL[2] which is a basic real type with
    // two components.  The suffices would be .0 and .1

    std::string type = Utils::lowercase(raw_type);

    // Step 0:
    // See if the type contains '[' and ']'
    char const *typestr = type.c_str();
    char const *lbrace  = std::strchr(typestr, '[');
    char const *rbrace  = std::strrchr(typestr, ']');

    if (lbrace == nullptr || rbrace == nullptr) {
      return false;
    }

    // Step 1:
    // First, we split off the basename (REAL/INTEGER) from the component count
    // ([2])
    // and see if the basename is a valid variable type and the count is a
    // valid integer.
    size_t len      = type.length() + 1;
    auto  *typecopy = new char[len];
    Utils::copy_string(typecopy, typestr, len);

    char *base = std::strtok(typecopy, "[]");
    assert(base != nullptr);
    auto iter = VariableType::registry().find(base);
    if (iter == registry().end()) {
      delete[] typecopy;
      return false;
    }

    char *countstr = std::strtok(nullptr, "[]");
    assert(countstr != nullptr);
    int count = std::strtol(countstr, nullptr, 10);
    if (count <= 0) {
      delete[] typecopy;
      return false;
    }

    // We now know we have a valid base type and an integer
    // specifying the number of 'components' in our new type.
    // Create the new type and register it in the registry...
    new ConstructedVariableType(type, count, true);
    delete[] typecopy;
    return true;
  }

  std::string VariableType::numeric_label(int which, int ncomp, const std::string &name)
  {
    if (ncomp >= 100000) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Variable '{}' has {} components which is larger than the current maximum"
                 " of 100,000. Please contact developer.\n",
                 name, fmt::group_digits(ncomp));
      IOSS_ERROR(errmsg);
    }

    size_t      width  = Ioss::Utils::number_width(ncomp);
    std::string digits = fmt::format("{:0{}}", which, width);
    return digits;
  }
} // namespace Ioss
