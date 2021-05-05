#ifndef IOSS_MESHCOPYOPTIONS_H
#define IOSS_MESHCOPYOPTIONS_H
/*
 * Copyright(C) 1999-2021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "vtk_ioss_mangle.h"

#include <vector>

namespace Ioss {
  struct MeshCopyOptions
  {
    std::vector<double> selected_times{};
    double              minimum_time{0.0};
    double              maximum_time{0.0};
    double              delay{0.0};
    int                 data_storage_type{0};
    bool                memory_statistics{false};
    bool                debug{false};
    bool                verbose{false};
    bool                ints_64_bit{false};
    bool                delete_timesteps{false};
    bool                reverse{false};     // Used for testing CGNS
    bool                add_proc_id{false}; // CGNS: Add proc_id field.
    bool boundary_sideset{false};           // Output a sideset of the boundary faces of the model

    // only used by Catalyst calls to `copy_database`; if false the
    // copy process skips the defining of the mesh geometry and the
    // defining of the field data, thus assuming it has already been
    // done. Used for calling `copy_database` multple times with
    // different timestep ranges.
    bool define_geometry{true};

    bool ignore_qa_info{false}; // In compare mode, ignore qa and info records.
  };
} // namespace Ioss
#endif
