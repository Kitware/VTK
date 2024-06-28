// Copyright(C) 1999-2020, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ComposedVariableType.h"
#include "Ioss_VariableType.h"
#include <cassert>
#include <map>
#include <string>

namespace Ioss {
  std::string ComposedVariableType::composed_name(const std::string &base,
                                                  const std::string &secondary)
  {
    static std::string SEPARATOR("*");
    std::string        name = base;
    name += SEPARATOR;
    name += secondary;
    return name;
  }

  VariableType *ComposedVariableType::composed_variable_type(const VariableType *inst,
                                                             const VariableType *secondary)
  {
    // See if we already constructed this composed type...
    std::string composed_type =
        ComposedVariableType::composed_name(inst->name(), secondary->name());

    VariableType *comp_inst = nullptr;
    auto          iter      = registry().find(composed_type);
    if (iter == registry().end()) {
      // Not found, construct new type...
      comp_inst = new ComposedVariableType(inst, secondary, true);
    }
    else {
      comp_inst = (*iter).second;
    }
    return comp_inst;
  }

  ComposedVariableType::ComposedVariableType(const VariableType *base_type,
                                             const VariableType *secondary_type, bool delete_me)
      : VariableType(composed_name(base_type->name(), secondary_type->name()),
                     base_type->component_count() * secondary_type->component_count(), delete_me),
        baseType(base_type), secondaryType(secondary_type)
  {
  }

  std::string ComposedVariableType::label(int which, const char suffix_sep) const
  {
    static char tmp_sep[2];

    // NOTE: 'which' is 1-based
    assert(which > 0 && which <= component_count());

    int base_comp      = baseType->component_count();
    int copies         = secondaryType->component_count();
    int which_instance = (which - 1) / base_comp;
    int which_base     = (which - 1) % base_comp;

    std::string my_label = baseType->label(which_base + 1);
    if (suffix_sep != 0 && base_comp > 1) {
      tmp_sep[0] = suffix_sep;
      my_label += tmp_sep;
    }
    my_label += VariableType::numeric_label(which_instance, copies, name());
    return my_label;
  }
} // namespace Ioss
