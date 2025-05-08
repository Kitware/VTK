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


#ifndef viskores_worklet_contourtree_augmented_process_contourtree_h
#define viskores_worklet_contourtree_augmented_process_contourtree_h

// global includes
#include <algorithm>
#include <iomanip>
#include <iostream>

// local includes
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

//VISKORES includes
#include <viskores/Pair.h>
#include <viskores/Types.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/Timer.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/processcontourtree/Branch.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/processcontourtree/SuperArcVolumetricComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/processcontourtree/SuperNodeBranchComparator.h>

#include <viskores/cont/Invoker.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/processcontourtree/HypersweepWorklets.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/processcontourtree/PointerDoubling.h>

//#define DEBUG_PRINT


namespace process_contourtree_inc_ns =
  viskores::worklet::contourtree_augmented::process_contourtree_inc;

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{


// TODO Many of the post processing routines still need to be parallelized
// Class with routines for post processing the contour tree
class ProcessContourTree
{ // class ProcessContourTree
public:
  // initialises contour tree arrays - rest is done by another class
  ProcessContourTree()
  { // ProcessContourTree()
  } // ProcessContourTree()

  // collect the sorted arcs
  void static CollectSortedArcs(const ContourTree& contourTree,
                                const IdArrayType& sortOrder,
                                EdgePairArray& sortedArcs)
  { // CollectSortedArcs
    // create an array for sorting the arcs
    std::vector<EdgePair> arcSorter;

    // fill it up
    auto arcsPortal = contourTree.Arcs.ReadPortal();
    auto sortOrderPortal = sortOrder.ReadPortal();

    for (viskores::Id node = 0; node < contourTree.Arcs.GetNumberOfValues(); node++)
    { // per node
      // retrieve ID of target supernode
      viskores::Id arcTo = arcsPortal.Get(node);

      // if this is true, it is the last pruned vertex & is omitted
      if (NoSuchElement(arcTo))
        continue;

      // otherwise, strip out the flags
      arcTo = MaskedIndex(arcTo);

      // now convert to mesh IDs from sort IDs
      // otherwise, we need to convert the IDs to regular mesh IDs
      viskores::Id regularID = sortOrderPortal.Get(node);

      // retrieve the regular ID for it
      viskores::Id regularTo = sortOrderPortal.Get(arcTo);

      // how we print depends on which end has lower ID
      if (regularID < regularTo)
        arcSorter.push_back(EdgePair(regularID, regularTo));
      else
        arcSorter.push_back(EdgePair(regularTo, regularID));
    } // per vertex

    // now sort it
    // Setting saddlePeak reference to the make_ArrayHandle directly does not work
    sortedArcs = viskores::cont::make_ArrayHandle(arcSorter, viskores::CopyFlag::On);
    viskores::cont::Algorithm::Sort(sortedArcs, SaddlePeakSort());
  } // CollectSortedArcs

  // collect the sorted superarcs
  void static CollectSortedSuperarcs(const ContourTree& contourTree,
                                     const IdArrayType& sortOrder,
                                     EdgePairArray& saddlePeak)
  { // CollectSortedSuperarcs()
    // create an array for sorting the arcs
    std::vector<EdgePair> superarcSorter;

    // fill it up
    auto supernodesPortal = contourTree.Supernodes.ReadPortal();
    auto superarcsPortal = contourTree.Superarcs.ReadPortal();
    auto sortOrderPortal = sortOrder.ReadPortal();

    for (viskores::Id supernode = 0; supernode < contourTree.Supernodes.GetNumberOfValues();
         supernode++)
    { // per supernode
      // sort ID of the supernode
      viskores::Id sortID = supernodesPortal.Get(supernode);

      // retrieve ID of target supernode
      viskores::Id superTo = superarcsPortal.Get(supernode);

      // if this is true, it is the last pruned vertex & is omitted
      if (NoSuchElement(superTo))
        continue;

      // otherwise, strip out the flags
      superTo = MaskedIndex(superTo);

      // otherwise, we need to convert the IDs to regular mesh IDs
      viskores::Id regularID = sortOrderPortal.Get(MaskedIndex(sortID));

      // retrieve the regular ID for it
      viskores::Id regularTo = sortOrderPortal.Get(MaskedIndex(supernodesPortal.Get(superTo)));

      // how we print depends on which end has lower ID
      if (regularID < regularTo)
      { // from is lower
        // extra test to catch duplicate edge
        if (superarcsPortal.Get(superTo) != supernode)
        {
          superarcSorter.push_back(EdgePair(regularID, regularTo));
        }
      } // from is lower
      else
      {
        superarcSorter.push_back(EdgePair(regularTo, regularID));
      }
    } // per vertex

    // Setting saddlePeak reference to the make_ArrayHandle directly does not work
    saddlePeak = viskores::cont::make_ArrayHandle(superarcSorter, viskores::CopyFlag::On);

    // now sort it
    viskores::cont::Algorithm::Sort(saddlePeak, SaddlePeakSort());
  } // CollectSortedSuperarcs()

  // routine to compute the volume for each hyperarc and superarc
  void static ComputeVolumeWeightsSerial(const ContourTree& contourTree,
                                         const viskores::Id nIterations,
                                         IdArrayType& superarcIntrinsicWeight,
                                         IdArrayType& superarcDependentWeight,
                                         IdArrayType& supernodeTransferWeight,
                                         IdArrayType& hyperarcDependentWeight)
  { // ContourTreeMaker::ComputeWeights()
    // start by storing the first sorted vertex ID for each superarc
    IdArrayType firstVertexForSuperparent;
    firstVertexForSuperparent.Allocate(contourTree.Superarcs.GetNumberOfValues());
    superarcIntrinsicWeight.Allocate(contourTree.Superarcs.GetNumberOfValues());
    auto superarcIntrinsicWeightPortal = superarcIntrinsicWeight.WritePortal();
    auto firstVertexForSuperparentPortal = firstVertexForSuperparent.WritePortal();
    auto superparentsPortal = contourTree.Superparents.ReadPortal();
    auto hyperparentsPortal = contourTree.Hyperparents.ReadPortal();
    auto hypernodesPortal = contourTree.Hypernodes.ReadPortal();
    auto hyperarcsPortal = contourTree.Hyperarcs.ReadPortal();
    // auto superarcsPortal = contourTree.Superarcs.ReadPortal();
    auto nodesPortal = contourTree.Nodes.ReadPortal();
    // auto whenTransferredPortal = contourTree.WhenTransferred.ReadPortal();
    for (viskores::Id sortedNode = 0; sortedNode < contourTree.Arcs.GetNumberOfValues();
         sortedNode++)
    { // per node in sorted order
      viskores::Id sortID = nodesPortal.Get(sortedNode);
      viskores::Id superparent = superparentsPortal.Get(sortID);
      if (sortedNode == 0)
        firstVertexForSuperparentPortal.Set(superparent, sortedNode);
      else if (superparent != superparentsPortal.Get(nodesPortal.Get(sortedNode - 1)))
        firstVertexForSuperparentPortal.Set(superparent, sortedNode);
    } // per node in sorted order
    // now we use that to compute the intrinsic weights
    for (viskores::Id superarc = 0; superarc < contourTree.Superarcs.GetNumberOfValues();
         superarc++)
      if (superarc == contourTree.Superarcs.GetNumberOfValues() - 1)
        superarcIntrinsicWeightPortal.Set(superarc,
                                          contourTree.Arcs.GetNumberOfValues() -
                                            firstVertexForSuperparentPortal.Get(superarc));
      else
        superarcIntrinsicWeightPortal.Set(superarc,
                                          firstVertexForSuperparentPortal.Get(superarc + 1) -
                                            firstVertexForSuperparentPortal.Get(superarc));

    // now initialise the arrays for transfer & dependent weights
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(
                                0, contourTree.Superarcs.GetNumberOfValues()),
                              superarcDependentWeight);
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(
                                0, contourTree.Supernodes.GetNumberOfValues()),
                              supernodeTransferWeight);
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(
                                0, contourTree.Hyperarcs.GetNumberOfValues()),
                              hyperarcDependentWeight);

    // set up the array which tracks which supernodes to deal with on which iteration
    auto firstSupernodePerIterationPortal = contourTree.FirstSupernodePerIteration.ReadPortal();
    auto firstHypernodePerIterationPortal = contourTree.FirstHypernodePerIteration.ReadPortal();
    auto supernodeTransferWeightPortal = supernodeTransferWeight.WritePortal();
    auto superarcDependentWeightPortal = superarcDependentWeight.WritePortal();
    auto hyperarcDependentWeightPortal = hyperarcDependentWeight.WritePortal();

    /*
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, nIterations + 1),
                          firstSupernodePerIteration);
    auto firstSupernodePerIterationPortal = firstSupernodePerIteration.WritePortal();
    for (viskores::Id supernode = 0; supernode < contourTree.Supernodes.GetNumberOfValues();
         supernode++)
    { // per supernode
      viskores::Id when = MaskedIndex(whenTransferredPortal.Get(supernode));
      if (supernode == 0)
      { // zeroth supernode
        firstSupernodePerIterationPortal.Set(when, supernode);
      } // zeroth supernode
      else if (when != MaskedIndex(whenTransferredPortal.Get(supernode - 1)))
      { // non-matching supernode
        firstSupernodePerIterationPortal.Set(when, supernode);
      } // non-matching supernode
    }   // per supernode
    for (viskores::Id iteration = 1; iteration < nIterations; ++iteration)
      if (firstSupernodePerIterationPortal.Get(iteration) == 0)
        firstSupernodePerIterationPortal.Set(iteration,
                                             firstSupernodePerIterationPortal.Get(iteration + 1));

    // set the sentinel at the end of the array
    firstSupernodePerIterationPortal.Set(nIterations, contourTree.Supernodes.GetNumberOfValues());

    // now use that array to construct a similar array for hypernodes
    IdArrayType firstHypernodePerIteration;
    firstHypernodePerIteration.Allocate(nIterations + 1);
    auto firstHypernodePerIterationPortal = firstHypernodePerIteration.WritePortal();
    auto supernodeTransferWeightPortal = supernodeTransferWeight.WritePortal();
    auto superarcDependentWeightPortal = superarcDependentWeight.WritePortal();
    auto hyperarcDependentWeightPortal = hyperarcDependentWeight.WritePortal();
    for (viskores::Id iteration = 0; iteration < nIterations; iteration++)
      firstHypernodePerIterationPortal.Set(
        iteration, hyperparentsPortal.Get(firstSupernodePerIterationPortal.Get(iteration)));
    firstHypernodePerIterationPortal.Set(nIterations, contourTree.Hypernodes.GetNumberOfValues());
    */

    // now iterate, propagating weights inwards
    for (viskores::Id iteration = 0; iteration < nIterations; iteration++)
    { // per iteration
      // pull the array bounds into register
      viskores::Id firstSupernode = firstSupernodePerIterationPortal.Get(iteration);
      viskores::Id lastSupernode = firstSupernodePerIterationPortal.Get(iteration + 1);
      viskores::Id firstHypernode = firstHypernodePerIterationPortal.Get(iteration);
      viskores::Id lastHypernode = firstHypernodePerIterationPortal.Get(iteration + 1);

      // Recall that the superarcs are sorted by (iteration, hyperarc), & that all superarcs for a given hyperarc are processed
      // in the same iteration.  Assume therefore that:
      //      i. we now have the intrinsic weight assigned for each superarc, and
      // ii. we also have the transfer weight assigned for each supernode.
      //
      // Suppose we have a sequence of superarcs
      //                      s11 s12 s13 s14 s21 s22 s23 s31
      // with transfer weights at their origins and intrinsic weights along them
      //      sArc                     s11 s12 s13 s14 s21 s22 s23 s31
      //      transfer wt               0   1   2   1   2   3   1   0
      //      intrinsic wt              1   2   1   5   2   6   1   1
      //
      //  now, if we do a prefix sum on each of these and add the two sums together, we get:
      //      sArc                                  s11 s12 s13 s14 s21 s22 s23 s31
      //      hyperparent sNode ID                  s11 s11 s11 s11 s21 s21 s21 s31
      //      transfer weight                       0   1   2   1   2   3   1   0
      //      intrinsic weight                      1   2   1   5   2   6   1   1
      //      sum(xfer + intrinsic)                 1   3   3   6   4   9   2   1
      //  prefix sum (xfer + int)                   1   4   7  13  17  26  28  29
      //  prefix sum (xfer + int - previous hArc)   1   4   7  13  4   13  15  16

      // so, step 1: add xfer + int & store in dependent weight
      for (viskores::Id supernode = firstSupernode; supernode < lastSupernode; supernode++)
      {
        superarcDependentWeightPortal.Set(supernode,
                                          supernodeTransferWeightPortal.Get(supernode) +
                                            superarcIntrinsicWeightPortal.Get(supernode));
      }

      // step 2: perform prefix sum on the dependent weight range
      for (viskores::Id supernode = firstSupernode + 1; supernode < lastSupernode; supernode++)
        superarcDependentWeightPortal.Set(supernode,
                                          superarcDependentWeightPortal.Get(supernode) +
                                            superarcDependentWeightPortal.Get(supernode - 1));

      // step 3: subtract out the dependent weight of the prefix to the entire hyperarc. This will be a transfer, but for now, it's easier
      // to show it in serial. NB: Loops backwards so that computation uses the correct value
      // As a bonus, note that we test > firstsupernode, not >=.  This is because we've got unsigned integers, & otherwise it will not terminate
      // But the first is always correct anyway (same reason as the short-cut termination on hyperparent), so we're fine
      for (viskores::Id supernode = lastSupernode - 1; supernode > firstSupernode; supernode--)
      { // per supernode
        // retrieve the hyperparent & convert to a supernode ID
        viskores::Id hyperparent = hyperparentsPortal.Get(supernode);
        viskores::Id hyperparentSuperID = hypernodesPortal.Get(hyperparent);

        // if the hyperparent is the first in the sequence, dependent weight is already correct
        if (hyperparent == firstHypernode)
          continue;

        // otherwise, subtract out the dependent weight *immediately* before the hyperparent's supernode
        superarcDependentWeightPortal.Set(
          supernode,
          superarcDependentWeightPortal.Get(supernode) -
            superarcDependentWeightPortal.Get(hyperparentSuperID - 1));
      } // per supernode

      // step 4: transfer the dependent weight to the hyperarc's target supernode
      for (viskores::Id hypernode = firstHypernode; hypernode < lastHypernode; hypernode++)
      { // per hypernode
        // last superarc for the hyperarc
        viskores::Id lastSuperarc;
        // special case for the last hyperarc
        if (hypernode == contourTree.Hypernodes.GetNumberOfValues() - 1)
          // take the last superarc in the array
          lastSuperarc = contourTree.Supernodes.GetNumberOfValues() - 1;
        else
          // otherwise, take the next hypernode's ID and subtract 1
          lastSuperarc = hypernodesPortal.Get(hypernode + 1) - 1;

        // now, given the last superarc for the hyperarc, transfer the dependent weight
        hyperarcDependentWeightPortal.Set(hypernode,
                                          superarcDependentWeightPortal.Get(lastSuperarc));

        // note that in parallel, this will have to be split out as a sort & partial sum in another array
        viskores::Id hyperarcTarget = MaskedIndex(hyperarcsPortal.Get(hypernode));
        supernodeTransferWeightPortal.Set(hyperarcTarget,
                                          supernodeTransferWeightPortal.Get(hyperarcTarget) +
                                            hyperarcDependentWeightPortal.Get(hypernode));
      } // per hypernode
    }   // per iteration
  }     // ContourTreeMaker::ComputeWeights()

  // routine to compute the branch decomposition by volume
  void static ComputeVolumeBranchDecompositionSerial(const ContourTree& contourTree,
                                                     const IdArrayType& superarcDependentWeight,
                                                     const IdArrayType& superarcIntrinsicWeight,
                                                     IdArrayType& whichBranch,
                                                     IdArrayType& branchMinimum,
                                                     IdArrayType& branchMaximum,
                                                     IdArrayType& branchSaddle,
                                                     IdArrayType& branchParent)
  { // ComputeVolumeBranchDecomposition()
    auto superarcDependentWeightPortal = superarcDependentWeight.ReadPortal();
    auto superarcIntrinsicWeightPortal = superarcIntrinsicWeight.ReadPortal();

    // cache the number of non-root supernodes & superarcs
    viskores::Id nSupernodes = contourTree.Supernodes.GetNumberOfValues();
    viskores::Id nSuperarcs = nSupernodes - 1;

    // STAGE I:  Find the upward and downwards weight for each superarc, and set up arrays
    IdArrayType upWeight;
    upWeight.Allocate(nSuperarcs);
    auto upWeightPortal = upWeight.WritePortal();
    IdArrayType downWeight;
    downWeight.Allocate(nSuperarcs);
    auto downWeightPortal = downWeight.WritePortal();
    IdArrayType bestUpward;
    auto noSuchElementArray =
      viskores::cont::ArrayHandleConstant<viskores::Id>((viskores::Id)NO_SUCH_ELEMENT, nSupernodes);
    viskores::cont::ArrayCopy(noSuchElementArray, bestUpward);
    IdArrayType bestDownward;
    viskores::cont::ArrayCopy(noSuchElementArray, bestDownward);
    viskores::cont::ArrayCopy(noSuchElementArray, whichBranch);
    auto bestUpwardPortal = bestUpward.WritePortal();
    auto bestDownwardPortal = bestDownward.WritePortal();

    // STAGE II: Pick the best (largest volume) edge upwards and downwards
    // II A. Pick the best upwards weight by sorting on lower vertex then processing by segments
    // II A 1.  Sort the superarcs by lower vertex
    // II A 2.  Per segment, best superarc writes to the best upwards array
    viskores::cont::ArrayHandle<EdgePair> superarcList;
    viskores::cont::ArrayCopy(
      viskores::cont::ArrayHandleConstant<EdgePair>(EdgePair(-1, -1), nSuperarcs), superarcList);
    auto superarcListWritePortal = superarcList.WritePortal();
    viskores::Id totalVolume = contourTree.Nodes.GetNumberOfValues();
#ifdef DEBUG_PRINT
    std::cout << "Total Volume: " << totalVolume << std::endl;
#endif
    auto superarcsPortal = contourTree.Superarcs.ReadPortal();

    // NB: Last element in array is guaranteed to be root superarc to infinity,
    // so we can easily skip it by not indexing to the full size
    for (viskores::Id superarc = 0; superarc < nSuperarcs; superarc++)
    { // per superarc
      if (IsAscending(superarcsPortal.Get(superarc)))
      { // ascending superarc
        superarcListWritePortal.Set(superarc,
                                    EdgePair(superarc, MaskedIndex(superarcsPortal.Get(superarc))));
        upWeightPortal.Set(superarc, superarcDependentWeightPortal.Get(superarc));
        // at the inner end, dependent weight is the total in the subtree.  Then there are vertices along the edge itself (intrinsic weight), including the supernode at the outer end
        // So, to get the "dependent" weight in the other direction, we start with totalVolume - dependent, then subtract (intrinsic - 1)
        downWeightPortal.Set(superarc,
                             (totalVolume - superarcDependentWeightPortal.Get(superarc)) +
                               (superarcIntrinsicWeightPortal.Get(superarc) - 1));
      } // ascending superarc
      else
      { // descending superarc
        superarcListWritePortal.Set(superarc,
                                    EdgePair(MaskedIndex(superarcsPortal.Get(superarc)), superarc));
        downWeightPortal.Set(superarc, superarcDependentWeightPortal.Get(superarc));
        // at the inner end, dependent weight is the total in the subtree.  Then there are vertices along the edge itself (intrinsic weight), including the supernode at the outer end
        // So, to get the "dependent" weight in the other direction, we start with totalVolume - dependent, then subtract (intrinsic - 1)
        upWeightPortal.Set(superarc,
                           (totalVolume - superarcDependentWeightPortal.Get(superarc)) +
                             (superarcIntrinsicWeightPortal.Get(superarc) - 1));
      } // descending superarc
    }   // per superarc

#ifdef DEBUG_PRINT
    std::cout << "II A. Weights Computed" << std::endl;
    PrintHeader(upWeight.GetNumberOfValues());
    //PrintIndices("Intrinsic Weight", superarcIntrinsicWeight);
    //PrintIndices("Dependent Weight", superarcDependentWeight);
    PrintIndices("Upwards Weight", upWeight);
    PrintIndices("Downwards Weight", downWeight);
    std::cout << std::endl;
#endif

    // II B. Pick the best downwards weight by sorting on upper vertex then processing by segments
    // II B 1.      Sort the superarcs by upper vertex
    IdArrayType superarcSorter;
    superarcSorter.Allocate(nSuperarcs);
    auto superarcSorterPortal = superarcSorter.WritePortal();
    for (viskores::Id superarc = 0; superarc < nSuperarcs; superarc++)
      superarcSorterPortal.Set(superarc, superarc);

    viskores::cont::Algorithm::Sort(
      superarcSorter,
      process_contourtree_inc_ns::SuperArcVolumetricComparator(upWeight, superarcList, false));

    // Initialize after in-place sort algorithm. (Kokkos)
    auto superarcSorterReadPortal = superarcSorter.ReadPortal();

    // II B 2.  Per segment, best superarc writes to the best upward array
    for (viskores::Id superarc = 0; superarc < nSuperarcs; superarc++)
    { // per superarc
      viskores::Id superarcID = superarcSorterReadPortal.Get(superarc);
      const EdgePair& edge = superarcListWritePortal.Get(superarcID);
      // if it's the last one
      if (superarc == nSuperarcs - 1)
        bestDownwardPortal.Set(edge.second, edge.first);
      else
      { // not the last one
        const EdgePair& nextEdge =
          superarcListWritePortal.Get(superarcSorterReadPortal.Get(superarc + 1));
        // if the next edge belongs to another, we're the highest
        if (nextEdge.second != edge.second)
          bestDownwardPortal.Set(edge.second, edge.first);
      } // not the last one
    }   // per superarc

    // II B 3.  Repeat for lower vertex
    viskores::cont::Algorithm::Sort(
      superarcSorter,
      process_contourtree_inc_ns::SuperArcVolumetricComparator(downWeight, superarcList, true));

    // Re-initialize after in-place sort algorithm. (Kokkos)
    superarcSorterReadPortal = superarcSorter.ReadPortal();

    // II B 2.  Per segment, best superarc writes to the best upward array
    for (viskores::Id superarc = 0; superarc < nSuperarcs; superarc++)
    { // per superarc
      viskores::Id superarcID = superarcSorterReadPortal.Get(superarc);
      const EdgePair& edge = superarcListWritePortal.Get(superarcID);
      // if it's the last one
      if (superarc == nSuperarcs - 1)
        bestUpwardPortal.Set(edge.first, edge.second);
      else
      { // not the last one
        const EdgePair& nextEdge =
          superarcListWritePortal.Get(superarcSorterReadPortal.Get(superarc + 1));
        // if the next edge belongs to another, we're the highest
        if (nextEdge.first != edge.first)
          bestUpwardPortal.Set(edge.first, edge.second);
      } // not the last one
    }   // per superarc

#ifdef DEBUG_PRINT
    std::cout << "II. Best Edges Selected" << std::endl;
    PrintHeader(bestUpward.GetNumberOfValues());
    PrintIndices("Best Upwards", bestUpward);
    PrintIndices("Best Downwards", bestDownward);
    std::cout << std::endl;
#endif

    ProcessContourTree::ComputeBranchData(contourTree,
                                          whichBranch,
                                          branchMinimum,
                                          branchMaximum,
                                          branchSaddle,
                                          branchParent,
                                          bestUpward,
                                          bestDownward);
  }

  // routine to compute the branch decomposition by volume
  void static ComputeBranchData(const ContourTree& contourTree,
                                IdArrayType& whichBranch,
                                IdArrayType& branchMinimum,
                                IdArrayType& branchMaximum,
                                IdArrayType& branchSaddle,
                                IdArrayType& branchParent,
                                IdArrayType& bestUpward,
                                IdArrayType& bestDownward)
  { // ComputeBranchData()

    // Set up constants
    viskores::Id nSupernodes = contourTree.Supernodes.GetNumberOfValues();
    auto noSuchElementArray =
      viskores::cont::ArrayHandleConstant<viskores::Id>((viskores::Id)NO_SUCH_ELEMENT, nSupernodes);
    viskores::cont::ArrayCopy(noSuchElementArray, whichBranch);

    // STAGE III: For each vertex, identify which neighbours are on same branch
    // Let v = BestUp(u). Then if u = BestDown(v), copy BestUp(u) to whichBranch(u)
    // Otherwise, let whichBranch(u) = BestUp(u) | TERMINAL to mark the end of the side branch
    // NB 1: Leaves already have the flag set, but it's redundant so its safe
    // NB 2: We don't need to do it downwards because it's symmetric
    viskores::cont::Invoker invoke;
    viskores::worklet::contourtree_augmented::process_contourtree_inc::PropagateBestUpDown
      propagateBestUpDownWorklet;
    invoke(propagateBestUpDownWorklet, bestUpward, bestDownward, whichBranch);

#ifdef DEBUG_PRINT
    std::cout << "III. Branch Neighbours Identified" << std::endl;
    PrintHeader(whichBranch.GetNumberOfValues());
    PrintIndices("Which Branch", whichBranch);
    std::cout << std::endl;
#endif

    // STAGE IV: Use pointer-doubling on whichBranch to propagate branches
    // Compute the number of log steps required in this pass
    viskores::Id numLogSteps = 1;
    for (viskores::Id shifter = nSupernodes; shifter != 0; shifter >>= 1)
      numLogSteps++;

    viskores::worklet::contourtree_augmented::process_contourtree_inc::PointerDoubling
      pointerDoubling(nSupernodes);

    // use pointer-doubling to build the branches
    for (viskores::Id iteration = 0; iteration < numLogSteps; iteration++)
    { // per iteration
      invoke(pointerDoubling, whichBranch);
    } // per iteration


#ifdef DEBUG_PRINT
    std::cout << "IV. Branch Chains Propagated" << std::endl;
    PrintHeader(whichBranch.GetNumberOfValues());
    PrintIndices("Which Branch", whichBranch);
    std::cout << std::endl;
#endif

    // Initialise
    IdArrayType chainToBranch;
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, nSupernodes),
                              chainToBranch);

    // Set 1 to every relevant
    viskores::worklet::contourtree_augmented::process_contourtree_inc::PrepareChainToBranch
      prepareChainToBranchWorklet;
    invoke(prepareChainToBranchWorklet, whichBranch, chainToBranch);

    // Prefix scanto get IDs
    viskores::Id nBranches = viskores::cont::Algorithm::ScanInclusive(chainToBranch, chainToBranch);

    viskores::worklet::contourtree_augmented::process_contourtree_inc::FinaliseChainToBranch
      finaliseChainToBranchWorklet;
    invoke(finaliseChainToBranchWorklet, whichBranch, chainToBranch);

    // V B.  Create the arrays for the branches
    auto noSuchElementArrayNBranches =
      viskores::cont::ArrayHandleConstant<viskores::Id>((viskores::Id)NO_SUCH_ELEMENT, nBranches);
    viskores::cont::ArrayCopy(noSuchElementArrayNBranches, branchMinimum);
    viskores::cont::ArrayCopy(noSuchElementArrayNBranches, branchMaximum);
    viskores::cont::ArrayCopy(noSuchElementArrayNBranches, branchSaddle);
    viskores::cont::ArrayCopy(noSuchElementArrayNBranches, branchParent);

