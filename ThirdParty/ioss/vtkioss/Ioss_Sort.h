// Copyright(C) 1999-2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

#if !defined(IOSS_USE_STD_SORT)
#include <pdqsort.h>
#endif

#include <cstddef>
#include <vector>

namespace Ioss {
  template <class Iter, class Comp> inline void sort(Iter begin, Iter end, Comp compare)
  {
#if defined(IOSS_USE_STD_SORT)
    std::sort(begin, end, compare);
#else
    pdqsort(begin, end, compare);
#endif
  }

  template <class Iter> inline void sort(Iter begin, Iter end)
  {
#if defined(IOSS_USE_STD_SORT)
    std::sort(begin, end);
#else
    pdqsort(begin, end);
#endif
  }

  template <typename INT> void sort(std::vector<INT> &v)
  {
    if (v.size() <= 1) {
      return;
    }
    Ioss::sort(v.begin(), v.end());
  }

} // namespace Ioss
