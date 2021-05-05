// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_DataSize_h
#define IOSS_Ioss_DataSize_h

#include "vtk_ioss_mangle.h"

namespace Ioss {

  /** \brief The number of bytes used to store an integer type.
   */
  enum DataSize { USE_INT32_API = 4, USE_INT64_API = 8 };
} // namespace Ioss
#endif // IOSS_Ioss_DataSize_h
