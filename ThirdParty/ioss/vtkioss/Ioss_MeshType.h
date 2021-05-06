// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_mesh_type_h
#define IOSS_mesh_type_h

#include "vtk_ioss_mangle.h"

namespace Ioss {
  /** \brief The mesh type -- structured, unstructured, hybrid (future), or unknown
   */
  enum class MeshType { UNKNOWN, STRUCTURED, UNSTRUCTURED, HYBRID };
} // namespace Ioss
#endif
