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

#ifndef IOSS_Ioss_SurfaceSplit_h
#define IOSS_Ioss_SurfaceSplit_h

#include "vtk_ioss_mangle.h"

namespace Ioss {
  /** \brief Method used to split sidesets into homogeneous blocks.
   */
  enum SurfaceSplitType {
    SPLIT_INVALID          = -1,
    SPLIT_BY_TOPOLOGIES    = 1,
    SPLIT_BY_ELEMENT_BLOCK = 2,
    SPLIT_BY_DONT_SPLIT    = 3,
    SPLIT_LAST_ENTRY       = 4
  };

  /** \brief Convert an integer code for the method used to split sidesets into homogeneous blocks.
   *
   * \param[in] split_int The code.
   * \returns The corresponding SurfaceSplitType.
   */
  inline SurfaceSplitType int_to_surface_split(int split_int)
  {
    SurfaceSplitType split_type = Ioss::SPLIT_INVALID;
    if (split_int == 1) {
      split_type = Ioss::SPLIT_BY_TOPOLOGIES;
    }
    else if (split_int == 2) {
      split_type = Ioss::SPLIT_BY_ELEMENT_BLOCK;
    }
    else if (split_int == 3) {
      split_type = Ioss::SPLIT_BY_DONT_SPLIT;
    }
    return split_type;
  }
} // namespace Ioss
#endif
