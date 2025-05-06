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
//=======================================================================================
//
//  Parallel Peak Pruning v. 2.0
//
//  Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// HierarchicalVolumetricBranchDecomposer.h
//
//=======================================================================================
//
// COMMENTS:
//
//      This class computes the branch decomposition by volume for a given hierarchical
//      contour tree.
//
//      It takes as input arrays of dependent and intrinsic volumes for each superarc
//      (it needs both, in order to compute the dependent volume at each end of each superarc)
//
//      Recall from the non-hierarchical version that in order to compute the branch decomposition,
//      we need to choose the "best up" and "best down" superarc for each supernode - i.e. the
//      superarc with the largest dependent volume. Since we only wish to compare superarcs that
//      meet at a given supernode, we tiebreak by always taking the superarc whose "other" end
//      has a higher ID.
//
//      Once the best up & best down have been found for each supernode, branches are identified
//      with (essentially) a graph connectivity computation.
//
//      Conceptually, each superarc is a vertex in a new (temporary) graph. For each supernode, the
//      "best up" superarc is connected to the "best down" superarc.  This defines a graph in which
//      each branch is a connected component.  A single path-doubling pass then collects the branches
//
//      In the non-hierarchical version, this was done with supernode IDs (I can't remember why),
//      with the upper end of each branch being treated as the root node.
//
//      To construct the hierarchical branch decomposition, we assume that the hierarchical contour
//      tree has already been augmented with all attachment points.  If not, the code may produce
//      undefined results.
//
//      In the first step, we will run a local routine for each rank to determine the best up / down
//      as far as the rank knows.  We will then do a fan-in swap to determine the best up / down for
//      shared vertices. At the end of this step, all ranks will share the knowledge of the best
//      up / down superarc, stored as:
//      i.              the superarc ID, which may be reused on other ranks
//      ii.             the global ID of the outer end of that superarc, which is unique across all ranks
//      iii.    the volume dependent on that superarc
//
//      In the second stage, each rank will do a local computation of the branches. However, most ranks
//      will not have the full set of supernodes / superarcs for each branch, even (or especially)
//      for the master branch.  It is therefore a bad idea to collapse to the upper end of the branch
//      as we did in the non-hierarchical version.
//
//      Instead, we will define the root of each component to be the most senior superarc ID.  This will
//      be canonical, because of the way we construct the hierarchical tree, with low superarc IDs
//      occurring at higher levels of the tree, so all shared superarcs are a prefix set.  Therefore,
//      the most senior superarc ID will always indicate the highest level of the tree through which the
//      branch passes, and is safe.  Moreover, it is not necessary for each rank to determine the full
//      branch, merely the part of the branch that passes through the superarcs it tracks.  It may even
//      happen that no single rank stores the entire branch, as for example if the global minimum
//      and maximum are interior to different ranks.
//
//      Note that most senior means testing iteration, round, then ID
//
//      RESIZE SEMANTICS:  Oliver Ruebel has asked for the semantics of all resize() calls to be annotated
//      in order to ease porting to viskores.  These will be flagged with a RESIZE SEMANTICS: comment, and will
//      generally fall into several patterns:
//      1.      FIXED:                  Resize() is used to initialize the array size for an array that will never change size
//      2.      COMPRESS:               Resize() is used after a compression operation (eg remove_if()) so that the
//                                              array size() call does not include the elements removed.  This is a standard
//                                              C++ pattern, but could be avoided by storing an explicit element count (curiously,
//                                              the std::vector class does exactly this with logical vs. physical array sizes).
//      3.      MULTI-COMPRESS: Resize() may also be used (as in the early stages of the PPP algorithm) to give
//                                              a collapsing array size of working elements.  Again, this could on principle by
//                                              avoided with an array count, but is likely to be intricate.
//
//=======================================================================================


#ifndef viskores_filter_scalar_topology_worklet_HierarchicalVolumetricBranchDecomposer_h
#define viskores_filter_scalar_topology_worklet_HierarchicalVolumetricBranchDecomposer_h

#include <iomanip>
#include <string>

// Contour tree includes, not yet moved into new filter structure
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/PrintGraph.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindRegularByGlobal.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindSuperArcBetweenNodes.h>

// Worklets
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/BranchEndComparator.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/CollapseBranchesPointerDoublingWorklet.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/CollapseBranchesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/GetOuterEndWorklet.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/LocalBestUpDownByVolumeBestUpDownEdgeWorklet.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/LocalBestUpDownByVolumeInitSuperarcListWorklet.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/LocalBestUpDownByVolumeWorklet.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/SuperArcVolumetricComparatorIndirectGlobalIdComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/NotNoSuchElementPredicate.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

#ifdef DEBUG_PRINT
#define DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
#endif

namespace viskores
{
namespace filter
{
namespace scalar_topology
{

/// Facture class for augmenting the hierarchical contour tree to enable computations of measures, e.g., volumne
class HierarchicalVolumetricBranchDecomposer
{ // class HierarchicalVolumetricBranchDecomposer
public:
  /// we will want arrays for swapping with our partners, holding the best up/down superarc & the corresponding volume
  /// the best up/down will be in local supernode IDs initially, but during the swap will need to be global node IDs
  viskores::worklet::contourtree_augmented::IdArrayType BestUpSupernode;
  viskores::worklet::contourtree_augmented::IdArrayType BestDownSupernode;
  viskores::worklet::contourtree_augmented::IdArrayType BestUpVolume;
  viskores::worklet::contourtree_augmented::IdArrayType BestDownVolume;

  /// working arrays - kept at class level to simplify debug print
  viskores::worklet::contourtree_augmented::IdArrayType UpVolume;
  viskores::worklet::contourtree_augmented::IdArrayType DownVolume;