#ifdef DEBUG_PRINT
    std::cout << "V. Branch Arrays Created" << std::endl;
    PrintHeader(chainToBranch.GetNumberOfValues());
    PrintIndices("Chain To Branch", chainToBranch);
    PrintHeader(nBranches);
    PrintIndices("Branch Minimum", branchMinimum);
    PrintIndices("Branch Maximum", branchMaximum);
    PrintIndices("Branch Saddle", branchSaddle);
    PrintIndices("Branch Parent", branchParent);
#endif

    IdArrayType supernodeSorter;
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(nSupernodes), supernodeSorter);

    viskores::cont::Algorithm::Sort(
      supernodeSorter,
      process_contourtree_inc_ns::SuperNodeBranchComparator(whichBranch, contourTree.Supernodes));

    IdArrayType permutedBranches;
    permutedBranches.Allocate(nSupernodes);
    PermuteArrayWithMaskedIndex<viskores::Id>(whichBranch, supernodeSorter, permutedBranches);

    IdArrayType permutedRegularID;
    permutedRegularID.Allocate(nSupernodes);
    PermuteArrayWithMaskedIndex<viskores::Id>(
      contourTree.Supernodes, supernodeSorter, permutedRegularID);

#ifdef DEBUG_PRINT
    std::cout << "VI A. Sorted into Branches" << std::endl;
    PrintHeader(nSupernodes);
    PrintIndices("Supernode IDs", supernodeSorter);
    PrintIndices("Branch", permutedBranches);
    PrintIndices("Regular ID", permutedRegularID);
