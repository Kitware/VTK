// Copyright(C) 1999-2022 National Technology & Engineering Solutions
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

  //! Copy the mesh in `region` to `output_region`.  Behavior can be controlled
  //! via options in `options`
  IOSS_EXPORT void copy_database(Ioss::Region &region, Ioss::Region &output_region,
                                 Ioss::MeshCopyOptions &options);

  IOSS_EXPORT void transfer_assemblies(Ioss::Region &region, Ioss::Region &output_region,
                                       const Ioss::MeshCopyOptions &options, int rank);

  IOSS_EXPORT void transfer_blobs(Ioss::Region &region, Ioss::Region &output_region,
                                  const Ioss::MeshCopyOptions &options, int rank);

  IOSS_EXPORT void transfer_coordinate_frames(Ioss::Region &region, Ioss::Region &output_region);

} // namespace Ioss
