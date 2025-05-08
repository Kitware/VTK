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

#ifndef viskores_worklet_contourtree_augmented_process_contourtree_inc_hypersweep_worklets_h
#define viskores_worklet_contourtree_augmented_process_contourtree_inc_hypersweep_worklets_h

#include <viskores/BinaryOperators.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

/**
 * Incorporates values of the parent of the current subtree in the subtree for the min and max hypersweeps
 */
namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{

class InitialiseArcsVolume : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _3;

  viskores::Id TotalVolume;

  VISKORES_EXEC_CONT InitialiseArcsVolume(viskores::Id _totalVolume)
    : TotalVolume(_totalVolume)
  {
  }

  template <typename IdWholeArrayInPortalType, typename EdgeWholeArrayInOutPortal>
  VISKORES_EXEC void operator()(const viskores::Id currentId,
                                const IdWholeArrayInPortalType& hypersweepSumValuesPortal,
                                const IdWholeArrayInPortalType& superarcIntrinsicWeightPortal,
                                const IdWholeArrayInPortalType& superarcsPortal,
                                const EdgeWholeArrayInOutPortal& arcsPortal) const
  {
    Id i = currentId;
    Id parent = MaskedIndex(superarcsPortal.Get(i));
    if (parent == 0)
    {
      // We expect the root to the last vertex in the supernodes array
      VISKORES_ASSERT(i != superarcsPortal.GetNumberOfValues() - 2);
      return;
    }

    EdgeDataVolume edge;
    edge.I = i;
    edge.J = parent;
    edge.UpEdge = IsAscending((superarcsPortal.Get(i)));
    edge.SubtreeVolume = (this->TotalVolume - hypersweepSumValuesPortal.Get(i)) +
      (superarcIntrinsicWeightPortal.Get(i) - 1);

    EdgeDataVolume oppositeEdge;
    oppositeEdge.I = parent;
    oppositeEdge.J = i;
    oppositeEdge.UpEdge = !edge.UpEdge;
    oppositeEdge.SubtreeVolume = hypersweepSumValuesPortal.Get(i);

    arcsPortal.Set(i * 2, edge);
    arcsPortal.Set(i * 2 + 1, oppositeEdge);
  }
};


class SetFirstVertexForSuperparent : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  VISKORES_EXEC_CONT SetFirstVertexForSuperparent() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id sortedNode,
    const IdWholeArrayInPortalType& nodesPortal,
    const IdWholeArrayInPortalType& superparentsPortal,
    const IdWholeArrayInOutPortalType& firstVertexForSuperparentPortal) const
  {
    viskores::Id sortID = nodesPortal.Get(sortedNode);
    viskores::Id superparent = superparentsPortal.Get(sortID);
    if (sortedNode == 0)
    {
      firstVertexForSuperparentPortal.Set(superparent, sortedNode);
    }
    else if (superparent != superparentsPortal.Get(nodesPortal.Get(sortedNode - 1)))
    {
      firstVertexForSuperparentPortal.Set(superparent, sortedNode);
    }
  }
};

class ComputeIntrinsicWeight : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _2;

  VISKORES_EXEC_CONT ComputeIntrinsicWeight() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id superarc,
    const IdWholeArrayInPortalType& arcsPortal,
    const IdWholeArrayInPortalType& superarcsPortal,
    const IdWholeArrayInPortalType& firstVertexForSuperparentPortal,
    const IdWholeArrayInOutPortalType& superarcIntrinsicWeightPortal) const
  {
    if (superarc == superarcsPortal.GetNumberOfValues() - 1)
    {
      superarcIntrinsicWeightPortal.Set(
        superarc, arcsPortal.GetNumberOfValues() - firstVertexForSuperparentPortal.Get(superarc));
    }
    else
    {
      superarcIntrinsicWeightPortal.Set(superarc,
                                        firstVertexForSuperparentPortal.Get(superarc + 1) -
                                          firstVertexForSuperparentPortal.Get(superarc));
    }
  }
};