#endif

    viskores::worklet::contourtree_augmented::process_contourtree_inc::WhichBranchNewId
      whichBranchNewIdWorklet;
    invoke(whichBranchNewIdWorklet, chainToBranch, whichBranch);

    viskores::worklet::contourtree_augmented::process_contourtree_inc::BranchMinMaxSet
      branchMinMaxSetWorklet(nSupernodes);
    invoke(branchMinMaxSetWorklet, supernodeSorter, whichBranch, branchMinimum, branchMaximum);

#ifdef DEBUG_PRINT
    std::cout << "VI. Branches Set" << std::endl;
    PrintHeader(nBranches);
    PrintIndices("Branch Maximum", branchMaximum);
    PrintIndices("Branch Minimum", branchMinimum);
    PrintIndices("Branch Saddle", branchSaddle);
    PrintIndices("Branch Parent", branchParent);
#endif

    viskores::worklet::contourtree_augmented::process_contourtree_inc::BranchSaddleParentSet
      branchSaddleParentSetWorklet;
    invoke(branchSaddleParentSetWorklet,
           whichBranch,
           branchMinimum,
           branchMaximum,
           bestDownward,
           bestUpward,
           branchSaddle,
           branchParent);

#ifdef DEBUG_PRINT
    std::cout << "VII. Branches Constructed" << std::endl;
    PrintHeader(nBranches);
    PrintIndices("Branch Maximum", branchMaximum);
    PrintIndices("Branch Minimum", branchMinimum);
    PrintIndices("Branch Saddle", branchSaddle);
    PrintIndices("Branch Parent", branchParent);
