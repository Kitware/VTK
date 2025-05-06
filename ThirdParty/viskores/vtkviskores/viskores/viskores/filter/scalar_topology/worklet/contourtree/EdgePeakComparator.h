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
// A comparator that sorts edges by:
// i.   the chain maximum for the upper end of the edge
//          this clusters all edges together that lead to the chain maximum
// ii.  the index of the low end of the edge
//          this sorts the edges for the chain max by the low end
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_edge_peak_comparator_h
#define viskores_worklet_contourtree_edge_peak_comparator_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ExecutionObjectBase.h>

namespace viskores
{
namespace worklet
{
namespace contourtree
{

// Comparator for edges to sort governing saddles high
template <typename T, typename StorageType>
class EdgePeakComparator : public viskores::cont::ExecutionObjectBase
{
public:
  using ValueArrayType = viskores::cont::ArrayHandle<T, StorageType>;
  using IdArrayType = viskores::cont::ArrayHandle<viskores::Id>;

  VISKORES_CONT
  EdgePeakComparator(ValueArrayType values,
                     IdArrayType valueIndex,
                     IdArrayType edgeFar,
                     IdArrayType edgeNear,
                     IdArrayType arcArray,
                     bool isJoinGraph)
    : Values(values)
    , ValueIndex(valueIndex)
    , EdgeFar(edgeFar)
    , EdgeNear(edgeNear)
    , ArcArray(arcArray)
    , IsJoinGraph(isJoinGraph)
  {
  }

  ValueArrayType Values;
  IdArrayType ValueIndex;
  IdArrayType EdgeFar;
  IdArrayType EdgeNear;
  IdArrayType ArcArray;
  bool IsJoinGraph;

  class ExecObject
  {
  public:
    using ValuePortalType = typename ValueArrayType::ReadPortalType;
    using IdPortalType = typename IdArrayType::ReadPortalType;

    VISKORES_CONT
    ExecObject(ValuePortalType values,
               IdPortalType valueIndex,
               IdPortalType edgeFar,
               IdPortalType edgeNear,
               IdPortalType arcArray,
               bool isJoinGraph)
      : Values(values)
      , ValueIndex(valueIndex)
      , EdgeFar(edgeFar)
      , EdgeNear(edgeNear)
      , ArcArray(arcArray)
      , IsJoinGraph(isJoinGraph)
    {
    }

    ValuePortalType Values;
    IdPortalType ValueIndex;
    IdPortalType EdgeFar;
    IdPortalType EdgeNear;
    IdPortalType ArcArray;
    bool IsJoinGraph;

    VISKORES_EXEC
    bool operator()(const viskores::Id& i, const viskores::Id& j) const
    {
      // first compare the far end
      if (this->EdgeFar.Get(i) < this->EdgeFar.Get(j))
        return true ^ this->IsJoinGraph;
      if (this->EdgeFar.Get(j) < this->EdgeFar.Get(i))
        return false ^ this->IsJoinGraph;

      // the compare the values of the low end
      viskores::Id valueIndex1 = this->ValueIndex.Get(this->EdgeNear.Get(i));
      viskores::Id valueIndex2 = this->ValueIndex.Get(this->EdgeNear.Get(j));

      if (this->Values.Get(valueIndex1) < this->Values.Get(valueIndex2))
        return true ^ this->IsJoinGraph;
      if (this->Values.Get(valueIndex2) < this->Values.Get(valueIndex1))
        return false ^ this->IsJoinGraph;

      // finally compare the indices
      if (valueIndex1 < valueIndex2)
        return true ^ this->IsJoinGraph;
      if (valueIndex2 < valueIndex1)
        return false ^ this->IsJoinGraph;

      if (i < j)
        return false ^ this->IsJoinGraph;
      if (j < i)
        return true ^ this->IsJoinGraph;

      // fallback can happen when multiple paths end at same extremum
      return false; //true ^ graph.isJoinGraph;
    }
  };

  VISKORES_CONT ExecObject PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                               viskores::cont::Token& token) const
  {
    return ExecObject(this->Values.PrepareForInput(device, token),
                      this->ValueIndex.PrepareForInput(device, token),
                      this->EdgeFar.PrepareForInput(device, token),
                      this->EdgeNear.PrepareForInput(device, token),
                      this->ArcArray.PrepareForInput(device, token),
                      this->IsJoinGraph);
  }
}; // EdgePeakComparator
}
}
}

#endif
