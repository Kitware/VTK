// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

namespace Ioss {

  /** \brief The number of bytes used to store an integer type.
   */
  enum DataSize { USE_INT32_API = 4, USE_INT64_API = 8 };
} // namespace Ioss
