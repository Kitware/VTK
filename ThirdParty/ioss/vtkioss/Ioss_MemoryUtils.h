// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once
#include <cstddef> // for size_t

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class IOSS_EXPORT MemoryUtils
  {
  public:
    /** \brief Return amount of memory being used on this processor */
    static size_t get_memory_info();

    /** \brief Return maximum amount of memory that was used on this processor */
    static size_t get_hwm_memory_info();
  };
} // namespace Ioss
