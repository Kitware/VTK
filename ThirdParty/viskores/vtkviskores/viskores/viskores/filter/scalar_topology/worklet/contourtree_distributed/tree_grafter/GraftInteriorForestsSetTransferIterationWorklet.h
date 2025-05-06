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

#ifndef viskores_worklet_contourtree_distributed_tree_grafter_graft_interior_forests_set_trandfer_iteration_worklet_h
#define viskores_worklet_contourtree_distributed_tree_grafter_graft_interior_forests_set_trandfer_iteration_worklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace tree_grafter
{

/// Now set the transfer iteration for all attachment points
/// If there were no supernodes to transfer, their types are all NO_SUCH_ELEMENT
class GraftInteriorForestsSetTransferIterationWorklet : public viskores::worklet::WorkletMapField
{
public:
  // NOTE: supernodeType is sized to ContourTree.Supernodes.GetNumberOfValues() so we can use it for our iteration
  // NOTE: for whenTransferred we neeed need FieldInOut type to avoid overwrite of existing value as not all values will be updated
  using ControlSignature = void(FieldIn supernodeType,       // input
                                FieldIn hierarchicalSuperId, // input
                                FieldInOut whenTransferred   // output
  );

  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  GraftInteriorForestsSetTransferIterationWorklet(const viskores::Id& numTransferIterations)
    : NumTransferIterations(numTransferIterations)
  {
  }

  VISKORES_EXEC void operator()(const viskores::Id& supernodeType,
                                const viskores::Id& hierarchicalSuperId,
                                viskores::Id& whenTransferred) const
  { // operator ()
    if ((supernodeType == viskores::worklet::contourtree_augmented::IS_ATTACHMENT) &&
        viskores::worklet::contourtree_augmented::NoSuchElement(hierarchicalSuperId))
    { // not a supernode in the hierarchical tree yet
      whenTransferred =
        (this->NumTransferIterations | viskores::worklet::contourtree_augmented::IS_SUPERNODE);
    } // not a supernode in the hierarchical tree yet

    // In serial this worklet implements the following operation
    /*
    //  Now set the transfer iteration for all attachment points
    //  If there were no supernodes to transfer, their types are all NO_SUCH_ELEMENT
    #pragma omp parallel for
    for (indexType supernode = 0; supernode < contourTree->supernodes.size(); supernode++)
      { // per supernode
      // std::cout << "Supernode " << supernode << std::endl;
      if ((supernodeType[supernode] == IS_ATTACHMENT) && noSuchElement(hierarchicalSuperID[supernode]))
        { // not a supernode in the hierarchical tree yet
        whenTransferred[supernode] = nTransferIterations | IS_SUPERNODE;
        } // not a supernode in the hierarchical tree yet
      } // per supernode
    */
  } // operator ()

private:
  viskores::Id NumTransferIterations;
}; // BoundaryVerticiesPerSuperArcStepOneWorklet

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