  /// working arrays for collecting ends of branches
  /// kept at class level for branch aggregation
  /// Note: Intrinsic Volume and Dependent Volume are only for the superarcs at the end
  viskores::worklet::contourtree_augmented::IdArrayType BranchRoot;
  viskores::worklet::contourtree_augmented::IdArrayType BranchRootGRId;
  viskores::worklet::contourtree_augmented::IdArrayType UpperEndGRId;
  viskores::worklet::contourtree_augmented::IdArrayType LowerEndGRId;
  viskores::cont::UnknownArrayHandle UpperEndValue;
  viskores::cont::UnknownArrayHandle LowerEndValue;
  viskores::worklet::contourtree_augmented::IdArrayType UpperEndSuperarcId;
  viskores::worklet::contourtree_augmented::IdArrayType LowerEndSuperarcId;
  viskores::worklet::contourtree_augmented::IdArrayType UpperEndIntrinsicVolume;
  viskores::worklet::contourtree_augmented::IdArrayType LowerEndIntrinsicVolume;
  viskores::worklet::contourtree_augmented::IdArrayType UpperEndDependentVolume;
  viskores::worklet::contourtree_augmented::IdArrayType LowerEndDependentVolume;
  // This information is only used when extracting isosurfaces
  // We need the upper and lower end within the block to determine the superarc containing the isovalue
  // The information should NOT be exchanged between blocks, since it's local id
  viskores::worklet::contourtree_augmented::IdArrayType UpperEndLocalId;
  viskores::worklet::contourtree_augmented::IdArrayType LowerEndLocalId;

  /// routines to compute branch decomposition by volume
  /// WARNING: we now have two types of hierarchical tree sharing a data structure:
  ///   I.      hierarchical tree without augmentation
  ///   II.     hierarchical tree with augmentation
  /// We only expect to call this for II, but it's wiser to make sure that it computes for I as well.
  /// Also, this code is substantially identical to ContourTreeMaker::ComputeVolumeBranchDecomposition()
  /// except for:
  ///   A.      it has to deal with the round/iteration paradigm of hierarchical trees, and
  ///   B.      Stages III-IV in particular are modified
  ///   C.      Several stages involve fan-ins
  /// The principal reason for the modifications in B. is that the old code collapses branches to their maximum
  /// which is often a leaf. In the hierarchical version, the leaf will often not be represented on all ranks, so
  /// we modify it to collapse towards the "most senior".  This will be easiest if we collapse by superarc IDs instead of supernode IDs
  /// For C., we have to break the code into separate routines so that the fan-in MPI can be outside this unit.
  ///
  /// WARNING! WARNING! WARNING!
  /// In the non-hierarchical version, the last (virtual root) superarc goes from the highest ID supernode to NO_SUCH_ELEMENT
  /// If it was included in the sorts, this could cause problems
  /// The (simple) way out of this was to set nSuperarcs = nSupernodes - 1 when copying our temporary list of superarcs
  /// that way we don't use it at all.
  /// In the hierarchical version, this no longer works, because attachment points may also have virtual superarcs
  /// So we either need to compress them out (an extra log step) or ignore them in the later loop.
  /// Of the two, compressing them out is safer
  ///
  /// routine that determines the best upwards/downwards edges at each vertex
  /// Unlike the local version, the best might only be stored on another rank
  /// so we will compute the locally best up or down, then swap until all ranks choose the same best
  void LocalBestUpDownByVolume(const viskores::cont::DataSet& hierarchicalTreeDataSet,
                               const viskores::cont::ArrayHandle<viskores::Id>& intrinsicValues,
                               const viskores::cont::ArrayHandle<viskores::Id>& dependentValues,
                               viskores::Id totalVolume);

  /// routine to compute the local set of superarcs that root at a given one
  void CollapseBranches(const viskores::cont::DataSet& hierarchicalTreeDataSet,
                        viskores::worklet::contourtree_augmented::IdArrayType& branchRoot);

  /// routine to find the upper node and the lower node of all branches within the local block
  template <bool isLower>
  void CollectEndsOfBranches(const viskores::cont::DataSet& hierarchicalTreeDataSet,
                             viskores::worklet::contourtree_augmented::IdArrayType& branchRoot);

  /// routine to compress out duplicate branch IDs from the results of CollectEndsOfBranches
  /// has to call CollectEndsOfBranches for both upper end and lower end
  void CollectBranches(const viskores::cont::DataSet& hierarchicalTreeDataSet,
                       viskores::worklet::contourtree_augmented::IdArrayType& branchRoot);

  /// routines to print branches
  template <typename IdArrayHandleType, typename DataValueArrayHandleType>
  static std::string PrintBranches(const IdArrayHandleType& hierarchicalTreeSuperarcsAH,
                                   const IdArrayHandleType& hierarchicalTreeSupernodesAH,
                                   const IdArrayHandleType& hierarchicalTreeRegularNodeGlobalIdsAH,
                                   const DataValueArrayHandleType& hierarchicalTreeDataValuesAH,
                                   const IdArrayHandleType& branchRootAH);
  static std::string PrintBranches(const viskores::cont::DataSet& ds);

