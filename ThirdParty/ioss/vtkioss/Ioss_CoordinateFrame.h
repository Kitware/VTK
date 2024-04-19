// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"

#include "vtk_ioss_mangle.h"

#include <cstdint> // for int64_t
#include <vector>  // for vector

namespace Ioss {
  class DatabaseIO;

  class IOSS_EXPORT CoordinateFrame
  {
  public:
    CoordinateFrame(int64_t my_id, char my_tag, const double *point_list);

    int64_t       id() const;
    char          tag() const;
    const double *coordinates() const;
    const double *origin() const;
    const double *axis_3_point() const;
    const double *plane_1_3_point() const;

    bool operator!=(const Ioss::CoordinateFrame &rhs) const;
    bool operator==(const Ioss::CoordinateFrame &rhs) const;
    bool equal(const Ioss::CoordinateFrame &rhs) const;

  private:
    bool                equal_(const Ioss::CoordinateFrame &rhs, bool quiet) const;
    std::vector<double> pointList_{};
    int64_t             id_{};
    char                tag_;
  };
} // namespace Ioss
