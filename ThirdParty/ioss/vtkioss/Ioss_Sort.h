// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

#include <pdqsort.h>

#include <cstddef>
#include <vector>
#if 0

// This is used instead of the std::sort since we were having issues
// with the std::sort on some compiler versions with certain options
// enabled (-fopenmp).  If this shows up as a hotspot in performance
// measurements, then we can use std::sort on most platforms and just
// use this version where there are compiler issues.

namespace {
  const int QSORT_CUTOFF = 12;

  template <typename INT> void SWAP(INT *V, size_t I, size_t J) { std::swap(V[I], V[J]); }

  template <typename INT> void order3(INT v[], size_t left, size_t center, size_t right)
  {
    if (v[left] > v[center]) {
      SWAP(v, left, center);
    }
    if (v[left] > v[right]) {
      SWAP(v, left, right);
    }
    if (v[center] > v[right]) {
      SWAP(v, center, right);
    }
  }

  template <typename INT> size_t median3(INT v[], size_t left, size_t right)
  {
    size_t center = (left + right) / 2;
    size_t pl     = left;
    size_t pm     = center;
    size_t pr     = right;

    if (right - left > 40) {
      size_t s = (right - left) / 8;
      order3(v, left, left + s, left + 2 * s);
      order3(v, center - s, center, center + s);
      order3(v, right - 2 * s, right - s, right);

      // Now set up to get median of the 3 medians...
      pl = left + s;
      pm = center;
      pr = right - s;
    }
    order3(v, pl, pm, pr);

    SWAP(v, center, right - 1);
    return right - 1;
  }

  template <typename INT> void qsort_int(INT v[], size_t left, size_t right)
  {
    if (left + QSORT_CUTOFF < right) {
      size_t pivot = median3(v, left, right);
      size_t i     = left;
      size_t j     = right - 1;

      for (;;) {
        while (v[++i] < v[pivot]) {
          ;
        }
        while (v[--j] > v[pivot]) {
          ;
        }
        if (i < j) {
          SWAP(v, i, j);
        }
        else {
          break;
        }
      }

      SWAP(v, i, right - 1);
      qsort_int(v, left, i - 1);
      qsort_int(v, i + 1, right);
    }
  }

  template <typename INT> void isort_int(INT v[], size_t N)
  {
    size_t j;
    size_t ndx = 0;

    if (N <= 1) {
      return;
    }
    INT small = v[0];
    for (size_t i = 1; i < N; i++) {
      if (v[i] < small) {
        small = v[i];
        ndx   = i;
      }
    }
    /* Put smallest value in slot 0 */
    SWAP(v, 0, ndx);

    for (size_t i = 1; i < N; i++) {
      INT tmp = v[i];
      for (j = i; tmp < v[j - 1]; j--) {
        v[j] = v[j - 1];
      }
      v[j] = tmp;
    }
  }
} // namespace
#endif

namespace Ioss {
  template <typename INT> void qsort(std::vector<INT> &v)
  {
    if (v.size() <= 1) {
      return;
    }
#if 0
    qsort_int(v.data(), 0, v.size() - 1);
    isort_int(v.data(), v.size());
#else
    pdqsort(v.begin(), v.end());
#endif
  }

  template <class Iter, class Comp> inline void sort(Iter begin, Iter end, Comp compare)
  {
#if USE_STD_SORT
    std::sort(begin, end, compare);
#else
    pdqsort(begin, end, compare);
#endif
  }

  template <class Iter> inline void sort(Iter begin, Iter end)
  {
#if USE_STD_SORT
    std::sort(begin, end);
#else
    pdqsort(begin, end);
#endif
  }
} // namespace Ioss