  /// debug routine
  std::string DebugPrint(std::string message, const char* fileName, long lineNum);

private:
  /// Used internally to Invoke worklets
  viskores::cont::Invoker Invoke;

}; // class HierarchicalVolumetricBranchDecomposer


inline void HierarchicalVolumetricBranchDecomposer::LocalBestUpDownByVolume(
  const viskores::cont::DataSet& hierarchicalTreeDataSet,
  const viskores::cont::ArrayHandle<viskores::Id>& intrinsicValues,
  const viskores::cont::ArrayHandle<viskores::Id>& dependentValues,
  viskores::Id totalVolume)
{
  // Get required arrays for hierarchical tree form data set
  auto hierarchicalTreeSupernodes = hierarchicalTreeDataSet.GetField("Supernodes")
                                      .GetData()
                                      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto hierarchicalTreeSuperarcs = hierarchicalTreeDataSet.GetField("Superarcs")
                                     .GetData()
                                     .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto hierarchicalTreeRegularNodeGlobalIds =
    hierarchicalTreeDataSet.GetField("RegularNodeGlobalIds")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  // LocalBestUpDownByVolume
  // STAGE I:   Allocate memory for our arrays
  viskores::Id nSupernodes = hierarchicalTreeSupernodes.GetNumberOfValues();
  // WARNING: This differs from the non-hierarchical version by using the full size *WITH* virtual superarcs
  viskores::Id nSuperarcs = hierarchicalTreeSuperarcs.GetNumberOfValues();

  // set up a list of superarcs as Edges for reference in our comparator
  viskores::worklet::contourtree_augmented::EdgePairArray superarcList;
  superarcList.Allocate(nSuperarcs);
  this->Invoke(viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
                 LocalBestUpDownByVolumeInitSuperarcListWorklet{}, // the worklet
               hierarchicalTreeSuperarcs,                          // input
               superarcList                                        // output
  );

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  {
    std::stringstream resultStream;
    viskores::worklet::contourtree_augmented::PrintHeader(superarcList.GetNumberOfValues(),
                                                          resultStream);
    viskores::worklet::contourtree_augmented::PrintEdgePairArray(
      "Superarc List", superarcList, -1, resultStream);
    resultStream << std::endl;
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
  }
#endif

  // create a list of the non-virtual superarcs
  viskores::worklet::contourtree_augmented::IdArrayType actualSuperarcs;
  // and fill it up with index values [0, 1, 2 ... nSuperarcs-1] while simultaneously stream compacting the
  // values by keeping only those indices where the hierarchicalTree->Superarcs is not NoSuchElement.
  viskores::cont::Algorithm::CopyIf(
    viskores::cont::ArrayHandleIndex(nSuperarcs), //input
    hierarchicalTreeSuperarcs,                    // stencil
    actualSuperarcs,                              // output target array
    viskores::worklet::contourtree_augmented::NotNoSuchElementPredicate{});
  // NOTE: The behavior here is slightly different from the original implementation, as the original code
  //       here does not resize actualSuperarcs but keeps it at the full length of nSuperacs and instead
  //       relies on the nActualSuperarcs parameter. However, the extra values are never used, so compacting
  //       the array here should be fine.
  viskores::Id nActualSuperarcs = actualSuperarcs.GetNumberOfValues();

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  {
    std::stringstream resultStream;
    viskores::worklet::contourtree_augmented::PrintHeader(nActualSuperarcs, resultStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Actual Superarcs", actualSuperarcs, -1, resultStream);
    resultStream << std::endl;
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
  }
#endif

  // and set up arrays for the best upwards, downwards superarcs at each supernode
  // initialize everything to NO_SUCH_ELEMENT for safety (we will test against this, so it's necessary)
  // Set up temporary constant arrays for each group of arrays and initalize the arrays
  // Initalize the arrays
  using viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
  this->UpVolume.AllocateAndFill(nSuperarcs, 0);
  this->DownVolume.AllocateAndFill(nSuperarcs, 0);
  this->BestUpSupernode.AllocateAndFill(nSupernodes, NO_SUCH_ELEMENT);
  this->BestDownSupernode.AllocateAndFill(nSupernodes, NO_SUCH_ELEMENT);
  this->BestUpVolume.AllocateAndFill(nSupernodes, 0);
  this->BestDownVolume.AllocateAndFill(nSupernodes, 0);

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint("Arrays Allocated", __FILE__, __LINE__));
#endif

  // STAGE II: Pick the best (largest volume) edge upwards and downwards
  // II A. Compute the up / down volumes for indirect sorting
  // this is the same in spirit as ContourTreeMaker::ComputeVolumeBranchDecomposition() STAGE II A.
  // given that we have already suppressed the non-virtual superarcs
  // however, in this case, we need to use the actualSuperarcs array instead of the main array
  {
    viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      LocalBestUpDownByVolumeBestUpDownEdgeWorklet bestUpDownEdgeWorklet(totalVolume);
    // permut input and output arrays here so we can use FieldIn and FieldOut to
    // avoid the use of WholeArray access in the worklet
    auto permutedHierarchicalTreeSuperarcs = viskores::cont::make_ArrayHandlePermutation(
      actualSuperarcs, hierarchicalTreeSuperarcs); // input
    auto permutedDependetValues =
      viskores::cont::make_ArrayHandlePermutation(actualSuperarcs, dependentValues); // input
    auto permutedIntrinsicValues =
      viskores::cont::make_ArrayHandlePermutation(actualSuperarcs, intrinsicValues); // input
    auto permutedUpVolume =
      viskores::cont::make_ArrayHandlePermutation(actualSuperarcs, this->UpVolume); // output
    auto permitedDownVolume =
      viskores::cont::make_ArrayHandlePermutation(actualSuperarcs, this->DownVolume); // outout

    this->Invoke(bestUpDownEdgeWorklet,             // the worklet
                 permutedHierarchicalTreeSuperarcs, // input
                 permutedDependetValues,            // input
                 permutedIntrinsicValues,           // input
                 permutedUpVolume,                  // output
                 permitedDownVolume                 // outout
    );
  }

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint("Volume Arrays Set Up", __FILE__, __LINE__));
  {
    std::stringstream resultStream;
    viskores::worklet::contourtree_augmented::PrintHeader(superarcList.GetNumberOfValues(),
                                                          resultStream);
    viskores::worklet::contourtree_augmented::PrintEdgePairArray(
      "Superarc List", superarcList, -1, resultStream);
    resultStream << std::endl;
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
  }
#endif
  // II B. Pick the best downwards volume by sorting on upper vertex then processing by segments (segmented by vertex)
  // II B 1.    Sort the superarcs by upper vertex
  // NB:  We reuse the actual superarcs list here - this works because we have indexed the volumes on the underlying superarc ID
  // NB 2: Notice that we only sort the "actual" ones - this is to avoid unnecessary resize() calls in viskores later on
  {
    viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      SuperArcVolumetricComparatorIndirectGlobalIdComparator
        SuperArcVolumetricComparatorIndirectGlobalIdComparator(
          this->UpVolume, superarcList, hierarchicalTreeRegularNodeGlobalIds, false);
    viskores::cont::Algorithm::Sort(actualSuperarcs,
                                    SuperArcVolumetricComparatorIndirectGlobalIdComparator);
  }

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  {
    std::stringstream resultStream;
    resultStream
      << "Actual Superarc List After Sorting By High End (Full Array, including ignored elements)"
      << std::endl;
    viskores::worklet::contourtree_augmented::PrintHeader(nActualSuperarcs, resultStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Actual Superarcs", actualSuperarcs, -1, resultStream);
    resultStream << std::endl;
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
  }
#endif
  // II B 2.  Per vertex, best superarc writes to the best downward array
  {
    auto permutedUpVolume =
      viskores::cont::make_ArrayHandlePermutation(actualSuperarcs, this->UpVolume);
    this->Invoke(viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
                   LocalBestUpDownByVolumeWorklet<true>{ nActualSuperarcs },
                 actualSuperarcs,                      // input
                 superarcList,                         // input
                 permutedUpVolume,                     // input
                 hierarchicalTreeRegularNodeGlobalIds, // input
                 hierarchicalTreeSupernodes,           // input
                 this->BestDownSupernode,              // output
                 this->BestDownVolume                  // output
    );
  }
#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint("BestDownSupernode Written", __FILE__, __LINE__));
#endif

  // II B 3.  Repeat for lower vertex
  {
    viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      SuperArcVolumetricComparatorIndirectGlobalIdComparator
        SuperArcVolumetricComparatorIndirectGlobalIdComparator(
          this->DownVolume, superarcList, hierarchicalTreeRegularNodeGlobalIds, true);
    viskores::cont::Algorithm::Sort(actualSuperarcs,
                                    SuperArcVolumetricComparatorIndirectGlobalIdComparator);
  }

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  {
    std::stringstream resultStream;
    resultStream
      << "Actual Superarc List After Sorting By Low End (Full Array, including ignored elements)"
      << std::endl;
    viskores::worklet::contourtree_augmented::PrintHeader(nActualSuperarcs, resultStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Actual Superarcs", actualSuperarcs, -1, resultStream);
    resultStream << std::endl;
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
  }
#endif

  // II B 2.  Per vertex, best superarc writes to the best upward array
  {
    auto permutedDownVolume =
      viskores::cont::make_ArrayHandlePermutation(actualSuperarcs, this->DownVolume);
    this->Invoke(viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
                   LocalBestUpDownByVolumeWorklet<false>{ nActualSuperarcs },
                 actualSuperarcs,                      // input
                 superarcList,                         // input
                 permutedDownVolume,                   // input
                 hierarchicalTreeRegularNodeGlobalIds, // input
                 hierarchicalTreeSupernodes,           // input
                 this->BestUpSupernode,                // output
                 this->BestUpVolume                    // output
    );
  }

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint("Local Best Up/Down Computed", __FILE__, __LINE__));
#endif
} // LocalBestUpDownByVolume


inline void HierarchicalVolumetricBranchDecomposer::CollapseBranches(
  const viskores::cont::DataSet& hierarchicalTreeDataSet,
  viskores::worklet::contourtree_augmented::IdArrayType& branchRoot)
{ // CollapseBranches
  // Get required arrays for hierarchical tree form data set
  auto hierarchicalTreeSupernodes = hierarchicalTreeDataSet.GetField("Supernodes")
                                      .GetData()
                                      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto hierarchicalTreeSuperarcs = hierarchicalTreeDataSet.GetField("Superarcs")
                                     .GetData()
                                     .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto hierarchicalTreeRegularNodeGlobalIds =
    hierarchicalTreeDataSet.GetField("RegularNodeGlobalIds")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto hierarchicalTreeRegularNodeSortOrder =
    hierarchicalTreeDataSet.GetField("RegularNodeSortOrder")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto hierarchicalTreeRegular2Supernode =
    hierarchicalTreeDataSet.GetField("Regular2Supernode")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto hierarchicalTreeWhichRound = hierarchicalTreeDataSet.GetField("WhichRound")
                                      .GetData()
                                      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  // initialise the superarcs to be their own branch roots
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(branchRoot.GetNumberOfValues()),
                            branchRoot);

