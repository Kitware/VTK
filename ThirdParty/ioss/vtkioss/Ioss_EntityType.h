// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

namespace Ioss {
  /** \brief The particular type of GroupingEntity.
   */
  enum EntityType {
    NODEBLOCK       = 1,
    EDGEBLOCK       = 2,
    FACEBLOCK       = 4,
    ELEMENTBLOCK    = 8,
    NODESET         = 16,
    EDGESET         = 32,
    FACESET         = 64,
    ELEMENTSET      = 128,
    SIDESET         = 256,
    SURFACE         = 256, //: Same as sideset
    COMMSET         = 512,
    SIDEBLOCK       = 1024,
    REGION          = 2048,
    SUPERELEMENT    = 4096,
    STRUCTUREDBLOCK = 8192,
    ASSEMBLY        = 16384,
    BLOB            = 32768,
    INVALID_TYPE    = 65536
  };
  constexpr int entityTypeCount = 16;
} // namespace Ioss
