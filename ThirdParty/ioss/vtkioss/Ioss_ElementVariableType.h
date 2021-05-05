// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_ElementVariableType_h
#define IOSS_Ioss_ElementVariableType_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_VariableType.h>
#include <string>

namespace Ioss {
  class ElementVariableType : public Ioss::VariableType
  {
  public:
    std::string label(int /*which*/, const char /*suffix_sep*/) const override { return ""; }
    std::string label_name(const std::string &base, int /*which*/,
                           const char /*suffix_sep*/) const override
    {
      return base;
    }
    int suffix_count() const override { return 0; }

  protected:
    ElementVariableType(const std::string &type, int comp_count);
  };

  inline ElementVariableType::ElementVariableType(const std::string &type, int comp_count)
      : Ioss::VariableType(type, comp_count, false)
  {
  }
} // namespace Ioss
#endif