  //    For each supernode, convert the best up into a superarc ID
  {
    viskores::worklet::contourtree_distributed::FindRegularByGlobal findRegularByGlobal{
      hierarchicalTreeRegularNodeSortOrder, hierarchicalTreeRegularNodeGlobalIds
    };
    viskores::worklet::contourtree_distributed::FindSuperArcBetweenNodes findSuperArcBetweenNodes{
      hierarchicalTreeSuperarcs
    };
    // Get the number of rounds
    auto numRoundsArray = hierarchicalTreeDataSet.GetField("NumRounds")
                            .GetData()
                            .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
    viskores::Id numRounds = viskores::cont::ArrayGetValue(0, numRoundsArray);

    using viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      CollapseBranchesWorklet;
    CollapseBranchesWorklet collapseBranchesWorklet(numRounds);
    this->Invoke(collapseBranchesWorklet,           // the worklet
                 this->BestUpSupernode,             // input
                 this->BestDownSupernode,           // input
                 hierarchicalTreeSuperarcs,         // input
                 findRegularByGlobal,               // input ExecutionObject
                 findSuperArcBetweenNodes,          // input ExecutionObject
                 hierarchicalTreeRegular2Supernode, // input
                 hierarchicalTreeWhichRound,        // input
                 branchRoot);
  }

  // OK.  We've now initialized it, and can use pointer-doubling
  // Compute the number of log steps required
  viskores::Id nLogSteps = 1;
  for (viskores::Id shifter = branchRoot.GetNumberOfValues(); shifter != 0; shifter >>= 1)
  {
    nLogSteps++;
  }

