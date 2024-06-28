// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ConstructedVariableType.h"
#include "Ioss_VariableType.h"
#include <cassert>
#include <string>

Ioss::ConstructedVariableType::ConstructedVariableType(const std::string &my_name,
                                                       int number_components, bool delete_me)
    : Ioss::VariableType(my_name, number_components, delete_me)
{
}

Ioss::ConstructedVariableType::ConstructedVariableType(int number_components, bool delete_me)
    : Ioss::VariableType(std::string("Real[") + std::to_string(number_components) +
                             std::string("]"),
                         number_components, delete_me)
{
}

std::string Ioss::ConstructedVariableType::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  if (component_count() == 1) {
    return "";
  }
  return VariableType::numeric_label(which, component_count(), name());
}
