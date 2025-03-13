// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {

  /*! The TopologyModified enumeration is used as an argument to the
   *  topology_modified() functions in io to
   *  specify the type of topology modification that has occurred.  The
   *  calls to topology_modified() are cumulative between
   *  output steps, so a call with TOPOLOGY_REORDER followed by a call
   *  with TOPOLOGY_SHUFFLE will do the right thing.  Typical examples
   *  of when these would be used are:
   *  - TOPOLOGY_SAME: No change, but easier to call function than not.
   *  - TOPOLOGY_REORDER: Element Death which reorders the Registrars
   *  - TOPOLOGY_SHUFFLE: Load Balancing
   *  - TOPOLOGY_HADAPT: H-Adaptivity
   *  - TOPOLOGY_GHOST: Ghost nodes/edges/faces/elements created/destroyed
   *  - TOPOLOGY_GEOMETRY: Model data is modified, overlap removal.
   *  - TOPOLOGY_CREATEDELETE: Surface erosion, particle creation
   *  - TOPOLOGY_UNKNOWN: Something else, catchall option.
   */
  enum TopologyModified {
    TOPOLOGY_SAME = (0), //!< No change, also used for initialization
    TOPOLOGY_REORDER =
        (1U << 0), //!< Data structures reordered on processor, no change between procs.
    TOPOLOGY_SHUFFLE = (1U << 1), //!< Globally the same, data moved among processors.
    TOPOLOGY_HADAPT  = (1U << 2), //!< Elements split/combined; not moved cross-proc
    TOPOLOGY_GHOST   = (1U << 3), //!< Ghost entities created/destroyed
    TOPOLOGY_GEOMETRY =
        (1U << 4), //!< Geometry (mesh coordinates) modified. Restart needs to know this.
    TOPOLOGY_CREATEFACE     = (1U << 5),  //!< Face/Edge are created/deleted.
    TOPOLOGY_CREATEELEM     = (1U << 6),  //!< Elements are created/deleted.
    TOPOLOGY_CREATENODE     = (1U << 7),  //!< Nodes are created/deleted.
    TOPOLOGY_CREATEASSEMBLY = (1U << 8),  //!< Assemblies are created/deleted.
    TOPOLOGY_UNKNOWN        = (1U << 9),  //!< Unknown change, recreate from scratch.
    TOPOLOGY_AUXILIARY      = (1U << 10), //!< An AUXILIARY relation was created/modified.
    TOPOLOGY_CONSTRAINT     = (1U << 11)  //!< Contact constraints

  };

  enum class FileControlOption { CONTROL_NONE, CONTROL_AUTO_MULTI_FILE, CONTROL_AUTO_GROUP_FILE };

} // namespace Ioss