  // loop that many times, pointer-doubling
  for (viskores::Id iteration = 0; iteration < nLogSteps; iteration++)
  { // per iteration
    // loop through the vertices, updating
    using viskores::filter::scalar_topology::hierarchical_volumetric_branch_decomposer::
      CollapseBranchesPointerDoublingWorklet;
    this->Invoke(CollapseBranchesPointerDoublingWorklet{}, branchRoot);
  } // per iteration
} // CollapseBranches


//// CollectEndsOfBranches
//// Find ends of branches locally
//// STEP 1A: Find upper end of branch locally
//// Segmented sort by branch ID of value of upper node of superarc
//// Sort superarcs by value value of upper node, segmenting by branchID
//// Upper node determined using ascending flag of superarc array
//// NOTE: Superarc array is stored in HierarchicalTreeDataSet
//// if ascending flag is NOT set, upper node is the source node of the superarc, whose
//// supernode ID is guaranteed to be the same as the ID of the superarc
//// if ascending flag is set, upper node is the target node of the superarc, which is stored
//// in the superarc array but maskIndex must be called to strip out flags
//// Create index array with IDs of all superarcs:
////    * Size is Supernodes.Size()-1 or Superarcs.Size()-1 because of last node as NULL superarc
////    * Fill viskores equivalent of std::iota
//// Segmented sort of the "superarcs" array sort by three keys:
////    (1) branchID (most senior superarc),
////    (2) data value
////    (3) global regular id (for simulation of simplicity)
//// Find highest vertex for branch (i.e., before branchID increases), special case for
//// end of array.
//// STEP 1B: Find lower end of branch locally
////    Inverse to STEP 1A
////
//// isLower: True if we look for the lower end of branches
template <bool isLower>
inline void HierarchicalVolumetricBranchDecomposer::CollectEndsOfBranches(
  const viskores::cont::DataSet& hierarchicalTreeDataSet,
  viskores::worklet::contourtree_augmented::IdArrayType& branchRoots)
{
  using IdArrayType = viskores::worklet::contourtree_augmented::IdArrayType;
  // Array superNodes stores the LOCAL regular ID of the superarc (supernode) to locate the data value
  // size: nSuperarcs
  auto supernodes =
    hierarchicalTreeDataSet.GetField("Supernodes").GetData().AsArrayHandle<IdArrayType>();

  // Array superArcs stores the target supernode of the superarc
  // size: nSuperarcs
  // NOTE: NSE referring to the innermost node. We will filter this node later.
  auto superarcs =
    hierarchicalTreeDataSet.GetField("Superarcs").GetData().AsArrayHandle<IdArrayType>();
  viskores::Id nSuperarcs = superarcs.GetNumberOfValues();

  // data value in UnknownArrayHandle
  // size: nVertices
  auto dataValues = hierarchicalTreeDataSet.GetField("DataValues").GetData();

  // global regular IDs are used for simulation of simplicity to break ties
  // size: nVertices
  auto globalRegularIds =
    hierarchicalTreeDataSet.GetField("RegularNodeGlobalIds").GetData().AsArrayHandle<IdArrayType>();

  auto intrinsicVolumes =
    hierarchicalTreeDataSet.GetField("IntrinsicVolume").GetData().AsArrayHandle<IdArrayType>();

  auto dependentVolumes =
    hierarchicalTreeDataSet.GetField("DependentVolume").GetData().AsArrayHandle<IdArrayType>();

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  if (isLower)
  {
    IdArrayType superarcGRId;
    viskores::cont::make_ArrayHandlePermutation(supernodes, globalRegularIds);
    viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
      globalRegularIds, supernodes, superarcGRId);

    std::stringstream resultStream;
    resultStream << "All Information In The Block" << std::endl;
    viskores::worklet::contourtree_augmented::PrintHeader(nSuperarcs, resultStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Superarcs", superarcs, -1, resultStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Supernodes", supernodes, -1, resultStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Regular IDs", superarcGRId, -1, resultStream);

    auto resolveOutput = [&](const auto& inArray)
    {
      using InArrayHandleType = std::decay_t<decltype(inArray)>;
      using ValueType = typename InArrayHandleType::ValueType;
      viskores::cont::ArrayHandle<ValueType> superarcValue;
      viskores::worklet::contourtree_augmented::PermuteArrayWithRawIndex<InArrayHandleType>(
        inArray, supernodes, superarcValue);
      viskores::worklet::contourtree_augmented::PrintValues(
        "Data Values", superarcValue, -1, resultStream);
    };
    dataValues.CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(
      resolveOutput);

    viskores::worklet::contourtree_augmented::PrintIndices(
      "Intrinsic Volumes", intrinsicVolumes, -1, resultStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Dependent Volumes", dependentVolumes, -1, resultStream);

    VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
  }
#endif

  // Get the outer end of all superarcs
  // Pseudo-code of the worklet GetSuperarcOuterNodeWorklet in serial:
  // ** Initialize array for outerNodes and its portal **
  // for (viskores::Id i=0; i<nSuperarcs; i++){
  //   if (NoSuchElement(superarcs[i]){
  //     outNodes[i] = NO_SUCH_ELEMENT;
  //     continue;
  //   }
  //
  //   bool ascendingSuperarc = IsAscending(superarcs[i]);
  //   if (ascendingSuperarc ^ isLower){
  //     viskores::Id superarcTo = MaskedIndex(superarcs[i]);
  //     outerNodes[i] = superarcTo;
  //   }
  //   else{
  //     outerNodes[i] = i;
  //   }
  // }
  // Other masked arrays: Hyperarcs, (Superarcs), Arcs, Hyperparents, Superparents
  // Rule of thumb:
  // 1. any arc/parent arrays can have ascending flag information
  // 2. always assume flag information on everything except proved otherwise
  // NOTE: NSE is always a flag on everything
  viskores::cont::ArrayHandleIndex superarcIndices(nSuperarcs);

  IdArrayType outerNodes;
  outerNodes.Allocate(nSuperarcs);

  // boolean parameter determines whether looking for the lower end of the superarc or not
  viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
    GetSuperarcOuterNodeWorklet<isLower>
      getSuperarcOuterNodeWorklet;
  viskores::cont::Invoker invoke;
  invoke(getSuperarcOuterNodeWorklet, // worklet
         superarcIndices,             // input
         superarcs,                   // input
         outerNodes);                 // output

  // create a list of the non-virtual superarcs (all superarcs except the most senior one)
  IdArrayType actualSuperarcs;
  // fill it up with index values [0, 1, 2 ... nSuperarcs-1]
  // while keeping only those indices where the Superarcs is not NSE.
  viskores::cont::Algorithm::CopyIf(
    superarcIndices, // input
    superarcs,       // stencil
    actualSuperarcs, // output target array
    viskores::worklet::contourtree_augmented::NotNoSuchElementPredicate{});
  viskores::Id nActualSuperarcs = actualSuperarcs.GetNumberOfValues();

  // Get the branch Id, data value and global regular ID for each actual superarc to be sorted
  // P.S. the data value and the regular ID of OUTER nodes of the superarc
  // Pseudo-code in serial (no explicit flag removal process):
  // for (viskores::Id i=0; i<nActualSuperarcs; i++)
  // {
  //   actualBranchRoots[i] = branchRoots[actualSuperarcs[i]];
  //   actualOuterNodeValues[i] = dataValues[supernodes[outerNodes[actualSuperarcs[i]]]];
  //   actualOuterNodeRegularIds[i] = globalRegularIds[supernodes[outerNodes[actualSuperarcs[i]]]];
  // }
  // Solution: PermuteArrayWithMaskedIndex helps allocate the space so no need for explicit allocation
  // PermuteArrayWithMaskedIndex also calls MaskedIndex

  // IdArrayType, size: nActualSuperarcs
  IdArrayType actualBranchRoots;
  viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
    branchRoots, actualSuperarcs, actualBranchRoots);

  // IdArrayType, size: nActualSuperarcs
  IdArrayType actualOuterNodes;
  viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
    outerNodes, actualSuperarcs, actualOuterNodes);

