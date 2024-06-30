/*
 * Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class IOSS_EXPORT Tracer
  {
  public:
    explicit Tracer(const char *function);
    ~Tracer();

  private:
    static int level;
  };
} // namespace Ioss
