// Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Region;
  struct MeshCopyOptions;
} // namespace Ioss

namespace Ioss {
  /* \brief Methods to compare databases.
   */
  class IOSS_EXPORT Compare
  {
  public:
    // Compare the mesh in 'input_region_1' to 'input_region_2'.  Behavior can be controlled
    // via options in 'options'
    static bool compare_database(Ioss::Region &input_region_1, Ioss::Region &input_region_2,
                                 const Ioss::MeshCopyOptions &options);
  };
} // namespace Ioss
