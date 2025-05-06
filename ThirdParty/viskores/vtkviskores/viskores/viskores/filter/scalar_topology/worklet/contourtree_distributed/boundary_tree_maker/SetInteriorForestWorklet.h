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

#ifndef viskores_worklet_contourtree_distributed_bract_maker_set_interior_forest_worklet_h
#define viskores_worklet_contourtree_distributed_bract_maker_set_interior_forest_worklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace bract_maker
{

/// Worklet to transfer the dependent counts for hyperarcs
/// Part of the BoundaryRestrictedAugmentedContourTree.PropagateBoundaryCounts function
class SetInteriorForestWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn contourTreeSupernodes,               // input
                                FieldIn interiorForestIsNecessary,           // input
                                FieldIn boundaryTreeMakerTree2Superset,      // input
                                WholeArrayIn meshGlobalIdsFromMeshIndices,   // input
                                WholeArrayIn boundaryTreeMakerUpNeighbour,   // input
                                WholeArrayIn boundaryTreeMakerDownNeighbour, // input
                                FieldInOut interiorForestAbove,              // output
                                FieldInOut interiorForestBelow               // output
  );
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  SetInteriorForestWorklet() {}

  // Allow for different portal type for the meshGlobalIds as they may be a fancy
  // array handle rather than a portal direclty to a IdArrayType
  template <typename InFieldPortalType, typename GlobalIdFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& sortId,
                                const viskores::Id& isNecessary,
                                const viskores::Id& supersetId,
                                const GlobalIdFieldPortalType& meshGlobalIdsPortal,
                                const InFieldPortalType& upNeighbourPortal,
                                const InFieldPortalType& downNeighbourPortal,
                                viskores::Id& interiorForestAbove,
                                viskores::Id& interiorForestBelow) const
  {
    (void)
      sortId; // TODO: Remove if not needed. This was included in original code but seems unused. Avoid compiler warning.
    // per supernode
    // ignore supernodes that weren't marked necessary, since they will never be searched for
    // all nodes to be searched for are necessary, but not all necessary nodes will be searched for
    if (isNecessary)
    { // necessary supernode
      // first, convert it to a sort ID: Asignement of sortId from contourTreeSupernodes done on input
      // now find it in the superset: Assignment of supersetId from boundaryTreeMakerTree2Superset done on input

      // find the up neighbour and convert it to a global ID: note that we may have a leaf
      // in which case this may be NO_SUCH_ELEMENT. This will not be searched for, but for safety,
      // we will test for it explicitly
      viskores::Id upSupersetId = upNeighbourPortal.Get(supersetId);
      if (viskores::worklet::contourtree_augmented::NoSuchElement(upSupersetId))
      { // no up neighbour
        interiorForestAbove = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      } // no up neighbour
      else
      { // up neighbour exists
        // mask it to get a superset ID
        upSupersetId = viskores::worklet::contourtree_augmented::MaskedIndex(upSupersetId);
        // look up the mesh ID. NOTE: meshGlobalIdsPortal is already indexed by bractVertexSuperset
        // so we no longer need to do the bractVertexSupersetPortal.Get(upSupersetId); lookup here
        viskores::Id upMeshId = upSupersetId;
        // then store the global ID in the "above" array
        interiorForestAbove = meshGlobalIdsPortal.Get(upMeshId);
      } // up neighbour exists

      // do the same for the down neighbour
      viskores::Id downSupersetId = downNeighbourPortal.Get(supersetId);
      if (viskores::worklet::contourtree_augmented::NoSuchElement(downSupersetId))
      { // no down neighbour
        interiorForestBelow = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      } // no down neighbour
      else
      { // down neighbour exists
        // mask it to get a superset ID
        downSupersetId = viskores::worklet::contourtree_augmented::MaskedIndex(downSupersetId);
        // look up the mesh ID. NOTE: meshGlobalIdsPortal is already indexed by bractVertexSuperset
        // so we no longer need to do the bractVertexSupersetPortal.Get(downSupersetId); lookup here
        viskores::Id downMeshId = downSupersetId;
        // then store the global ID in the "above" array
        interiorForestBelow = meshGlobalIdsPortal.Get(downMeshId);
      } // up neighbour exists
    }   // necessary supernode

    // In serial this worklet implements the following operation
    /*
    for (indexType supernode = 0; supernode < contourTree->supernodes.size(); supernode++)
       { // per supernode
       // ignore supernodes that weren't marked necessary, since they will never be searched for
       // all nodes to be searched for are necessary, but not all necessary nodes will be searched for
       if (residue->isNecessary[supernode])
         { // necessary supernode
         // first, convert it to a sort ID
         indexType sortID = contourTree->supernodes[supernode];
         // now find it in the superset
         indexType supersetID = tree2Superset[supernode];

         // find the up neighbour and convert it to a global ID: note that we may have a leaf
         // in which case this may be NO_SUCH_ELEMENT. This will not be searched for, but for safety,
         // we will test for it explicitly
         indexType upSupersetID = upNeighbour[supersetID];
         if (noSuchElement(upSupersetID))
           { // no up neighbour
           residue->above[supernode] = NO_SUCH_ELEMENT;
           } // no up neighbour
         else
           { // up neighbour exists
           // mask it to get a superset ID
           upSupersetID = maskedIndex(upSupersetID);
           // look up the mesh ID
           indexType upMeshID = bractVertexSuperset[upSupersetID];
           // then store the global ID in the "above" array
           residue->above[supernode] = mesh->GetGlobalIDFromMeshIndex(upMeshID);
           } // up neighbour exists

         // do the same for the down neighbour
         indexType downSupersetID = downNeighbour[supersetID];
         if (noSuchElement(downSupersetID))
           { // no down neighbour
           residue->below[supernode] = NO_SUCH_ELEMENT;
           } // no down neighbour
         else
           { // down neighbour exists
           // mask it to get a superset ID
           downSupersetID = maskedIndex(downSupersetID);
           // look up the mesh ID
           indexType downMeshID = bractVertexSuperset[downSupersetID];
           // then store the global ID in the "above" array
           residue->below[supernode] = mesh->GetGlobalIDFromMeshIndex(downMeshID);
           } // up neighbour exists
         } // necessary supernode
       } // per supernode
    */
  }

}; // SetInteriorForestWorklet


} // namespace bract_maker
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
