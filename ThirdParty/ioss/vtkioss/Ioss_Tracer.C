// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Tracer.h"
#include "Ioss_Utils.h"
#include <cassert>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)

namespace Ioss {
  int Tracer::level;

  Tracer::Tracer(const char *function)
  {
    fmt::print(Ioss::DebugOut(), "Entering Function: {} at level {}\n", function, ++level);
    assert(level == 1);
  }

  Tracer::~Tracer() { --level; }
} // namespace Ioss
