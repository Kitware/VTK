// Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include <cstdint> // for int64_t
#include <vector>  // for vector

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class DatabaseIO;

  class IOSS_EXPORT CoordinateFrame
  {
  public:
    CoordinateFrame(int64_t my_id, char my_tag, const double *point_list);

    IOSS_NODISCARD int64_t       id() const;
    IOSS_NODISCARD char          tag() const;
    IOSS_NODISCARD const double *coordinates() const;
    IOSS_NODISCARD const double *origin() const;
    IOSS_NODISCARD const double *axis_3_point() const;
    IOSS_NODISCARD const double *plane_1_3_point() const;

    IOSS_NODISCARD bool operator!=(const Ioss::CoordinateFrame &rhs) const;
    IOSS_NODISCARD bool operator==(const Ioss::CoordinateFrame &rhs) const;
    IOSS_NODISCARD bool equal(const Ioss::CoordinateFrame &rhs) const;

  private:
    IOSS_NODISCARD bool equal_(const Ioss::CoordinateFrame &rhs, bool quiet) const;
    std::vector<double> pointList_{};
    int64_t             id_{};
    char                tag_;
  };
} // namespace Ioss
