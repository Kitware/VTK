// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_ConstructedVariableType_h
#define IOSS_Ioss_ConstructedVariableType_h

#include "vtk_ioss_mangle.h"

#include <Ioss_VariableType.h> // for VariableType
#include <string>              // for string

namespace Ioss {
  class ConstructedVariableType : public VariableType
  {
  public:
    std::string label(int which, char suffix_sep = '_') const override;
    ConstructedVariableType(const std::string &my_name, int number_components, bool delete_me);
    explicit ConstructedVariableType(int number_components, bool delete_me);
    ConstructedVariableType(const ConstructedVariableType &) = delete;
  };
} // namespace Ioss
#endif
