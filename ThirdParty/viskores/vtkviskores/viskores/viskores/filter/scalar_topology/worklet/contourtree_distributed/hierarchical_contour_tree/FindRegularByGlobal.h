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
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_worklet_contourtree_distributed_find_regular_by_global_device_data_h
#define viskores_worklet_contourtree_distributed_find_regular_by_global_device_data_h

#include <viskores/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>


namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{


/// Device implementation of FindRegularByGlobal for the HierarchicalContourTree
class FindRegularByGlobalDeviceData
{
public:
  using IndicesPortalType =
    typename viskores::worklet::contourtree_augmented::IdArrayType::ReadPortalType;

  VISKORES_CONT
  FindRegularByGlobalDeviceData(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token,
    const viskores::worklet::contourtree_augmented::IdArrayType& regularNodeSortOrder,
    const viskores::worklet::contourtree_augmented::IdArrayType& regularNodeGlobalIds)
  {
    // Prepare the arrays for input and store the array portals
    // so that they can be used inside a workelt
    this->RegularNodeSortOrder = regularNodeSortOrder.PrepareForInput(device, token);
    this->RegularNodeGlobalIds = regularNodeGlobalIds.PrepareForInput(device, token);
  }

  /// Define also as an operator so that we can use it in ArrayHandleTransform directly
  VISKORES_EXEC
  viskores::Id operator()(viskores::Id globalId) const
  {
    return this->FindRegularByGlobal(globalId);
  }

  // TODO: This is just a binary search. Does Viskores have an implementation we can use to replace this with?
  /// routine to search the array of regular nodes for a particular global ID
  VISKORES_EXEC
  viskores::Id FindRegularByGlobal(viskores::Id globalId) const
  { // FindRegularByGlobal()
    // this is just a binary search, but the C++ STL doesn't seem to implement it . . .
    viskores::Id left = 0;
    viskores::Id right = this->RegularNodeSortOrder.GetNumberOfValues() - 1;

    // pull LHE into register & check whether target is in range
    viskores::Id leftId = this->RegularNodeGlobalIds.Get(this->RegularNodeSortOrder.Get(left));
    if (leftId > globalId)
    {
      return viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    }

    // exact match on the left
    if (leftId == globalId)
    {
      return this->RegularNodeSortOrder.Get(left);
    }

    // pull RHE into register & check whether target is in range
    viskores::Id rightId = this->RegularNodeGlobalIds.Get(this->RegularNodeSortOrder.Get(right));
    // RHE is less than target, so not present
    if (rightId < globalId)
    {
      return viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    }

    // exact match on the right
    if (rightId == globalId)
    {
      return this->RegularNodeSortOrder.Get(right);
    }

    // loop until the two meet
    while (left <= right)
    { // while loop

      // compute the midpoint
      viskores::Id mid = (left + right) / 2;
      viskores::Id midId = this->RegularNodeGlobalIds.Get(this->RegularNodeSortOrder.Get(mid));

      // compare with the value
      if (midId == globalId)
      { // exact match
        return this->RegularNodeSortOrder.Get(mid);
      } // exact match

      if (midId < globalId)
      { // mid is lower: global is in right half
        left = mid + 1;
        leftId = this->RegularNodeGlobalIds.Get(this->RegularNodeSortOrder.Get(left));
      } // mid is lower: global is in right half

      if (midId > globalId)
      { // mid is higher: global is in left half
        right = mid - 1;
        rightId = this->RegularNodeGlobalIds.Get(this->RegularNodeSortOrder.Get(right));
      } // mid is higher: global is in left half
    }   // while loop

    // if we fell through, we didn't find it
    return viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
  } // FindRegularByGlobal()

private:
  // Array portals needed by FindRegularByGlobal
  IndicesPortalType RegularNodeSortOrder;
  IndicesPortalType RegularNodeGlobalIds;
};


/// ExecutionObject to generate a device object to use FindRegularByGlobal for the HierarchicalContourTree
class FindRegularByGlobal : public viskores::cont::ExecutionObjectBase
{
public:
  /// constructor
  VISKORES_CONT
  FindRegularByGlobal(
    const viskores::worklet::contourtree_augmented::IdArrayType& regularNodeSortOrder,
    const viskores::worklet::contourtree_augmented::IdArrayType& regularNodeGlobalIds)
    : RegularNodeSortOrder(regularNodeSortOrder)
    , RegularNodeGlobalIds(regularNodeGlobalIds)
  {
  }

  VISKORES_CONT FindRegularByGlobalDeviceData
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return FindRegularByGlobalDeviceData(
      device, token, this->RegularNodeSortOrder, this->RegularNodeGlobalIds);
  }

private:
  // Array portals needed by FindRegularByGlobal
  viskores::worklet::contourtree_augmented::IdArrayType RegularNodeSortOrder;
  viskores::worklet::contourtree_augmented::IdArrayType RegularNodeGlobalIds;
};


} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