  // IdArrayType, size: nActualSuperarcs
  IdArrayType actualOuterNodeLocalIds;
  viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
    supernodes, actualOuterNodes, actualOuterNodeLocalIds);

  // IdArrayType, size: nActualSuperarcs
  IdArrayType actualOuterNodeRegularIds;
  viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
    globalRegularIds, actualOuterNodeLocalIds, actualOuterNodeRegularIds);

  auto resolveArray = [&](const auto& inArray)
  {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    using ValueType = typename InArrayHandleType::ValueType;

    // Sort all superarcs based on the key in order
    //      (1) branchID (most senior superarc),
    //      (2) data value
    //      (3) global regular id (for simulation of simplicity)

    // ValueArrayType
    auto actualOuterNodeValues =
      viskores::cont::make_ArrayHandlePermutation(actualOuterNodeLocalIds, inArray);

    viskores::cont::ArrayHandleIndex actualSuperarcsIdx(nActualSuperarcs);
    // IdArrayType, size: nActualSuperarcs, value range: [0, nActualSuperarcs-1]
    // This array is ONLY used for sorting
    // NOTE: To be distinguished from actualSuperarcs, whose value range is [0, nSuperarcs-1]
    IdArrayType sortedSuperarcs;
    viskores::cont::Algorithm::Copy(actualSuperarcsIdx, sortedSuperarcs);

    viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      BranchEndComparator<ValueType, isLower>
        branchEndComparator(actualBranchRoots, actualOuterNodeValues, actualOuterNodeRegularIds);
    viskores::cont::Algorithm::Sort(sortedSuperarcs, branchEndComparator);

    /// Permute the branch roots and global regular IDs based on the sorted order
    /// Then segment selection: pick the last element for each consecutive segment of BranchRoots
    /// Solution: mark the last element as 1 in a 01 array

    // This is the real superarc local ID after permutation
    IdArrayType permutedActualSuperarcs;
    viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id>(
      actualSuperarcs, sortedSuperarcs, permutedActualSuperarcs);

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
    {
      std::stringstream resultStream;
      resultStream << "Sorted Actual Superarcs" << std::endl;
      viskores::worklet::contourtree_augmented::PrintHeader(nActualSuperarcs, resultStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "actualSortSuperarcs", sortedSuperarcs, -1, resultStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "actualSuperarcs", permutedActualSuperarcs, -1, resultStream);
      resultStream << std::endl;
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
    }
#endif

    // NOTE: permutedSuperarcsTo stores the superarcTo information.
    // It should only be used to determine the direction of the superarc
    auto permutedSuperarcsTo =
      viskores::cont::make_ArrayHandlePermutation(permutedActualSuperarcs, superarcs);

    // The following
    auto permutedBranchRoots =
      viskores::cont::make_ArrayHandlePermutation(sortedSuperarcs, actualBranchRoots);
    auto permutedRegularIds =
      viskores::cont::make_ArrayHandlePermutation(sortedSuperarcs, actualOuterNodeRegularIds);
    auto permutedLocalIds =
      viskores::cont::make_ArrayHandlePermutation(sortedSuperarcs, actualOuterNodeLocalIds);
    auto permutedDataValues =
      viskores::cont::make_ArrayHandlePermutation(sortedSuperarcs, actualOuterNodeValues);
    auto permutedIntrinsicVolumes =
      viskores::cont::make_ArrayHandlePermutation(permutedActualSuperarcs, intrinsicVolumes);
    auto permutedDependentVolumes =
      viskores::cont::make_ArrayHandlePermutation(permutedActualSuperarcs, dependentVolumes);

    viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      OneIfBranchEndWorklet oneIfBranchEndWorklet;
    IdArrayType oneIfBranchEnd;
    oneIfBranchEnd.Allocate(nActualSuperarcs);

    invoke(oneIfBranchEndWorklet, // worklet
           actualSuperarcsIdx,    // input
           permutedBranchRoots,   // whole array input, need to check the neighbor information
           oneIfBranchEnd         // output
    );

    IdArrayType actualDirectedSuperarcs;
    actualDirectedSuperarcs.Allocate(nActualSuperarcs);
    viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      CopyArcDirectionWorklet copyArcDirectionWorklet;
    invoke(copyArcDirectionWorklet,
           permutedActualSuperarcs,
           permutedSuperarcsTo,
           actualDirectedSuperarcs);

    // For all branch roots, we need their global regular IDs for communication
    // Pseudo-code:
    // for (viskores::Id i=0; i<nBranches; i++)
    //   branchRootGRIds[i] = globalRegularIds[supernode[permutedBranchRoots[i]]]
    auto branchRootRegIds =
      viskores::cont::make_ArrayHandlePermutation(permutedBranchRoots, supernodes);
    auto branchRootGRIds =
      viskores::cont::make_ArrayHandlePermutation(branchRootRegIds, globalRegularIds);

    // We only keep the end of the branch in the arrays for future process
    // each branch in the block should store exactly one entry
    // We keep the following information
    //   (1) Branch ID (senior-most superarc ID), and its global regular ID
    //   (2) Superarc ID on both ends of the branch
    //   (3) Global regular ID and data value of supernodes at the branch ends
    //   (4) Intrinsic / Dependent volume of superarcs at the branch ends
    if (isLower)
    {
      viskores::cont::Algorithm::CopyIf(permutedBranchRoots, oneIfBranchEnd, this->BranchRoot);
      viskores::cont::Algorithm::CopyIf(branchRootGRIds, oneIfBranchEnd, this->BranchRootGRId);
      viskores::cont::Algorithm::CopyIf(permutedLocalIds, oneIfBranchEnd, this->LowerEndLocalId);
      viskores::cont::Algorithm::CopyIf(
        actualDirectedSuperarcs, oneIfBranchEnd, this->LowerEndSuperarcId);
      viskores::cont::Algorithm::CopyIf(permutedRegularIds, oneIfBranchEnd, this->LowerEndGRId);
      viskores::cont::Algorithm::CopyIf(
        permutedIntrinsicVolumes, oneIfBranchEnd, this->LowerEndIntrinsicVolume);
      viskores::cont::Algorithm::CopyIf(
        permutedDependentVolumes, oneIfBranchEnd, this->LowerEndDependentVolume);
      InArrayHandleType lowerEndValue;
      viskores::cont::Algorithm::CopyIf(permutedDataValues, oneIfBranchEnd, lowerEndValue);
      this->LowerEndValue = lowerEndValue;
    }
    else
    {
      // VERIFICATION: We assume that lower end is computed
      // Go check HierarchicalVolumetricBranchDecomposer::CollectBranches() to see the order
      // We have already got the unique branch ID along with its branch lower end
      // the BranchRoot should be in the same order as UpperBranchRoot
      // Let's do a sanity check here. Would remove this part upon release
      {
        IdArrayType UpperBranchRoot;
        viskores::cont::Algorithm::CopyIf(permutedBranchRoots, oneIfBranchEnd, UpperBranchRoot);
        bool identical =
          this->BranchRoot.GetNumberOfValues() == UpperBranchRoot.GetNumberOfValues();
        if (identical)
        {
          viskores::cont::ArrayHandle<bool> branchRootIdentical;
          viskores::cont::Algorithm::Transform(
            this->BranchRoot, UpperBranchRoot, branchRootIdentical, viskores::Equal());
          identical =
            viskores::cont::Algorithm::Reduce(branchRootIdentical, true, viskores::LogicalAnd());
        }
        if (!identical)
        {
          VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                         "Two reduced BranchRoot arrays are not identical!");
        }
      }
      viskores::cont::Algorithm::CopyIf(branchRootGRIds, oneIfBranchEnd, this->BranchRootGRId);
      viskores::cont::Algorithm::CopyIf(
        actualDirectedSuperarcs, oneIfBranchEnd, this->UpperEndSuperarcId);
      viskores::cont::Algorithm::CopyIf(permutedRegularIds, oneIfBranchEnd, this->UpperEndGRId);
      viskores::cont::Algorithm::CopyIf(permutedLocalIds, oneIfBranchEnd, this->UpperEndLocalId);
      viskores::cont::Algorithm::CopyIf(
        permutedIntrinsicVolumes, oneIfBranchEnd, this->UpperEndIntrinsicVolume);
      viskores::cont::Algorithm::CopyIf(
        permutedDependentVolumes, oneIfBranchEnd, this->UpperEndDependentVolume);
      InArrayHandleType upperEndValue;
      viskores::cont::Algorithm::CopyIf(permutedDataValues, oneIfBranchEnd, upperEndValue);
      this->UpperEndValue = upperEndValue;
    }
  };

  dataValues.CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(
    resolveArray);

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  std::stringstream resultStream;
  const std::string lowerStr = isLower ? "Lower" : "Upper";
  resultStream << "Actual Branches With " << lowerStr << " Ends In The Block" << std::endl;
  const IdArrayType& printBranchEndRegularId = isLower ? this->LowerEndGRId : this->UpperEndGRId;
  const IdArrayType& printBranchEndSuperarcId =
    isLower ? this->LowerEndSuperarcId : this->UpperEndSuperarcId;
  const IdArrayType& printBranchEndIntrinsicVolume =
    isLower ? this->LowerEndIntrinsicVolume : this->UpperEndIntrinsicVolume;
  const IdArrayType& printBranchEndDependentVolume =
    isLower ? this->LowerEndDependentVolume : this->UpperEndDependentVolume;
  const viskores::Id nBranches = this->BranchRoot.GetNumberOfValues();
  viskores::worklet::contourtree_augmented::PrintHeader(nBranches, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BranchRoot", this->BranchRoot, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BranchRootRegularId", this->BranchRootGRId, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BranchEndSuperarcId", printBranchEndSuperarcId, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BranchEndRegularId", printBranchEndRegularId, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BranchEndIntrinsicVolume", printBranchEndIntrinsicVolume, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BranchEndDependentVolume", printBranchEndDependentVolume, -1, resultStream);
  resultStream << std::endl;
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
#endif
} // CollectEndsOfBranches


