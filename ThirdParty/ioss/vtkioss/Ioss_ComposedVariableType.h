// Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"
#include "Ioss_VariableType.h" // for VariableType
#include <string>              // for string

namespace Ioss {
  class IOSS_EXPORT ComposedVariableType : public VariableType
  {
  public:
    IOSS_NODISCARD static std::string   composed_name(const std::string &base,
                                                      const std::string &secondary);
    IOSS_NODISCARD static VariableType *composed_variable_type(const VariableType *inst,
                                                               const VariableType *secondary);

    IOSS_NODISCARD VariableType::Type type() const override { return Type::COMPOSED; }
    IOSS_NODISCARD std::string type_string() const override;

    IOSS_NODISCARD std::string label(int which, char suffix_sep = '_') const override;

    ComposedVariableType(const VariableType *base_type, const VariableType *secondary,
                         bool delete_me);
    ComposedVariableType(const ComposedVariableType &) = delete;

    IOSS_NODISCARD const VariableType *get_base_type() const { return baseType; }
    IOSS_NODISCARD const VariableType *get_secondary_type() const { return secondaryType; }

  private:
    const VariableType *baseType{nullptr};
    const VariableType *secondaryType{nullptr};
  };
} // namespace Ioss
