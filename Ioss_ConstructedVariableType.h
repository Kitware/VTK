// Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_VariableType.h" // for VariableType
#include <string>              // for string

namespace Ioss {
  class IOSS_EXPORT ConstructedVariableType : public VariableType
  {
  public:
    IOSS_NODISCARD std::string label(int which, char suffix_sep = '_') const override;

    IOSS_NODISCARD VariableType::Type type() const override { return Type::CONSTRUCTED; }
    IOSS_NODISCARD std::string type_string() const override { return "Constructed"; }

    ConstructedVariableType(const std::string &my_name, int number_components, bool delete_me);
    explicit ConstructedVariableType(int number_components, bool delete_me);
    ConstructedVariableType(const ConstructedVariableType &) = delete;
  };
} // namespace Ioss