//// CollectBranches
//// Step 1A + 1B. Call CollectEndsOfBranches to find ends of branches locally
//// STEP 1C: Compress out duplicate branch IDs
////   * Temporary array "knownBranches" with size of superarcs array, initialize to NO_SUCH_ELEMENT
////   * Every highest vertex we find in STEP 1A has a branch ID, use that ID to set knownBranches[bID] = bID;
//// . * Remove/compress out NO_SUCH_ELEMENT entries
//// . * Array now is a list of all known (to the block) branches
//// STEP 2: Look up (and add) global regular ID, value, and terminal volume both intrinsic and dependent
//// Target: get the information to explicitly extract the branch
//// NOTE: Both STEP 1 and STEP 2 are implemented in HierarchicalVolumetricBranchDecomposer::CollectBranches()
inline void HierarchicalVolumetricBranchDecomposer::CollectBranches(
  const viskores::cont::DataSet& hierarchicalTreeDataSet,
  viskores::worklet::contourtree_augmented::IdArrayType& branchRoot)
{
  // The order of these two lines matters
  // check the comment noted "VERIFICATION" above
  // Step 1B + 1C + 2: Collect the lower node of all branches in the block
  this->CollectEndsOfBranches<true>(hierarchicalTreeDataSet, branchRoot);
  // Step 1A + 1C + 2: Collect the upper node of all branches in the block
  this->CollectEndsOfBranches<false>(hierarchicalTreeDataSet, branchRoot);
}


