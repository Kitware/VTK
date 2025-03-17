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
  class IOSS_EXPORT CompositeVariableType : public VariableType
  {
  public:
    IOSS_NODISCARD static std::string   composite_name(const std::string &base, int copies);
    IOSS_NODISCARD static VariableType *composite_variable_type(const VariableType *inst,
                                                                int                 copies);

    IOSS_NODISCARD VariableType::Type type() const override { return Type::COMPOSITE; }
    IOSS_NODISCARD std::string type_string() const override;

    IOSS_NODISCARD std::string label(int which, char suffix_sep = '_') const override;
    CompositeVariableType(const std::string &my_name, int number_components, bool delete_me);
    CompositeVariableType(const VariableType *base_type, int copies, bool delete_me);
    CompositeVariableType(const CompositeVariableType &) = delete;

    IOSS_NODISCARD const VariableType *get_base_type() const { return baseType; }
    IOSS_NODISCARD int                 get_num_copies() const { return copies_; }

    // Kept for backward compatibility...
    IOSS_NODISCARD [[deprecated("Use get_base_type")]] const VariableType *GetBaseType() const
    {
      return baseType;
    }
    IOSS_NODISCARD [[deprecated("Use get_num_copies")]] int GetNumCopies() const { return copies_; }

  private:
    const VariableType *baseType;
    int                 copies_;
  };
} // namespace Ioss