class SetFirstSupernodePerIteration : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2);
  using InputDomain = _1;

  VISKORES_EXEC_CONT SetFirstSupernodePerIteration() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id supernode,
    const IdWholeArrayInPortalType& whenTransferredPortal,
    const IdWholeArrayInOutPortalType& firstSupernodePerIterationPortal) const
  {
    viskores::Id when = MaskedIndex(whenTransferredPortal.Get(supernode));
    if (supernode == 0)
    {
      firstSupernodePerIterationPortal.Set(when, supernode);
    }
    else if (when != MaskedIndex(whenTransferredPortal.Get(supernode - 1)))
    {
      firstSupernodePerIterationPortal.Set(when, supernode);
    }
  }
};




template <typename Operator>
class AddDependentWeightHypersweep : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn iterationHypernodes,
                                WholeArrayIn hypernodes,
                                WholeArrayIn hyperarcs,
                                WholeArrayIn howManyUsed,
                                AtomicArrayInOut minMaxIndex);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4, _5);
  using InputDomain = _1;

  Operator Op;

  // Default Constructor
  VISKORES_EXEC_CONT AddDependentWeightHypersweep(Operator _op)
    : Op(_op)
  {
  }

  template <typename IdWholeArrayHandleCountingIn,
            typename IdWholeArrayIn,
            typename IdWholeArrayInOut>
  VISKORES_EXEC void operator()(const viskores::Id hyperarcId,
                                const IdWholeArrayHandleCountingIn& iterationHypernodesPortal,
                                const IdWholeArrayIn& hypernodesPortal,
                                const IdWholeArrayIn& hyperarcsPortal,
                                const IdWholeArrayIn& howManyUsedPortal,
                                const IdWholeArrayInOut& minMaxIndexPortal) const
  {
    Id i = iterationHypernodesPortal.Get(hyperarcId);

    // If it's the last hyperacs (there's nothing to do it's just the root)
    if (i >= hypernodesPortal.GetNumberOfValues() - 1)
    {
      return;
    }

    //
    // The value of the prefix scan is now accumulated in the last supernode of the hyperarc. Transfer is to the target
    //
    viskores::Id lastSupernode =
      MaskedIndex(hypernodesPortal.Get(i + 1)) - howManyUsedPortal.Get(i);

    //
    // Transfer the accumulated value to the target supernode
    //
    Id vertex = lastSupernode - 1;
    Id parent = MaskedIndex(hyperarcsPortal.Get(i));

    Id vertexValue = minMaxIndexPortal.Get(vertex);
    //Id parentValue = minMaxIndexPortal.Get(parent);

    //Id writeValue = op(vertexValue, parentValue);

    auto cur = minMaxIndexPortal.Get(parent); // Load the current value at idx
    // Use a compare-exchange loop to ensure the operation gets applied atomically
    while (!minMaxIndexPortal.CompareExchange(parent, &cur, this->Op(cur, vertexValue)))
      ;

    //minMaxIndexPortal.Set(parent, writeValue);
  }
}; // ComputeMinMaxValues

class InitialiseArcs : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  viskores::Id GlobalMinSortedIndex, GlobalMaxSortedIndex, RootSupernodeId;

  VISKORES_EXEC_CONT InitialiseArcs(viskores::Id _globalMinSortedIndex,
                                    viskores::Id _globalMaxSortedIndex,
                                    viskores::Id _rootSupernodeId)
    : GlobalMinSortedIndex(_globalMinSortedIndex)
    , GlobalMaxSortedIndex(_globalMaxSortedIndex)
    , RootSupernodeId(_rootSupernodeId)
  {
  }

  template <typename IdWholeArrayInPortalType, typename EdgeWholeArrayInOutPortal>
  VISKORES_EXEC void operator()(const viskores::Id currentId,
                                const IdWholeArrayInPortalType& minParentsPortal,
                                const IdWholeArrayInPortalType& maxParentsPortal,
                                const IdWholeArrayInPortalType& minValuesPortal,
                                const IdWholeArrayInPortalType& maxValuesPortal,
                                const IdWholeArrayInPortalType& superarcsPortal,
                                const EdgeWholeArrayInOutPortal& arcsPortal) const
  {
    Id i = currentId;
    Id parent = MaskedIndex(superarcsPortal.Get(i));

    // The root does not correspond to an arc
    if (parent == 0)
      return;

    EdgeDataHeight edge;
    edge.I = i;
    edge.J = parent;
    edge.UpEdge = IsAscending((superarcsPortal.Get(i)));

    EdgeDataHeight oppositeEdge;
    oppositeEdge.I = parent;
    oppositeEdge.J = i;
    oppositeEdge.UpEdge = !edge.UpEdge;


    // Is it in the direction of the minRootedTree?
    if (MaskedIndex(minParentsPortal.Get(edge.J)) == edge.I)
    {
      edge.SubtreeMin = minValuesPortal.Get(edge.J);
      oppositeEdge.SubtreeMin = this->GlobalMinSortedIndex;
    }
    else
    {
      oppositeEdge.SubtreeMin = minValuesPortal.Get(oppositeEdge.J);
      edge.SubtreeMin = this->GlobalMinSortedIndex;
    }

    // Is it in the direction of the maxRootedTree?
    if (MaskedIndex(maxParentsPortal.Get(edge.J)) == edge.I)
    {
      edge.SubtreeMax = maxValuesPortal.Get(edge.J);
      oppositeEdge.SubtreeMax = this->GlobalMaxSortedIndex;
    }
    else
    {
      oppositeEdge.SubtreeMax = maxValuesPortal.Get(oppositeEdge.J);
      edge.SubtreeMax = this->GlobalMaxSortedIndex;
    }

    // We technically don't need this because the root is supposed to be the last vertex
    if (i > this->RootSupernodeId)
    {
      i--;
    }

    arcsPortal.Set(i * 2, edge);
    arcsPortal.Set(i * 2 + 1, oppositeEdge);
  }
};


