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
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_worklet_contourtree_distributed_hierarchical_augmenter_resize_arrays_build_new_supernode_ids_worklet_h
#define viskores_worklet_contourtree_distributed_hierarchical_augmenter_resize_arrays_build_new_supernode_ids_worklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_augmenter
{

/// Worklet used in HierarchicalAugmenter<FieldType>ResizeArrays to build the newSupernodeIds array
class ResizeArraysBuildNewSupernodeIdsWorklet : public viskores::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  using ControlSignature = void(
    FieldIn supernodeIndex, // input domain ArrayHandleIndex(SupernodeSorter.GetNumberOfValues())
    // FieldIn supernodeIdSetPermuted, // input supernodeIDSet permuted by supernodeSorter to allow for FieldIn
    FieldIn
      globalRegularIdSet, // input globalRegularIdSet permuted by supernodeSorter to allow for FieldIn
    ExecObject findRegularByGlobal,
    WholeArrayIn baseTreeRegular2Supernode,
    WholeArrayInOut
      newSupernodeIds // output/input (both are necessary since not all valyes will be overwritten)
  );

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  ResizeArraysBuildNewSupernodeIdsWorklet(const viskores::Id& numSupernodesAlready)
    : NumSupernodesAlready(numSupernodesAlready)
  {
  }

  template <typename InOutFieldPortalType, typename InFieldPortalType, typename ExecObjectType>
  VISKORES_EXEC void operator()(
    const viskores::Id& supernode, // InputIndex of supernodeSorter
    // const viskores::Id& oldSupernodeId, // same as supernodeIDSet[supernodeSetIndex];
    const viskores::Id& globalRegularIdSetValue, // same as globalRegularIDSet[supernodeSetIndex]
    const ExecObjectType& findRegularByGlobal,
    const InFieldPortalType& baseTreeRegular2SupernodePortal,
    const InOutFieldPortalType& newSupernodeIdsPortal) const
  {
    // per supernode
    // retrieve the index from the sorting index array. supernodeSetIndex set on input

    // work out the correct new supernode ID
    viskores::Id newSupernodeId = this->NumSupernodesAlready + supernode;

    // retrieve the old supernode ID from the sorting array, remembering
    // that if it came from another block it will be set to NO_SUCH_ELEMENT
    // viskores::Id oldSupernodeId set on input since we use ArrayHandlePermutation to
    // shuffle supernodeIDSet by supernodeSorter;
    // TODO/WARNING:  Logic error in that comment for presimplified trees, but not for the original version.  See RetrieveOldSupernodes() for why.

    // TODO/WARNING:  We substitute a search in the old hierarchical tree for the supernode.  If it is present, then we fill in it's entry in the

    // newSupernodeIDs array. If not, we're happy.
    viskores::Id oldRegularId = findRegularByGlobal.FindRegularByGlobal(globalRegularIdSetValue);
    viskores::Id oldSupernodeId = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    if (!viskores::worklet::contourtree_augmented::NoSuchElement(oldRegularId))
    {
      oldSupernodeId = baseTreeRegular2SupernodePortal.Get(oldRegularId);
    }

    // and write to the lookup array
    if (!viskores::worklet::contourtree_augmented::NoSuchElement(oldSupernodeId))
    {
      newSupernodeIdsPortal.Set(oldSupernodeId, newSupernodeId);
    }

    // In serial this worklet implements the following operation
    /*
     for (viskores::Id supernode = 0; supernode < supernodeSorter.size(); supernode++)
     { // per supernode
       // retrieve the index from the sorting index array
       viskores::Id supernodeSetIndex = supernodeSorter[supernode];

       // work out the correct new supernode ID
       viskores::Id newSupernodeID = numSupernodesAlready + supernode;

       // retrieve the old supernode ID from the sorting array, remembering that if it came from another block it will be set to NO_SUCH_ELEMENT
       viskores::Id oldSupernodeID = supernodeIDSet[supernodeSetIndex];

       // and write to the lookup array
       if (!noSuchElement(oldSupernodeID))
         newSupernodeIDs[oldSupernodeID] = newSupernodeID;
     } // per supernode
    */
  } // operator()()
private:
  const viskores::Id NumSupernodesAlready;

}; // ResizeArraysBuildNewSupernodeIdsWorklet

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
