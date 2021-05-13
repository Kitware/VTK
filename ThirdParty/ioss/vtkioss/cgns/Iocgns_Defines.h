// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_IOCGNS_DEFINES_H
#define IOSS_IOCGNS_DEFINES_H

#include <vtk_cgns.h>
#include VTK_CGNS(cgnstypes.h)
#include <vector>

#define CGNS_MAX_NAME_LENGTH 255
using CGNSIntVector = std::vector<cgsize_t>;

#endif
