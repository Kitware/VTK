/*
 * Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"
#include <string>

#include "Ioss_VariableType.h"

namespace Ioss {
  class IOSS_EXPORT NamedSuffixVariableType : public VariableType
  {
  public:
    //  'which' is 1-based
    IOSS_NODISCARD std::string label(int which, const char /* suffix_sep */) const override
    {
      return suffixList[which - 1];
    }

    NamedSuffixVariableType(const std::string &my_name, int number_components, bool delete_me)
        : Ioss::VariableType(my_name, number_components, delete_me)
    {
      suffixList.resize(number_components);
      suffixList.assign(number_components, "UNSET");
    }
    NamedSuffixVariableType(const NamedSuffixVariableType &) = delete;

    IOSS_NODISCARD VariableType::Type type() const override { return Type::NAMED_SUFFIX; }
    IOSS_NODISCARD std::string type_string() const override { return "NamedSuffix"; }

    //! Define the suffix list for this field.
    //  'which' is 1-based to conform to the 'label' function usage.
    // If user doesn't add suffices, then 'label' will return "UNSET"
    void add_suffix(size_t which, const std::string &suffix) { suffixList[which - 1] = suffix; }

    void print() const override final;

  private:
    Ioss::NameList suffixList{};
  };
} // namespace Ioss
