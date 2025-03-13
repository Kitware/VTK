#pragma once
/*
 * Copyright(C) 1999-2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#include <vector>

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  struct IOSS_EXPORT MeshCopyOptions
  {
    std::vector<double>      selected_times{};
    std::vector<std::string> omitted_sets{};
    std::string              selected_change_sets{};
    double                   minimum_time{0.0};
    double                   maximum_time{0.0};
    double                   delay{0.0};
    double                   time_scale{1.0};
    double                   time_offset{0.0};

    double rel_tolerance{};
    double abs_tolerance{};
    double tol_floor{};

    // POINTER=1, STD_VECTOR=2, KOKKOS_VIEW_1D=3, KOKKOS_VIEW_2D=4,
    // KOKKOS_VIEW_2D_LAYOUTRIGHT_HOSTSPACE=5
    int  data_storage_type{0};
    bool memory_statistics{false};
    bool debug{false};
    bool verbose{false};
    bool output_summary{false};
    bool ints_64_bit{false};
    bool delete_timesteps{false};
    bool reverse{false};          // Used for testing CGNS
    bool add_proc_id{false};      // CGNS: Add proc_id field.
    bool boundary_sideset{false}; // Output a sideset of the boundary faces of the model
    bool omitted_blocks{false};

    // only used by Catalyst calls to `copy_database`; if false the
    // copy process skips the defining of the mesh geometry and the
    // defining of the field data, thus assuming it has already been
    // done. Used for calling `copy_database` multiple times with
    // different timestep ranges.
    bool define_geometry{true};

    bool ignore_qa_info{false}; // In compare mode, ignore qa and info records.
  };
} // namespace Ioss