class ComputeSubtreeHeight : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _4;

  VISKORES_EXEC_CONT ComputeSubtreeHeight() {}

  template <typename Float64WholeArrayInPortalType,
            typename IdWholeArrayInPortalType,
            typename EdgeWholeArrayInOutPortal>
  VISKORES_EXEC void operator()(const viskores::Id currentId,
                                const Float64WholeArrayInPortalType& fieldValuesPortal,
                                const IdWholeArrayInPortalType& ctSortOrderPortal,
                                const IdWholeArrayInPortalType& supernodesPortal,
                                const EdgeWholeArrayInOutPortal& arcsPortal) const
  {
    Id i = currentId;
    EdgeDataHeight edge = arcsPortal.Get(i);

    Float64 minIsoval = fieldValuesPortal.Get(ctSortOrderPortal.Get(edge.SubtreeMin));
    Float64 maxIsoval = fieldValuesPortal.Get(ctSortOrderPortal.Get(edge.SubtreeMax));
    Float64 vertexIsoval =
      fieldValuesPortal.Get(ctSortOrderPortal.Get(supernodesPortal.Get(edge.I)));

    // We need to incorporate the value of the vertex into the height of the tree (otherwise leafs edges have 0 persistence)
    minIsoval = viskores::Minimum()(minIsoval, vertexIsoval);
    maxIsoval = viskores::Maximum()(maxIsoval, vertexIsoval);

    edge.SubtreeHeight = maxIsoval - minIsoval;

    arcsPortal.Set(i, edge);
  }
}; // ComputeMinMaxValues




class SetBestUpDown : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayInOut, WholeArrayInOut, WholeArrayIn);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _3;

  VISKORES_EXEC_CONT SetBestUpDown() {}

  template <typename IdWholeArrayInPortalType, typename EdgeWholeArrayInOutPortal>
  VISKORES_EXEC void operator()(const viskores::Id currentId,
                                const IdWholeArrayInPortalType& bestUpwardPortal,
                                const IdWholeArrayInPortalType& bestDownwardPortal,
                                const EdgeWholeArrayInOutPortal& arcsPortal) const
  {
    viskores::Id i = currentId;

    if (i == 0)
    {
      if (arcsPortal.Get(0).UpEdge == 0)
      {
        bestDownwardPortal.Set(arcsPortal.Get(0).I, arcsPortal.Get(0).J);
      }
      else
      {
        bestUpwardPortal.Set(arcsPortal.Get(0).I, arcsPortal.Get(0).J);
      }
    }
    else
    {
      if (arcsPortal.Get(i).UpEdge == 0 && arcsPortal.Get(i).I != arcsPortal.Get(i - 1).I)
      {
        bestDownwardPortal.Set(arcsPortal.Get(i).I, arcsPortal.Get(i).J);
      }

      if (arcsPortal.Get(i).UpEdge == 1 &&
          (arcsPortal.Get(i).I != arcsPortal.Get(i - 1).I || arcsPortal.Get(i - 1).UpEdge == 0))
      {
        bestUpwardPortal.Set(arcsPortal.Get(i).I, arcsPortal.Get(i).J);
      }
    }
  }
}; // ComputeMinMaxValues


