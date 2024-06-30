// Copyright(C) 1999-2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include <stddef.h>
#include <string>
#include <vector>

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Field;
  class VariableType;

  class IOSS_EXPORT Transform
  {
  public:
    virtual ~Transform() = default;
    IOSS_NODISCARD virtual const Ioss::VariableType                               *
    output_storage(const Ioss::VariableType *in) const = 0;
    IOSS_NODISCARD virtual size_t output_count(size_t in) const                      = 0;

    bool execute(const Ioss::Field &field, void *data);

    virtual void set_property(const std::string &name, int value);
    virtual void set_property(const std::string &name, double value);
    virtual void set_properties(const std::string &name, const std::vector<int> &values);
    virtual void set_properties(const std::string &name, const std::vector<double> &values);

    IOSS_NODISCARD static Transform *create(const std::string &transform);

  protected:
    Transform() = default;

    virtual bool internal_execute(const Ioss::Field &field, void *data) = 0;
  };
} // namespace Ioss
