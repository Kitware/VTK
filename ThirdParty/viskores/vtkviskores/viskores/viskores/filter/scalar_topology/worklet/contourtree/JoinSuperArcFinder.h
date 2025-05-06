//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

//=======================================================================================
//
// COMMENTS:
//
// After the core join tree is constructed, we need to assign each vertex to a join superarc
// This was previously done with a set of rocking iterations, which burned extra memory and
// work.  The OpenMP version was therefore updated so that each vertex looped until it found
// it's destination arc.
//
// This functor implements that for use by a for_each call.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_join_super_arc_finder_h
#define viskores_worklet_contourtree_join_super_arc_finder_h

#include <viskores/filter/scalar_topology/worklet/contourtree/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/VertexValueComparator.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree
{

// Worklet for finding join superarc - expressed as a unary functor since it is not
// guaranteed to write back
// There will be no out-of-sequence writes, since:
// 1.  Critical points are already set and are simply skipped
// 2.  Regular points only read from critical points
// 3.  Regular points only write to critical points
//
template <typename T>
class JoinSuperArcFinder : public viskores::worklet::WorkletMapField
{
public:
  using TagType = viskores::List<T>;

  using ControlSignature = void(FieldIn vertex,           // (input) index into sorted edges
                                WholeArrayIn values,      // (input) data values
                                WholeArrayInOut saddles,  // (in out) saddles
                                WholeArrayInOut extrema); // (in out) maxima
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  bool isJoinTree;

  // Constructor
  VISKORES_EXEC_CONT
  JoinSuperArcFinder(bool IsJoinTree)
    : isJoinTree(IsJoinTree)
  {
  }

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& vertex,
                                const InFieldPortalType& values,
                                const OutFieldPortalType& saddles,
                                const OutFieldPortalType& extrema) const
  {
    VertexValueComparator<InFieldPortalType> lessThan(values);

    // local copies
    viskores::Id saddle = saddles.Get(vertex);
    viskores::Id extreme = extrema.Get(vertex);

    // now test for regular points
    if (saddle != extreme)
      return;

    // while loop
    while (lessThan(saddle, vertex, isJoinTree))
    { //  saddle above vertex
      extreme = extrema.Get(saddle);
      saddle = saddles.Get(saddle);
      // if we're in the trunk, break immediately
      if (saddle == NO_VERTEX_ASSIGNED)
        break;
    } // saddle above vertex

    // when done, write back to the array
    extrema.Set(vertex, extreme);
    saddles.Set(vertex, saddle);
  }
}; // JoinSuperArcFinder
}
}
}

#endif