// PrintBranches
// we want to dump out the branches as viewed by this rank.
// most of the processing will be external, so we keep this simple.
// For each end of the superarc, we print out value & global ID prefixed by global ID of the branch root
// The external processing will then sort them to construct segments (as usual) in the array
// then a post-process can find the first and last in each segment & print out the branch
// In order for the sort to work lexicographically, we need to print out in the following order:
//            I       Branch Root Global ID
//            II      Supernode Value
//            III     Supernode Global ID

// Note the following is a template to be called via cast-and-call
template <typename IdArrayHandleType, typename DataValueArrayHandleType>
std::string HierarchicalVolumetricBranchDecomposer::PrintBranches(
  const IdArrayHandleType& hierarchicalTreeSuperarcsAH,
  const IdArrayHandleType& hierarchicalTreeSupernodesAH,
  const IdArrayHandleType& hierarchicalTreeRegularNodeGlobalIdsAH,
  const DataValueArrayHandleType& hierarchicalTreeDataValuesAH,
  const IdArrayHandleType& branchRootAH)
{
  auto hierarchicalTreeSuperarcsPortal = hierarchicalTreeSuperarcsAH.ReadPortal();
  viskores::Id nSuperarcs = hierarchicalTreeSuperarcsAH.GetNumberOfValues();
  auto hierarchicalTreeSupernodesPortal = hierarchicalTreeSupernodesAH.ReadPortal();
  auto hierarchicalTreeRegularNodeGlobalIdsPortal =
    hierarchicalTreeRegularNodeGlobalIdsAH.ReadPortal();
  auto hierarchicalTreeDataValuesPortal = hierarchicalTreeDataValuesAH.ReadPortal();
  auto branchRootPortal = branchRootAH.ReadPortal();

  std::stringstream resultStream;
  // loop through the individual superarcs

  for (viskores::Id superarc = 0; superarc < nSuperarcs; superarc++)
  { // per superarc
    // explicit test for root superarc / attachment points
    if (viskores::worklet::contourtree_augmented::NoSuchElement(
          hierarchicalTreeSuperarcsPortal.Get(superarc)))
    {
      continue;
    }

    // now retrieve the branch root's global ID
    viskores::Id branchRootSuperId = branchRootPortal.Get(superarc);
    viskores::Id branchRootRegularId = hierarchicalTreeSupernodesPortal.Get(branchRootSuperId);
    viskores::Id branchRootGlobalId =
      hierarchicalTreeRegularNodeGlobalIdsPortal.Get(branchRootRegularId);

    // now retrieve the global ID & value for each end & output them
    viskores::Id superFromRegularId = hierarchicalTreeSupernodesPortal.Get(superarc);
    viskores::Id superFromGlobalId =
      hierarchicalTreeRegularNodeGlobalIdsPortal.Get(superFromRegularId);
    typename DataValueArrayHandleType::ValueType superFromValue =
      hierarchicalTreeDataValuesPortal.Get(superFromRegularId);
    resultStream << branchRootGlobalId << "\t" << superFromValue << "\t" << superFromGlobalId
                 << std::endl;

    // now retrieve the global ID & value for each end & output them
    viskores::Id superToRegularId = viskores::worklet::contourtree_augmented::MaskedIndex(
      hierarchicalTreeSuperarcsPortal.Get(superarc));
    viskores::Id superToGlobalId = hierarchicalTreeRegularNodeGlobalIdsPortal.Get(superToRegularId);
    typename DataValueArrayHandleType::ValueType superToValue =
      hierarchicalTreeDataValuesPortal.Get(superToRegularId);
    resultStream << branchRootGlobalId << "\t" << superToValue << "\t" << superToGlobalId
                 << std::endl;
  } // per superarc

  return resultStream.str();
} // PrintBranches

inline std::string HierarchicalVolumetricBranchDecomposer::PrintBranches(
  const viskores::cont::DataSet& ds)
{
  auto hierarchicalTreeSuperarcsAH =
    ds.GetField("Superarcs").GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto hierarchicalTreeSupernodesAH =
    ds.GetField("Supernodes").GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  auto hierarchicalTreeRegularNodeGlobalIdsAH =
    ds.GetField("RegularNodeGlobalIds")
      .GetData()
      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  auto hierarchicalTreeDataValuesData = ds.GetField("DataValues").GetData();

  auto branchRootAH =
    ds.GetField("BranchRoots").GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  std::string result;

  hierarchicalTreeDataValuesData
    .CastAndCallForTypes<TypeListScalarAll, VISKORES_DEFAULT_STORAGE_LIST>(
      [&](const auto& hierarchicalTreeDataValuesAH)
      {
        result = PrintBranches(hierarchicalTreeSuperarcsAH,
                               hierarchicalTreeSupernodesAH,
                               hierarchicalTreeRegularNodeGlobalIdsAH,
                               hierarchicalTreeDataValuesAH,
                               branchRootAH);
      });

  return result;
} // PrintBranches



// debug routine
inline std::string HierarchicalVolumetricBranchDecomposer::DebugPrint(std::string message,
                                                                      const char* fileName,
                                                                      long lineNum)
{ // DebugPrint()
  std::stringstream resultStream;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << std::endl;
  resultStream << std::left << message << std::endl;
  resultStream << "Hypersweep Value Array Contains:        " << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::endl;

  viskores::worklet::contourtree_augmented::PrintHeader(this->UpVolume.GetNumberOfValues(),
                                                        resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Up Volume by SA", this->UpVolume, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Down Volume by SA", this->DownVolume, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Best Down Snode by SN", this->BestDownSupernode, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Best Down Volume by SN", this->BestDownVolume, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Best Up Snode by SN", this->BestUpSupernode, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Best Up Volume by SN", this->BestUpVolume, -1, resultStream);
  std::cout << std::endl;
  return resultStream.str();
} // DebugPrint()

} // namespace scalar_topology
} // namespace filter
} // namespace viskores


#endif