class UnmaskArray : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1);
  using InputDomain = _1;


  // Default Constructor
  VISKORES_EXEC_CONT UnmaskArray() {}

  template <typename IdWholeArrayInPortalType>
  VISKORES_EXEC void operator()(const viskores::Id currentId,
                                const IdWholeArrayInPortalType& maskedArrayPortal) const
  {
    const auto currentValue = maskedArrayPortal.Get(currentId);
    maskedArrayPortal.Set(currentId, MaskedIndex(currentValue));
  }
}; // ComputeMinMaxValues

class PropagateBestUpDown : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayOut);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _3;


  // Default Constructor
  VISKORES_EXEC_CONT PropagateBestUpDown() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayOutPortalType>
  VISKORES_EXEC void operator()(const viskores::Id supernodeId,
                                const IdWholeArrayInPortalType& bestUpwardPortal,
                                const IdWholeArrayInPortalType& bestDownwardPortal,
                                const IdWholeArrayOutPortalType& whichBranchPortal) const
  {
    viskores::Id bestUp = bestUpwardPortal.Get(supernodeId);
    if (NoSuchElement(bestUp))
    {
      // flag it as an upper leaf
      whichBranchPortal.Set(supernodeId, TERMINAL_ELEMENT | supernodeId);
    }
    else if (bestDownwardPortal.Get(bestUp) == supernodeId)
      whichBranchPortal.Set(supernodeId, bestUp);
    else
      whichBranchPortal.Set(supernodeId, TERMINAL_ELEMENT | supernodeId);
  }
}; // ComputeMinMaxValues

class WhichBranchNewId : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2);
  using InputDomain = _2;


  // Default Constructor
  VISKORES_EXEC_CONT WhichBranchNewId() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VISKORES_EXEC void operator()(const viskores::Id supernode,
                                const IdWholeArrayInPortalType& chainToBranchPortal,
                                const IdWholeArrayInOutPortalType& whichBranchPortal) const
  {
    const auto currentValue = MaskedIndex(whichBranchPortal.Get(supernode));
    whichBranchPortal.Set(supernode, chainToBranchPortal.Get(currentValue));
  }
}; // ComputeMinMaxValues

class BranchMinMaxSet : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayInOut, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _2;

  viskores::Id NumSupernodes;

  // Default Constructor
  VISKORES_EXEC_CONT BranchMinMaxSet(viskores::Id _NumSupernodes)
    : NumSupernodes(_NumSupernodes)
  {
  }

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VISKORES_EXEC void operator()(const viskores::Id supernode,
                                const IdWholeArrayInPortalType& supernodeSorterPortal,
                                const IdWholeArrayInPortalType& whichBranchPortal,
                                const IdWholeArrayInOutPortalType& branchMinimumPortal,
                                const IdWholeArrayInOutPortalType& branchMaximumPortal) const
  {
    // retrieve supernode & branch IDs
    viskores::Id supernodeID = supernodeSorterPortal.Get(supernode);
    viskores::Id branchID = whichBranchPortal.Get(supernodeID);
    // save the branch ID as the owner
    // use LHE of segment to set branch minimum
    if (supernode == 0)
    { // sn = 0
      branchMinimumPortal.Set(branchID, supernodeID);
    } // sn = 0
    else if (branchID != whichBranchPortal.Get(supernodeSorterPortal.Get(supernode - 1)))
    { // LHE
      branchMinimumPortal.Set(branchID, supernodeID);
    } // LHE
    // use RHE of segment to set branch maximum
    if (supernode == this->NumSupernodes - 1)
    { // sn = max
      branchMaximumPortal.Set(branchID, supernodeID);
    } // sn = max
    else if (branchID != whichBranchPortal.Get(supernodeSorterPortal.Get(supernode + 1)))
    { // RHE
      branchMaximumPortal.Set(branchID, supernodeID);
    } // RHE
  }
}; // ComputeMinMaxValues

