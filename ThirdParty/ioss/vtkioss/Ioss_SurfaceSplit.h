// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

namespace Ioss {
  /** \brief Method used to split sidesets into homogeneous blocks.
   */
  enum SurfaceSplitType {
    SPLIT_INVALID          = -1,
    SPLIT_BY_TOPOLOGIES    = 1,
    SPLIT_BY_ELEMENT_BLOCK = 2,
    SPLIT_BY_DONT_SPLIT    = 3,
    SPLIT_LAST_ENTRY       = 4
  };

  /** \brief Convert an integer code for the method used to split sidesets into homogeneous blocks.
   *
   * \param[in] split_int The code.
   * \returns The corresponding SurfaceSplitType.
   */
  inline SurfaceSplitType int_to_surface_split(int split_int)
  {
    SurfaceSplitType split_type = Ioss::SPLIT_INVALID;
    if (split_int == 1) {
      split_type = Ioss::SPLIT_BY_TOPOLOGIES;
    }
    else if (split_int == 2) {
      split_type = Ioss::SPLIT_BY_ELEMENT_BLOCK;
    }
    else if (split_int == 3) {
      split_type = Ioss::SPLIT_BY_DONT_SPLIT;
    }
    return split_type;
  }
} // namespace Ioss
