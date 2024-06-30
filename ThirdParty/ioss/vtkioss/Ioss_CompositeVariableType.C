// Copyright(C) 1999-2020, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CompositeVariableType.h"
#include "Ioss_VariableType.h"
#include <cassert>
#include <map>
#include <string>

namespace Ioss {
  std::string CompositeVariableType::composite_name(const std::string &base, int copies)
  {
    static std::string SEPARATOR("*");
    std::string        name = base;
    name += SEPARATOR;
    name += std::to_string(copies);
    return name;
  }

  VariableType *CompositeVariableType::composite_variable_type(const VariableType *inst, int copies)
  {
    VariableType *comp_inst = nullptr;

    // See if we already constructed this composite type...
    std::string composite_type = CompositeVariableType::composite_name(inst->name(), copies);

    auto iter = registry().find(composite_type);
    if (iter == registry().end()) {
      // Not found, construct new type...
      comp_inst = new CompositeVariableType(inst, copies, true);
    }
    else {
      comp_inst = (*iter).second;
    }
    return comp_inst;
  }

  CompositeVariableType::CompositeVariableType(const VariableType *base_type, int copies,
                                               bool delete_me)
      : VariableType(composite_name(base_type->name(), copies),
                     base_type->component_count() * copies, delete_me),
        baseType(base_type), copies_(copies)
  {
  }

  CompositeVariableType::CompositeVariableType(const std::string &my_name, int number_components,
                                               bool delete_me)
      : VariableType(my_name, number_components, delete_me), baseType(nullptr), copies_(0)
  {
  }

  std::string CompositeVariableType::label(int which, const char suffix_sep) const
  {
    static char tmp_sep[2];

    // NOTE: 'which' is 1-based
    assert(which > 0 && which <= component_count());

    int base_comp      = baseType->component_count();
    int which_instance = (which - 1) / base_comp;
    int which_base     = (which - 1) % base_comp;

    std::string my_label = baseType->label(which_base + 1);
    if (suffix_sep != 0 && base_comp > 1) {
      tmp_sep[0] = suffix_sep;
      my_label += tmp_sep;
    }
    my_label += VariableType::numeric_label(which_instance + 1, copies_, name());
    return my_label;
  }
} // namespace Ioss