class BranchSaddleParentSet : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayInOut,
                                WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _2;

  // Default Constructor
  VISKORES_EXEC_CONT BranchSaddleParentSet() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VISKORES_EXEC void operator()(const viskores::Id branchID,
                                const IdWholeArrayInPortalType& whichBranchPortal,
                                const IdWholeArrayInPortalType& branchMinimumPortal,
                                const IdWholeArrayInPortalType& branchMaximumPortal,
                                const IdWholeArrayInPortalType& bestDownwardPortal,
                                const IdWholeArrayInPortalType& bestUpwardPortal,
                                const IdWholeArrayInOutPortalType& branchSaddlePortal,
                                const IdWholeArrayInOutPortalType& branchParentPortal) const
  {
    viskores::Id branchMax = branchMaximumPortal.Get(branchID);
    // check whether the maximum is NOT a leaf
    if (!NoSuchElement(bestUpwardPortal.Get(branchMax)))
    { // points to a saddle
      branchSaddlePortal.Set(branchID, MaskedIndex(bestUpwardPortal.Get(branchMax)));
      // if not, then the bestUp points to a saddle vertex at which we join the parent
      branchParentPortal.Set(branchID, whichBranchPortal.Get(bestUpwardPortal.Get(branchMax)));
    } // points to a saddle
    // now do the same with the branch minimum
    viskores::Id branchMin = branchMinimumPortal.Get(branchID);
    // test whether NOT a lower leaf
    if (!NoSuchElement(bestDownwardPortal.Get(branchMin)))
    { // points to a saddle
      branchSaddlePortal.Set(branchID, MaskedIndex(bestDownwardPortal.Get(branchMin)));
      // if not, then the bestUp points to a saddle vertex at which we join the parent
      branchParentPortal.Set(branchID, whichBranchPortal.Get(bestDownwardPortal.Get(branchMin)));
    } // points to a saddle
  }
}; // ComputeMinMaxValues



class PrepareChainToBranch : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2);
  using InputDomain = _1;


  // Default Constructor
  VISKORES_EXEC_CONT PrepareChainToBranch() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VISKORES_EXEC void operator()(const viskores::Id supernode,
                                const IdWholeArrayInPortalType& whichBranchPortal,
                                const IdWholeArrayInOutPortalType& chainToBranchPortal) const
  {
    // test whether the supernode points to itself to find the top ends
    if (MaskedIndex(whichBranchPortal.Get(supernode)) == supernode)
    {
      chainToBranchPortal.Set(supernode, 1);
    }
  }
}; // ComputeMinMaxValues


class FinaliseChainToBranch : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2);
  using InputDomain = _1;

  VISKORES_EXEC_CONT FinaliseChainToBranch() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VISKORES_EXEC void operator()(const viskores::Id supernode,
                                const IdWholeArrayInPortalType& whichBranchPortal,
                                const IdWholeArrayInOutPortalType& chainToBranchPortal) const
  {
    // test whether the supernode points to itself to find the top ends
    if (MaskedIndex(whichBranchPortal.Get(supernode)) == supernode)
    {
      const auto value = chainToBranchPortal.Get(supernode);
      chainToBranchPortal.Set(supernode, value - 1);
    }
    else
    {
      chainToBranchPortal.Set(supernode, (viskores::Id)NO_SUCH_ELEMENT);
    }
  }
}; // ComputeMinMaxValues


template <typename Operator>
class IncorporateParent : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  Operator Op;

  // Default Constructor
  VISKORES_EXEC_CONT IncorporateParent(Operator _op)
    : Op(_op)
  {
  }

  template <typename IdWholeArrayIn, typename IdWholeArrayInOut>
  VISKORES_EXEC void operator()(const viskores::Id superarcId,
                                const IdWholeArrayIn& parentsPortal,
                                const IdWholeArrayIn& supernodesPortal,
                                const IdWholeArrayInOut& hypersweepValuesPortal) const
  {
    Id i = superarcId;

    Id parent = MaskedIndex(parentsPortal.Get(i));

    Id subtreeValue = hypersweepValuesPortal.Get(i);
    Id parentValue = MaskedIndex(supernodesPortal.Get(parent));

    hypersweepValuesPortal.Set(i, this->Op(subtreeValue, parentValue));
  }
}; // ComputeMinMaxValues


} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores


#endif
