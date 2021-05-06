// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_BoundingBox_h
#define IOSS_Ioss_BoundingBox_h

#include "vtk_ioss_mangle.h"

namespace Ioss {
  struct AxisAlignedBoundingBox
  {
    AxisAlignedBoundingBox() = default;

    AxisAlignedBoundingBox(double xm, double ym, double zm, double xM, double yM, double zM)
        : xmin(xm), ymin(ym), zmin(zm), xmax(xM), ymax(yM), zmax(zM)
    {
    }

    double xmin{};
    double ymin{};
    double zmin{};

    double xmax{};
    double ymax{};
    double zmax{};
  };
} // namespace Ioss

#endif
