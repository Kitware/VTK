/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#ifndef IOSS_Tracer_h
#define IOSS_Tracer_h

#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Tracer
  {
  public:
    explicit Tracer(const char *function);
    ~Tracer();

  private:
    static int level;
  };
} // namespace Ioss
#endif