#endif

  } // ComputeBranchData()

  // Create branch decomposition from contour tree
  template <typename T, typename StorageType>
  static process_contourtree_inc_ns::Branch<T>* ComputeBranchDecomposition(
    const IdArrayType& contourTreeSuperparents,
    const IdArrayType& contourTreeSupernodes,
    const IdArrayType& whichBranch,
    const IdArrayType& branchMinimum,
    const IdArrayType& branchMaximum,
    const IdArrayType& branchSaddle,
    const IdArrayType& branchParent,
    const IdArrayType& sortOrder,
    const viskores::cont::ArrayHandle<T, StorageType>& dataField,
    bool dataFieldIsSorted)
  {
    return process_contourtree_inc_ns::Branch<T>::ComputeBranchDecomposition(
      contourTreeSuperparents,
      contourTreeSupernodes,
      whichBranch,
      branchMinimum,
      branchMaximum,
      branchSaddle,
      branchParent,
      sortOrder,
      dataField,
      dataFieldIsSorted);
  }

  // routine to compute the branch decomposition by volume
  void static ComputeVolumeBranchDecomposition(const ContourTree& contourTree,
                                               const viskores::Id nIterations,
                                               IdArrayType& whichBranch,
                                               IdArrayType& branchMinimum,
                                               IdArrayType& branchMaximum,
                                               IdArrayType& branchSaddle,
                                               IdArrayType& branchParent)
  { // ComputeHeightBranchDecomposition()

    viskores::cont::Invoker Invoke;

    // STEP 1. Compute the number of nodes in every superarc, that's the intrinsic weight
    IdArrayType superarcIntrinsicWeight;
    superarcIntrinsicWeight.Allocate(contourTree.Superarcs.GetNumberOfValues());

    IdArrayType firstVertexForSuperparent;
    firstVertexForSuperparent.Allocate(contourTree.Superarcs.GetNumberOfValues());

    // Compute the number of regular nodes on every superarcs (the intrinsic weight)
    viskores::worklet::contourtree_augmented::process_contourtree_inc::SetFirstVertexForSuperparent
      setFirstVertexForSuperparent;
    Invoke(setFirstVertexForSuperparent,
           contourTree.Nodes,
           contourTree.Superparents,
           firstVertexForSuperparent);

    viskores::worklet::contourtree_augmented::process_contourtree_inc::ComputeIntrinsicWeight
      computeIntrinsicWeight;
    Invoke(computeIntrinsicWeight,
           contourTree.Arcs,
           contourTree.Superarcs,
           firstVertexForSuperparent,
           superarcIntrinsicWeight);


    // Cache the number of non-root supernodes & superarcs
    viskores::Id nSupernodes = contourTree.Supernodes.GetNumberOfValues();
    auto noSuchElementArray =
      viskores::cont::ArrayHandleConstant<viskores::Id>((viskores::Id)NO_SUCH_ELEMENT, nSupernodes);

    // Set up bestUpward and bestDownward array, these are the things we want to compute in this routine.
    IdArrayType bestUpward, bestDownward;
    viskores::cont::ArrayCopy(noSuchElementArray, bestUpward);
    viskores::cont::ArrayCopy(noSuchElementArray, bestDownward);

    // We initiale with the weight of the superarcs, once we sum those up we'll get the hypersweep weight
    IdArrayType sumValues;
    viskores::cont::ArrayCopy(superarcIntrinsicWeight, sumValues);

    // This should be 0 here, because we're not changing the root
    viskores::cont::ArrayHandle<viskores::Id> howManyUsed;
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(
                                0, contourTree.Hyperarcs.GetNumberOfValues()),
                              howManyUsed);

    // Perform a sum hypersweep
    hyperarcScan<decltype(viskores::Sum())>(contourTree.Supernodes,
                                            contourTree.Hypernodes,
                                            contourTree.Hyperarcs,
                                            contourTree.Hyperparents,
                                            contourTree.Hyperparents,
                                            contourTree.WhenTransferred,
                                            howManyUsed,
                                            nIterations,
                                            viskores::Sum(),
                                            sumValues);

    // For every directed arc store the volume of it's associate subtree
    viskores::cont::ArrayHandle<viskores::worklet::contourtree_augmented::EdgeDataVolume> arcs;
    arcs.Allocate(contourTree.Superarcs.GetNumberOfValues() * 2 - 2);

    viskores::Id totalVolume = contourTree.Nodes.GetNumberOfValues();
    viskores::worklet::contourtree_augmented::process_contourtree_inc::InitialiseArcsVolume
      initArcs(totalVolume);
    Invoke(initArcs, sumValues, superarcIntrinsicWeight, contourTree.Superarcs, arcs);

    // Sort arcs to obtain the best up and down
    viskores::cont::Algorithm::Sort(arcs, viskores::SortLess());

    viskores::worklet::contourtree_augmented::process_contourtree_inc::SetBestUpDown setBestUpDown;
    Invoke(setBestUpDown, bestUpward, bestDownward, arcs);

    ProcessContourTree::ComputeBranchData(contourTree,
                                          whichBranch,
                                          branchMinimum,
                                          branchMaximum,
                                          branchSaddle,
                                          branchParent,
                                          bestUpward,
                                          bestDownward);

  } // ComputeHeightBranchDecomposition()

  // routine to compute the branch decomposition by volume
  void static ComputeHeightBranchDecomposition(const ContourTree& contourTree,
                                               const cont::ArrayHandle<Float64> fieldValues,
                                               const IdArrayType& ctSortOrder,
                                               const viskores::Id nIterations,
                                               IdArrayType& whichBranch,
                                               IdArrayType& branchMinimum,
                                               IdArrayType& branchMaximum,
                                               IdArrayType& branchSaddle,
                                               IdArrayType& branchParent)
  { // ComputeHeightBranchDecomposition()

    // Cache the number of non-root supernodes & superarcs
    viskores::Id nSupernodes = contourTree.Supernodes.GetNumberOfValues();
    auto noSuchElementArray =
      viskores::cont::ArrayHandleConstant<viskores::Id>((viskores::Id)NO_SUCH_ELEMENT, nSupernodes);

    // Set up bestUpward and bestDownward array, these are the things we want to compute in this routine.
    IdArrayType bestUpward, bestDownward;
    viskores::cont::ArrayCopy(noSuchElementArray, bestUpward);
    viskores::cont::ArrayCopy(noSuchElementArray, bestDownward);

    // maxValues and minValues store the values from the max and min hypersweep respectively.
    IdArrayType minValues, maxValues;
    viskores::cont::ArrayCopy(contourTree.Supernodes, maxValues);
    viskores::cont::ArrayCopy(contourTree.Supernodes, minValues);

    // Store the direction of the superarcs in the min and max hypersweep (find a way to get rid of these, the only differing direction is on the path from the root to the min/max).
    IdArrayType minParents, maxParents;
    viskores::cont::ArrayCopy(contourTree.Superarcs, minParents);
    viskores::cont::ArrayCopy(contourTree.Superarcs, maxParents);

    auto minParentsPortal = minParents.WritePortal();
    auto maxParentsPortal = maxParents.WritePortal();

    // Cache the glonal minimum and global maximum (these will be the roots in the min and max hypersweep)
    Id minSuperNode = MaskedIndex(contourTree.Superparents.ReadPortal().Get(0));
    Id maxSuperNode = MaskedIndex(
      contourTree.Superparents.ReadPortal().Get(contourTree.Nodes.GetNumberOfValues() - 1));

    // Find the path from the global minimum to the root, not parallelisable (but it's fast, no need to parallelise)
    auto minPath = findSuperPathToRoot(contourTree.Superarcs.ReadPortal(), minSuperNode);

    // Find the path from the global minimum to the root, not parallelisable (but it's fast, no need to parallelise)
    auto maxPath = findSuperPathToRoot(contourTree.Superarcs.ReadPortal(), maxSuperNode);

    // Reserve the direction of the superarcs on the min path.
    for (std::size_t i = 1; i < minPath.size(); i++)
    {
      minParentsPortal.Set(minPath[i], minPath[i - 1]);
    }
    minParentsPortal.Set(minPath[0], 0);

    // Reserve the direction of the superarcs on the max path.
    for (std::size_t i = 1; i < maxPath.size(); i++)
    {
      maxParentsPortal.Set(maxPath[i], maxPath[i - 1]);
    }
    maxParentsPortal.Set(maxPath[0], 0);

    viskores::cont::Invoker Invoke;
    viskores::worklet::contourtree_augmented::process_contourtree_inc::UnmaskArray
      unmaskArrayWorklet;
    Invoke(unmaskArrayWorklet, minValues);
    Invoke(unmaskArrayWorklet, maxValues);

    // Thse arrays hold the changes hyperarcs in the min and max hypersweep respectively
    viskores::cont::ArrayHandle<viskores::Id> minHyperarcs, maxHyperarcs;
    viskores::cont::ArrayCopy(contourTree.Hyperarcs, minHyperarcs);
    viskores::cont::ArrayCopy(contourTree.Hyperarcs, maxHyperarcs);

    // These arrays hold the changed hyperarcs for the min and max hypersweep
    viskores::cont::ArrayHandle<viskores::Id> minHyperparents, maxHyperparents;
    viskores::cont::ArrayCopy(contourTree.Hyperparents, minHyperparents);
    viskores::cont::ArrayCopy(contourTree.Hyperparents, maxHyperparents);

    auto minHyperparentsPortal = minHyperparents.WritePortal();
    auto maxHyperparentsPortal = maxHyperparents.WritePortal();

    for (std::size_t i = 0; i < minPath.size(); i++)
    {
      // Set a unique dummy Id (something that the prefix scan by key will leave alone)
      minHyperparentsPortal.Set(minPath[i],
                                contourTree.Hypernodes.GetNumberOfValues() + minPath[i]);
    }

    for (std::size_t i = 0; i < maxPath.size(); i++)
    {
      // Set a unique dummy Id (something that the prefix scan by key will leave alone)
      maxHyperparentsPortal.Set(maxPath[i],
                                contourTree.Hypernodes.GetNumberOfValues() + maxPath[i]);
    }

    // These arrays hold the number of nodes in each hypearcs that are on the min or max path for the min and max hypersweep respectively.
    viskores::cont::ArrayHandle<viskores::Id> minHowManyUsed, maxHowManyUsed;
    viskores::cont::ArrayCopy(
      viskores::cont::ArrayHandleConstant<viskores::Id>(0, maxHyperarcs.GetNumberOfValues()),
      minHowManyUsed);
    viskores::cont::ArrayCopy(
      viskores::cont::ArrayHandleConstant<viskores::Id>(0, maxHyperarcs.GetNumberOfValues()),
      maxHowManyUsed);

    // Min Hypersweep
    const auto minOperator = viskores::Minimum();

    // Cut hyperarcs at the first node on the path from the max to the root
    editHyperarcs(contourTree.Hyperparents.ReadPortal(),
                  minPath,
                  minHyperarcs.WritePortal(),
                  minHowManyUsed.WritePortal());

    // Perform an ordinary hypersweep on those new hyperarcs
    hyperarcScan<decltype(viskores::Minimum())>(contourTree.Supernodes,
                                                contourTree.Hypernodes,
                                                minHyperarcs,
                                                contourTree.Hyperparents,
                                                minHyperparents,
                                                contourTree.WhenTransferred,
                                                minHowManyUsed,
                                                nIterations,
                                                viskores::Minimum(),
                                                minValues);

    // Prefix sum along the path from the min to the root
    fixPath(viskores::Minimum(), minPath, minValues.WritePortal());

    // Max Hypersweep
    const auto maxOperator = viskores::Maximum();

    // Cut hyperarcs at the first node on the path from the max to the root
    editHyperarcs(contourTree.Hyperparents.ReadPortal(),
                  maxPath,
                  maxHyperarcs.WritePortal(),
                  maxHowManyUsed.WritePortal());

    // Perform an ordinary hypersweep on those new hyperarcs
    hyperarcScan<decltype(viskores::Maximum())>(contourTree.Supernodes,
                                                contourTree.Hypernodes,
                                                maxHyperarcs,
                                                contourTree.Hyperparents,
                                                maxHyperparents,
                                                contourTree.WhenTransferred,
                                                maxHowManyUsed,
                                                nIterations,
                                                viskores::Maximum(),
                                                maxValues);

    // Prefix sum along the path from the max to the root
    fixPath(viskores::Maximum(), maxPath, maxValues.WritePortal());

    // For every directed edge (a, b) consider that subtree who's root is b and does not contain a.
    // We have so far found the min and max in all sub subtrees, now we compare those to a and incorporate a into that.
    viskores::worklet::contourtree_augmented::process_contourtree_inc::IncorporateParent<
      decltype(viskores::Minimum())>
      incorporateParentMinimumWorklet(minOperator);
    Invoke(incorporateParentMinimumWorklet, minParents, contourTree.Supernodes, minValues);

    viskores::worklet::contourtree_augmented::process_contourtree_inc::IncorporateParent<
      decltype(viskores::Maximum())>
      incorporateParentMaximumWorklet(maxOperator);
    Invoke(incorporateParentMaximumWorklet, maxParents, contourTree.Supernodes, maxValues);

    // Initialise all directed superarcs in the contour tree. Those will correspond to subtrees whos height we need for the branch decomposition.
    viskores::cont::ArrayHandle<viskores::worklet::contourtree_augmented::EdgeDataHeight> arcs;
    arcs.Allocate(contourTree.Superarcs.GetNumberOfValues() * 2 - 2);

    viskores::worklet::contourtree_augmented::process_contourtree_inc::InitialiseArcs initArcs(
      0, contourTree.Arcs.GetNumberOfValues() - 1, minPath[minPath.size() - 1]);

    Invoke(initArcs, minParents, maxParents, minValues, maxValues, contourTree.Superarcs, arcs);

    // Use the min & max to compute the height of all subtrees
    viskores::worklet::contourtree_augmented::process_contourtree_inc::ComputeSubtreeHeight
      computeSubtreeHeight;
    Invoke(computeSubtreeHeight, fieldValues, ctSortOrder, contourTree.Supernodes, arcs);

    // Sort all directed edges based on the height of their subtree
    viskores::cont::Algorithm::Sort(arcs, viskores::SortLess());

    // Select a best up and best down neighbour for every vertex in the contour tree using heights of all subtrees
    viskores::worklet::contourtree_augmented::process_contourtree_inc::SetBestUpDown setBestUpDown;
    Invoke(setBestUpDown, bestUpward, bestDownward, arcs);

    // Having computed the bestUp/Down we can propagte those to obtain the branches of the branch decomposition
    ProcessContourTree::ComputeBranchData(contourTree,
                                          whichBranch,
                                          branchMinimum,
                                          branchMaximum,
                                          branchSaddle,
                                          branchParent,
                                          bestUpward,
                                          bestDownward);

  } // ComputeHeightBranchDecomposition()

  std::vector<Id> static findSuperPathToRoot(
    viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType parentsPortal,
    viskores::Id vertex)
  {
    // Initialise the empty path and starting vertex
    std::vector<viskores::Id> path;
    viskores::Id current = vertex;

    // Go up the parent list until we reach the root
    while (MaskedIndex(parentsPortal.Get(current)) != 0)
    {
      path.push_back(current);
      current = MaskedIndex(parentsPortal.Get(current));
    }
    path.push_back(current);

    return path;
  }

  // Given a path from a leaf (the global min/max) to the root of the contour tree and a hypersweep (where all hyperarcs are cut at the path)
  // This function performs a prefix scan along that path to obtain the correct hypersweep values (as in the global min/max is the root of the hypersweep)
  void static fixPath(const std::function<viskores::Id(viskores::Id, viskores::Id)> operation,
                      const std::vector<viskores::Id> path,
                      viskores::cont::ArrayHandle<viskores::Id>::WritePortalType minMaxIndex)
  {
    using viskores::worklet::contourtree_augmented::MaskedIndex;

    // Fix path from the old root to the new root. Parallelisble with a prefix scan, but sufficiently fast for now.
    for (auto i = path.size() - 2; i > 0; i--)
    {
      const auto vertex = path[i + 1];
      const auto parent = path[i];

      const auto vertexValue = minMaxIndex.Get(vertex);
      const auto parentValue = minMaxIndex.Get(parent);

      minMaxIndex.Set(parent, operation(vertexValue, parentValue));
    }
  }

  // This function edits all the hyperarcs which contain vertices which are on the supplied path. This path is usually the path between the global min/max to the root of the tree.
  // This function effectively cuts hyperarcs at the first node they encounter along that path.
  // In addition to this it computed the number of supernodes every hyperarc has on that path. This helps in the function hyperarcScan for choosing the new target of the cut hyperarcs.
  // NOTE: It is assumed that the supplied path starts at a leaf and ends at the root of the contour tree.
  void static editHyperarcs(
    const viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType hyperparentsPortal,
    const std::vector<viskores::Id> path,
    viskores::cont::ArrayHandle<viskores::Id>::WritePortalType hyperarcsPortal,
    viskores::cont::ArrayHandle<viskores::Id>::WritePortalType howManyUsedPortal)
  {
    using viskores::worklet::contourtree_augmented::MaskedIndex;

    std::size_t i = 0;
    while (i < path.size())
    {
      // Cut the hyperacs at the first point
      hyperarcsPortal.Set(MaskedIndex(hyperparentsPortal.Get(path[i])), path[i]);

      Id currentHyperparent = MaskedIndex(hyperparentsPortal.Get(path[i]));

      // Skip the rest of the supernodes which are on the same hyperarc
      while (i < path.size() && MaskedIndex(hyperparentsPortal.Get(path[i])) == currentHyperparent)
      {
        const auto value = howManyUsedPortal.Get(MaskedIndex(hyperparentsPortal.Get(path[i])));
        howManyUsedPortal.Set(MaskedIndex(hyperparentsPortal.Get(path[i])), value + 1);
        i++;
      }
    }
  }

  template <class BinaryFunctor>
  void static hyperarcScan(const viskores::cont::ArrayHandle<viskores::Id> supernodes,
                           const viskores::cont::ArrayHandle<viskores::Id> hypernodes,
                           const viskores::cont::ArrayHandle<viskores::Id> hyperarcs,
                           const viskores::cont::ArrayHandle<viskores::Id> hyperparents,
                           const viskores::cont::ArrayHandle<viskores::Id> hyperparentKeys,
                           const viskores::cont::ArrayHandle<viskores::Id> whenTransferred,
                           const viskores::cont::ArrayHandle<viskores::Id> howManyUsed,
                           const viskores::Id nIterations,
                           const BinaryFunctor operation,
                           viskores::cont::ArrayHandle<viskores::Id> minMaxIndex)
  {
    using viskores::worklet::contourtree_augmented::MaskedIndex;

    viskores::cont::Invoker invoke;

    auto supernodesPortal = supernodes.ReadPortal();
    auto hypernodesPortal = hypernodes.ReadPortal();
    auto hyperparentsPortal = hyperparents.ReadPortal();

    // Set the first supernode per iteration
    viskores::cont::ArrayHandle<viskores::Id> firstSupernodePerIteration;
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, nIterations + 1),
                              firstSupernodePerIteration);

    // The first different from the previous is the first in the iteration
    viskores::worklet::contourtree_augmented::process_contourtree_inc::SetFirstSupernodePerIteration
      setFirstSupernodePerIteration;
    invoke(setFirstSupernodePerIteration, whenTransferred, firstSupernodePerIteration);

    auto firstSupernodePerIterationPortal = firstSupernodePerIteration.WritePortal();
    for (viskores::Id iteration = 1; iteration < nIterations; ++iteration)
    {
      if (firstSupernodePerIterationPortal.Get(iteration) == 0)
      {
        firstSupernodePerIterationPortal.Set(iteration,
                                             firstSupernodePerIterationPortal.Get(iteration + 1));
      }
    }

    // set the sentinel at the end of the array
    firstSupernodePerIterationPortal.Set(nIterations, supernodesPortal.GetNumberOfValues());

    // Set the first hypernode per iteration
    viskores::cont::ArrayHandle<viskores::Id> firstHypernodePerIteration;
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, nIterations + 1),
                              firstHypernodePerIteration);
    auto firstHypernodePerIterationPortal = firstHypernodePerIteration.WritePortal();

    for (viskores::Id iteration = 0; iteration < nIterations; iteration++)
    {
      firstHypernodePerIterationPortal.Set(
        iteration, hyperparentsPortal.Get(firstSupernodePerIterationPortal.Get(iteration)));
    }

    // Set the sentinel at the end of the array
    firstHypernodePerIterationPortal.Set(nIterations, hypernodesPortal.GetNumberOfValues());

    // This workled is used in every iteration of the following loop, so it's initialised outside.
    viskores::worklet::contourtree_augmented::process_contourtree_inc::AddDependentWeightHypersweep<
      BinaryFunctor>
      addDependentWeightHypersweepWorklet(operation);

    // For every iteration do a prefix scan on all hyperarcs in that iteration and then transfer the scanned value to every hyperarc's target supernode
    for (viskores::Id iteration = 0; iteration < nIterations; iteration++)
    {
      // Determine the first and last hypernode in the current iteration (all hypernodes between them are also in the current iteration)
      viskores::Id firstHypernode = firstHypernodePerIterationPortal.Get(iteration);
      viskores::Id lastHypernode = firstHypernodePerIterationPortal.Get(iteration + 1);
      lastHypernode = viskores::Minimum()(lastHypernode, hypernodes.GetNumberOfValues() - 1);

      // Determine the first and last supernode in the current iteration (all supernode between them are also in the current iteration)
      viskores::Id firstSupernode = MaskedIndex(hypernodesPortal.Get(firstHypernode));
      viskores::Id lastSupernode = MaskedIndex(hypernodesPortal.Get(lastHypernode));
      lastSupernode = viskores::Minimum()(lastSupernode, hyperparents.GetNumberOfValues() - 1);

      // Prefix scan along all hyperarcs in the current iteration
      auto subarrayValues = viskores::cont::make_ArrayHandleView(
        minMaxIndex, firstSupernode, lastSupernode - firstSupernode);
      auto subarrayKeys = viskores::cont::make_ArrayHandleView(
        hyperparentKeys, firstSupernode, lastSupernode - firstSupernode);
      viskores::cont::Algorithm::ScanInclusiveByKey(
        subarrayKeys, subarrayValues, subarrayValues, operation);

      // Array containing the Ids of the hyperarcs in the current iteration
      viskores::cont::ArrayHandleCounting<viskores::Id> iterationHyperarcs(
        firstHypernode, 1, lastHypernode - firstHypernode);

      // Transfer the value accumulated in the last entry of the prefix scan to the hypernode's targe supernode
      invoke(addDependentWeightHypersweepWorklet,
             iterationHyperarcs,
             hypernodes,
             hyperarcs,
             howManyUsed,
             minMaxIndex);
    }
  }
}; // class ProcessContourTree
} // namespace contourtree_augmented
} // worklet
} // viskores

#endif
