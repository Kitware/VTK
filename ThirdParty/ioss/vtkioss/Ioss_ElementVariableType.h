// Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"
#include "Ioss_VariableType.h"
#include <string>

namespace Ioss {
  class IOSS_EXPORT ElementVariableType : public Ioss::VariableType
  {
  public:
    IOSS_NODISCARD std::string label(int /*which*/, const char /*suffix_sep*/) const override
    {
      return "";
    }
    IOSS_NODISCARD std::string label_name(const std::string &base, int /*which*/,
                                          const char /*suffix_sep*/, const char /*suffix_sep*/,
                                          bool /* suffices_uppercase */) const override
    {
      return base;
    }
    IOSS_NODISCARD int suffix_count() const override { return 0; }

    IOSS_NODISCARD VariableType::Type type() const override { return Type::ELEMENT; }
    IOSS_NODISCARD std::string type_string() const override { return "Element"; }

  protected:
    ElementVariableType(const std::string &type, int comp_count);
  };

  inline ElementVariableType::ElementVariableType(const std::string &type, int comp_count)
      : Ioss::VariableType(type, comp_count, false)
  {
  }
} // namespace Ioss
