// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

// This class should be included if you want to use the lib::fmt
// output for any of the classes shown below:
// * ZoneConnectivity
// * Field
// * BoundaryCondition
#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)

#if FMT_VERSION >= 90000
#include "Ioss_Field.h"
#include "Ioss_StructuredBlock.h"
#include "Ioss_ZoneConnectivity.h"

namespace fmt {
  template <> struct formatter<Ioss::ZoneConnectivity> : ostream_formatter
  {
  };

  template <> struct formatter<Ioss::Field> : ostream_formatter
  {
  };

  template <> struct formatter<Ioss::BoundaryCondition> : ostream_formatter
  {
  };
} // namespace fmt
#endif
