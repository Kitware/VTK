// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef IOSS_Ioss_Sort_h
#define IOSS_Ioss_Sort_h

#include "vtk_ioss_mangle.h"

#include <cstddef>
#include <vector>

// This is used instead of the std::sort since we were having issues
// with the std::sort on some compiler versions with certain options
// enabled (-fopenmp).  If this shows up as a hotspot in performance
// measurements, then we can use std::sort on most platforms and just
// use this version where there are compiler issues.

// Using Explicit Template Instantiation with the types:
//
// std::vector<int>, std::vector<int64_t>, std::vector<std::pair<int64_t,int64_t>>
//
// Update in Ioss_Sort.C if other types are needed.

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
    size_t pivot;
    size_t i, j;

    if (left + QSORT_CUTOFF < right) {
      pivot = median3(v, left, right);
      i     = left;
      j     = right - 1;

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
    size_t i, j;
    size_t ndx = 0;
    INT    small;
    INT    tmp;

    if (N <= 1) {
      return;
    }
    small = v[0];
    for (i = 1; i < N; i++) {
      if (v[i] < small) {
        small = v[i];
        ndx   = i;
      }
    }
    /* Put smallest value in slot 0 */
    SWAP(v, 0, ndx);

    for (i = 1; i < N; i++) {
      tmp = v[i];
      for (j = i; tmp < v[j - 1]; j--) {
        v[j] = v[j - 1];
      }
      v[j] = tmp;
    }
  }
} // namespace

namespace Ioss {
  template <typename INT> void qsort(std::vector<INT> &v)
  {
    if (v.size() <= 1) {
      return;
    }
    qsort_int(v.data(), 0, v.size() - 1);
    isort_int(v.data(), v.size());
  }
} // namespace Ioss

#endif
