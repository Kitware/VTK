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

#ifndef viskores_worklet_contourtree_distributed_tree_grafter_identify_leaf_hyperarcs_worklet_h
#define viskores_worklet_contourtree_distributed_tree_grafter_identify_leaf_hyperarcs_worklet_h


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

/// \brief Worklet implementing the TreeGrafter.IdentifyLeafHyperarcs function
///
///  At this stage, we have:
///  i.    hierarchicalRegularID set for any supernode stored at all in the parent
///   ii.   hierarchicalSuperID set for any supernode that is a supernode in the parent
///  iii.  hierarchicalHyperParent set for any attachment point
///  iv.    supernodeType set to indicate what type of supernode
///  v.    up/dn neighbours set for all supernodes
///
/// at the end of the chain collapse, the up/down neighbours define the start & end of the hyperarc
/// one end may be a leaf, in which case we can transfer the hyperarc
/// note that because we are grafting, we have a guarantee that they can't both be leaves
/// we therefore:
/// a. for leaves, determine whether up or down hyperarc, create hyperarc
/// b. for regular vertices pointing to a leaf hyperarc, set superarc / hyperparent
/// c. for other vertices, ignore
class IdentifyLeafHyperarcsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn
      activeSuperarcs, // input iteration index. loop to one less than ContourTree->Supernodes.GetNumberOfValues()
    WholeArrayIn supernodeType,               // input
    WholeArrayIn upNeighbour,                 // input
    WholeArrayIn downNeighbour,               // input
    WholeArrayOut hierarchicalHyperparent,    //output
    WholeArrayOut hierarchicalHyperarcPortal, // output
    WholeArrayOut whenTransferredPortal       // output
  );

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  IdentifyLeafHyperarcsWorklet(const viskores::Id& numTransferIterations)
    : NumTransferIterations(numTransferIterations)
  {
  }

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::worklet::contourtree_augmented::EdgePair& activeSuperarc,
    const InFieldPortalType supernodeTypePortal,
    const InFieldPortalType upNeighbourPortal,
    const InFieldPortalType downNeighbourPortal,
    const OutFieldPortalType& hierarchicalHyperparentPortal,
    const OutFieldPortalType& hierarchicalHyperarcPortal,
    const OutFieldPortalType& whenTransferredPortal) const
  { // operator ()
    // per active superarc
    // retrieve the supernode IDs for the two ends
    viskores::Id low = activeSuperarc.first;
    viskores::Id high = activeSuperarc.second;

    // test whether the top end is an upper leaf
    switch (supernodeTypePortal.Get(high))
    { // switch on upper end
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_UPPER_LEAF:
      { // upper end is a leaf
        // in lower leaf rounds, never recognise these
        hierarchicalHyperparentPortal.Set(high, high);
        hierarchicalHyperarcPortal.Set(
          high,
          viskores::worklet::contourtree_augmented::MaskedIndex(downNeighbourPortal.Get(high)));
        whenTransferredPortal.Set(high,
                                  this->NumTransferIterations |
                                    viskores::worklet::contourtree_augmented::IS_HYPERNODE);
        break;
      } // upper end is a leaf
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_REGULAR:
      { // upper end is regular
        // notice that this is redundant, so will be set from both arcs
        // this is parallel safe, because it sets the same value anyway
        // testing would be more complex
        // find the up & down neighbours
        viskores::Id upNbr =
          viskores::worklet::contourtree_augmented::MaskedIndex(upNeighbourPortal.Get(high));
        viskores::Id downNbr =
          viskores::worklet::contourtree_augmented::MaskedIndex(downNeighbourPortal.Get(high));

        // test the up neighbour first for leaf-hood
        // but only if the corresponding flag is true
        if (supernodeTypePortal.Get(upNbr) ==
            (viskores::Id)viskores::worklet::contourtree_augmented::IS_UPPER_LEAF)
        { // up neighbour is an upper leaf
          hierarchicalHyperparentPortal.Set(high, upNbr);
          whenTransferredPortal.Set(
            high,
            this->NumTransferIterations |
              (viskores::Id)viskores::worklet::contourtree_augmented::IS_SUPERNODE);
        } // up neighbour is an upper leaf
        // then the down neighbour (cannot both be true)
        else if (supernodeTypePortal.Get(downNbr) ==
                 (viskores::Id)viskores::worklet::contourtree_augmented::IS_LOWER_LEAF)
        { // down neighbour is a lower leaf
          hierarchicalHyperparentPortal.Set(high, downNbr);
          whenTransferredPortal.Set(
            high,
            this->NumTransferIterations |
              (viskores::Id)viskores::worklet::contourtree_augmented::IS_SUPERNODE);
        } // down neighbour is a lower leaf
        break;
      } // case: upper end is regular
      // all other cases do nothing
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_SADDLE:
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_ATTACHMENT:
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_LOWER_LEAF:
      default:
        break;
    } // switch on upper end

    // test whether the bottom end is a lower leaf
    switch (supernodeTypePortal.Get(low))
    { // switch on lower end
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_LOWER_LEAF:
      { // lower end is a leaf
        hierarchicalHyperparentPortal.Set(low, low);
        hierarchicalHyperarcPortal.Set(
          low,
          viskores::worklet::contourtree_augmented::MaskedIndex(upNeighbourPortal.Get(low)) |
            (viskores::Id)viskores::worklet::contourtree_augmented::IS_ASCENDING);
        whenTransferredPortal.Set(
          low,
          this->NumTransferIterations |
            (viskores::Id)viskores::worklet::contourtree_augmented::IS_HYPERNODE);
        break;
      } // lower end is a leaf
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_REGULAR:
      { // lower end is regular
        // notice that this is redundant, so will be set from both arcs
        // this is parallel safe, because it sets the same value anyway
        // testing would be more complex
        // find the up & down neighbours
        viskores::Id upNbr =
          viskores::worklet::contourtree_augmented::MaskedIndex(upNeighbourPortal.Get(low));
        viskores::Id downNbr =
          viskores::worklet::contourtree_augmented::MaskedIndex(downNeighbourPortal.Get(low));

        // test the up neighbour first for leaf-hood
        if (supernodeTypePortal.Get(upNbr) ==
            (viskores::Id)viskores::worklet::contourtree_augmented::IS_UPPER_LEAF)
        { // up neighbour is an upper leaf
          hierarchicalHyperparentPortal.Set(low, upNbr);
          whenTransferredPortal.Set(
            low,
            this->NumTransferIterations |
              (viskores::Id)viskores::worklet::contourtree_augmented::IS_SUPERNODE);
        } // up neighbour is an upper leaf
        // then the down neighbour (cannot both be true)
        else if (supernodeTypePortal.Get(downNbr) ==
                 (viskores::Id)viskores::worklet::contourtree_augmented::IS_LOWER_LEAF)
        { // down neighbour is a lower leaf
          hierarchicalHyperparentPortal.Set(low, downNbr);
          whenTransferredPortal.Set(
            low,
            this->NumTransferIterations |
              (viskores::Id)viskores::worklet::contourtree_augmented::IS_SUPERNODE);
        } // down neighbour is a lower leaf
        break;
      } // lower end is regular
      // all other cases do nothing
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_SADDLE:
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_ATTACHMENT:
      case (viskores::Id)viskores::worklet::contourtree_augmented::IS_UPPER_LEAF:
      default:
        break;
    } // switch on lower end

    // In serial this worklet implements the following operation
    /*
    #pragma omp parallel for
    for (indexType activeSuper = 0; activeSuper < activeSuperarcs.size(); activeSuper++)
      { // per active superarc
      // retrieve the supernode IDs for the two ends
      indexType low = activeSuperarcs[activeSuper].low;
      indexType high = activeSuperarcs[activeSuper].high;

      // test whether the top end is an upper leaf
      switch (supernodeType[high])
        { // switch on upper end
         case IS_UPPER_LEAF:
          { // upper end is a leaf
          // in lower leaf rounds, never recognise these
          hierarchicalHyperparent[high] = high;
          hierarchicalHyperarc[high] = maskedIndex(downNeighbour[high]);
          whenTransferred[high] = nTransferIterations | IS_HYPERNODE;
          break;
          } // upper end is a leaf
        case IS_REGULAR:
          { // upper end is regular
          // notice that this is redundant, so will be set from both arcs
          // this is parallel safe, because it sets the same value anyway
          // testing would be more complex
          // find the up & down neighbours
          indexType upNbr = maskedIndex(upNeighbour[high]);
          indexType downNbr = maskedIndex(downNeighbour[high]);

          // test the up neighbour first for leaf-hood
          // but only if the corresponding flag is true
          if (supernodeType[upNbr] == IS_UPPER_LEAF)
            { // up neighbour is an upper leaf
            hierarchicalHyperparent[high] = upNbr;
            whenTransferred[high] = nTransferIterations | IS_SUPERNODE;
            } // up neighbour is an upper leaf
          // then the down neighbour (cannot both be true)
          else if (supernodeType[downNbr] == IS_LOWER_LEAF)
            { // down neighbour is a lower leaf
            hierarchicalHyperparent[high] = downNbr;
            whenTransferred[high] = nTransferIterations | IS_SUPERNODE;
            } // down neighbour is a lower leaf
          break;
          } // upper end is regular
        // all other cases do nothing
        case IS_SADDLE:
        case IS_ATTACHMENT:
        case IS_LOWER_LEAF:
        default:
          break;
        } // switch on upper end

      // test whether the bottom end is a lower leaf
      switch (supernodeType[low])
        { // switch on lower end
         case IS_LOWER_LEAF:
          { // lower end is a leaf
          hierarchicalHyperparent[low] = low;
          hierarchicalHyperarc[low] = maskedIndex(upNeighbour[low]) | IS_ASCENDING;
          whenTransferred[low] = nTransferIterations | IS_HYPERNODE;
          break;
          } // lower end is a leaf
        case IS_REGULAR:
          { // lower end is regular
          // notice that this is redundant, so will be set from both arcs
          // this is parallel safe, because it sets the same value anyway
          // testing would be more complex
          // find the up & down neighbours
          indexType upNbr = maskedIndex(upNeighbour[low]);
          indexType downNbr = maskedIndex(downNeighbour[low]);

          // test the up neighbour first for leaf-hood
          if (supernodeType[upNbr] == IS_UPPER_LEAF)
            { // up neighbour is an upper leaf
            hierarchicalHyperparent[low] = upNbr;
            whenTransferred[low] = nTransferIterations | IS_SUPERNODE;
            } // up neighbour is an upper leaf
          // then the down neighbour (cannot both be true)
          else if (supernodeType[downNbr] == IS_LOWER_LEAF)
            { // down neighbour is a lower leaf
            hierarchicalHyperparent[low] = downNbr;
            whenTransferred[low] = nTransferIterations | IS_SUPERNODE;
            } // down neighbour is a lower leaf
          break;
          } // lower end is regular
        // all other cases do nothing
        case IS_SADDLE:
        case IS_ATTACHMENT:
        case IS_UPPER_LEAF:
        default:
          break;
        } // switch on lower end
      } // per active superarc
    */
  } // operator ()

private:
  viskores::Id NumTransferIterations;

}; // IdentifyLeafHyperarcsWorklet

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
