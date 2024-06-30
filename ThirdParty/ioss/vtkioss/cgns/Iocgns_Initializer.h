// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "iocgns_export.h"
#include "vtk_ioss_mangle.h"

namespace Iocgns {
  class IOCGNS_EXPORT Initializer
  {
  public:
    Initializer();
    ~Initializer();
    // Copy constructor
    // Assignment operator
  private:
    static int useCount;
  };
} // namespace Iocgns
