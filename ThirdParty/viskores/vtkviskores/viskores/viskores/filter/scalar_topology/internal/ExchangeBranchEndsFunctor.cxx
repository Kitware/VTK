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

#include <viskores/filter/scalar_topology/internal/ExchangeBranchEndsFunctor.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/BranchEndGlobalUpdateWorklet.h>

#include <viskores/Types.h>
#include <viskores/cont/Logging.h>

#ifdef DEBUG_PRINT
#define DEBUG_PRINT_COMBINED_BLOCK_IDS
#endif

namespace viskores
{
namespace filter
{
namespace scalar_topology
{
namespace internal
{

void ExchangeBranchEndsFunctor::operator()(
  BranchDecompositionBlock* b,
  const viskoresdiy::ReduceProxy& rp,     // communication proxy
  const viskoresdiy::RegularSwapPartners& // partners of the current block (unused)
) const
{
  // Get our rank and DIY id
  const viskores::Id rank = viskores::cont::EnvironmentTracker::GetCommunicator().rank();
  const auto selfid = rp.gid();

  // Aliases to reduce verbosity
  auto& branchDecomposer = b->VolumetricBranchDecomposer;
  using IdArrayType = viskores::worklet::contourtree_augmented::IdArrayType;

  viskores::cont::Invoker invoke;

  std::vector<int> incoming;
  rp.incoming(incoming);
  for (const int ingid : incoming)
  {
    // NOTE/IMPORTANT: In each round we should have only one swap partner (despite for-loop here).
    // If that assumption does not hold, it will break things.
    // NOTE/IMPORTANT: This assumption only holds if the number of blocks is a power of two.
    // Otherwise, we may need to process more than one incoming block
    if (ingid != selfid)
    {

#ifdef DEBUG_PRINT_COMBINED_BLOCK_IDS
      int incomingGlobalBlockId;
      rp.dequeue(ingid, incomingGlobalBlockId);
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Combining local block " << b->GlobalBlockId << " with incoming block "
                                              << incomingGlobalBlockId);
#endif

      // Receive data from swap partner
      // IMPORTANT: Needs to be exact same order as enqueue later in code
      IdArrayType incomingBranchRootGRId;
      rp.dequeue(ingid, incomingBranchRootGRId);
      IdArrayType incomingUpperEndGRId;
      rp.dequeue(ingid, incomingUpperEndGRId);
      IdArrayType incomingLowerEndGRId;
      rp.dequeue(ingid, incomingLowerEndGRId);
      viskores::cont::UnknownArrayHandle incomingUpperEndValue;
      rp.dequeue(ingid, incomingUpperEndValue);
      viskores::cont::UnknownArrayHandle incomingLowerEndValue;
      rp.dequeue(ingid, incomingLowerEndValue);
      IdArrayType incomingUpperEndSuperarcId;
      rp.dequeue(ingid, incomingUpperEndSuperarcId);
      IdArrayType incomingLowerEndSuperarcId;
      rp.dequeue(ingid, incomingLowerEndSuperarcId);
      IdArrayType incomingUpperEndIntrinsicVolume;
      rp.dequeue(ingid, incomingUpperEndIntrinsicVolume);
      IdArrayType incomingLowerEndIntrinsicVolume;
      rp.dequeue(ingid, incomingLowerEndIntrinsicVolume);
      IdArrayType incomingUpperEndDependentVolume;
      rp.dequeue(ingid, incomingUpperEndDependentVolume);
      IdArrayType incomingLowerEndDependentVolume;
      rp.dequeue(ingid, incomingLowerEndDependentVolume);

      std::stringstream dataSizeStream;
      // Log the amount of exchanged data
      dataSizeStream << "    " << std::setw(38) << std::left << "Incoming branch size"
                     << ": " << incomingBranchRootGRId.GetNumberOfValues() << std::endl;

      VISKORES_LOG_S(this->TimingsLogLevel,
                     std::endl
                       << "    ---------------- Exchange Branch Ends Step ---------------------"
                       << std::endl
                       << "    Rank    : " << rank << std::endl
                       << "    DIY Id  : " << selfid << std::endl
                       << "    Inc Id  : " << ingid << std::endl
                       << dataSizeStream.str());

      /// Superarc and Branch IDs are given based on the hierarchical level
      /// Shared branches should lie on the smaller ID side of the branch array consecutively
      /// We filter out shared branches first
      /// because we need data to be in the same length to apply worklet
      IdArrayType oneIfSharedBranch;
      viskores::cont::Algorithm::Transform(incomingBranchRootGRId,
                                           branchDecomposer.BranchRootGRId,
                                           oneIfSharedBranch,
                                           viskores::Equal());

      viskores::Id nSharedBranches =
        viskores::cont::Algorithm::Reduce(oneIfSharedBranch, 0, viskores::Sum());

#ifdef DEBUG_PRINT_COMBINED_BLOCK_IDS
      std::stringstream precheckStream;
      viskores::worklet::contourtree_augmented::PrintHeader(
        branchDecomposer.BranchRoot.GetNumberOfValues(), precheckStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "SelfBranchRootGRId", branchDecomposer.BranchRootGRId, -1, precheckStream);
      precheckStream << std::endl;

      viskores::worklet::contourtree_augmented::PrintHeader(
        incomingBranchRootGRId.GetNumberOfValues(), precheckStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "OtherBranchRootGRId", incomingBranchRootGRId, -1, precheckStream);
      precheckStream << std::endl;

      if (nSharedBranches > 0)
      {
        viskores::worklet::contourtree_augmented::PrintHeader(nSharedBranches, precheckStream);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "OneIfSharedBranch", oneIfSharedBranch, -1, precheckStream);
        precheckStream << std::endl;
      }
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, precheckStream.str());
#endif

      // Now apply worklet
      // Input field should be sharedBranchGRId because its size is nSharedBranches
      // Worklet task:
      //   1. decide the shared upper node and lower node
      //   2. update local information if necessary
      viskores::cont::ArrayHandleIndex sharedBranchesIndices(nSharedBranches);
      auto resolveValueType = [&](const auto& inArray)
      {
        using InArrayHandleType = std::decay_t<decltype(inArray)>;
        using ValueType = typename InArrayHandleType::ValueType;

        // Need to cast other data value arrays into known value types
        auto concreteSelfLowerEndValue = viskores::cont::make_ArrayHandleView(
          branchDecomposer.LowerEndValue.AsArrayHandle<InArrayHandleType>(), 0, nSharedBranches);
        auto concreteOtherLowerEndValue = viskores::cont::make_ArrayHandleView(
          incomingLowerEndValue.AsArrayHandle<InArrayHandleType>(), 0, nSharedBranches);

        // We need ArrayHandleView to restrict the array to be the same size as nSharedBranches
        auto selfLowerEndGRId =
          viskores::cont::make_ArrayHandleView(branchDecomposer.LowerEndGRId, 0, nSharedBranches);
        auto otherLowerEndGRId =
          viskores::cont::make_ArrayHandleView(incomingLowerEndGRId, 0, nSharedBranches);
        auto selfLowerEndSuperarcId = viskores::cont::make_ArrayHandleView(
          branchDecomposer.LowerEndSuperarcId, 0, nSharedBranches);
        auto otherLowerEndSuperarcId =
          viskores::cont::make_ArrayHandleView(incomingLowerEndSuperarcId, 0, nSharedBranches);
        auto selfLowerEndIntrinsicVolume = viskores::cont::make_ArrayHandleView(
          branchDecomposer.LowerEndIntrinsicVolume, 0, nSharedBranches);
        auto otherLowerEndIntrinsicVolume =
          viskores::cont::make_ArrayHandleView(incomingLowerEndIntrinsicVolume, 0, nSharedBranches);
        auto selfLowerEndDependentVolume = viskores::cont::make_ArrayHandleView(
          branchDecomposer.LowerEndDependentVolume, 0, nSharedBranches);
        auto otherLowerEndDependentVolume =
          viskores::cont::make_ArrayHandleView(incomingLowerEndDependentVolume, 0, nSharedBranches);

        viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
          UpdateBranchEndByExchangeWorklet<ValueType, true>
            updateLowerEndWorklet;
        invoke(updateLowerEndWorklet,
               sharedBranchesIndices,
               selfLowerEndGRId,
               otherLowerEndGRId,
               concreteSelfLowerEndValue,
               concreteOtherLowerEndValue,
               selfLowerEndSuperarcId,
               otherLowerEndSuperarcId,
               selfLowerEndIntrinsicVolume,
               otherLowerEndIntrinsicVolume,
               selfLowerEndDependentVolume,
               otherLowerEndDependentVolume);

        // write the self lower end value array back to branchDecomposer.LowerEndValue
        // no need to write this explicitly, because they share the same memory

        // For upper end, the branchDecomposer.UpperEndValue is already casted
        // So we can omit the step to cast its type
        auto concreteSelfUpperEndValue =
          viskores::cont::make_ArrayHandleView(inArray, 0, nSharedBranches);
        auto concreteOtherUpperEndValue = viskores::cont::make_ArrayHandleView(
          incomingUpperEndValue.AsArrayHandle<InArrayHandleType>(), 0, nSharedBranches);

        // We need ArrayHandleView to restrict the array to be the same size as nSharedBranches
        auto selfUpperEndGRId =
          viskores::cont::make_ArrayHandleView(branchDecomposer.UpperEndGRId, 0, nSharedBranches);
        auto otherUpperEndGRId =
          viskores::cont::make_ArrayHandleView(incomingUpperEndGRId, 0, nSharedBranches);
        auto selfUpperEndSuperarcId = viskores::cont::make_ArrayHandleView(
          branchDecomposer.UpperEndSuperarcId, 0, nSharedBranches);
        auto otherUpperEndSuperarcId =
          viskores::cont::make_ArrayHandleView(incomingUpperEndSuperarcId, 0, nSharedBranches);
        auto selfUpperEndIntrinsicVolume = viskores::cont::make_ArrayHandleView(
          branchDecomposer.UpperEndIntrinsicVolume, 0, nSharedBranches);
        auto otherUpperEndIntrinsicVolume =
          viskores::cont::make_ArrayHandleView(incomingUpperEndIntrinsicVolume, 0, nSharedBranches);
        auto selfUpperEndDependentVolume = viskores::cont::make_ArrayHandleView(
          branchDecomposer.UpperEndDependentVolume, 0, nSharedBranches);
        auto otherUpperEndDependentVolume =
          viskores::cont::make_ArrayHandleView(incomingUpperEndDependentVolume, 0, nSharedBranches);

        viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
          UpdateBranchEndByExchangeWorklet<ValueType, false>
            updateUpperEndWorklet;
        invoke(updateUpperEndWorklet,
               sharedBranchesIndices,
               selfUpperEndGRId,
               otherUpperEndGRId,
               concreteSelfUpperEndValue,
               concreteOtherUpperEndValue,
               selfUpperEndSuperarcId,
               otherUpperEndSuperarcId,
               selfUpperEndIntrinsicVolume,
               otherUpperEndIntrinsicVolume,
               selfUpperEndDependentVolume,
               otherUpperEndDependentVolume);

#ifdef DEBUG_PRINT_COMBINED_BLOCK_IDS
        std::stringstream resultStream;
        resultStream << "Branches After Combination (nSharedBranches = " << nSharedBranches << ")"
                     << std::endl;
        viskores::worklet::contourtree_augmented::PrintHeader(
          branchDecomposer.BranchRoot.GetNumberOfValues(), resultStream);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "BranchRoot", branchDecomposer.BranchRoot, -1, resultStream);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "BranchRootGRID", branchDecomposer.BranchRootGRId, -1, resultStream);

        viskores::worklet::contourtree_augmented::PrintIndices(
          "UpperEndGRID", branchDecomposer.UpperEndGRId, -1, resultStream);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "UpperEndSuperarcID", branchDecomposer.UpperEndSuperarcId, -1, resultStream);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "UpperEndIntrinsicVolume", branchDecomposer.UpperEndIntrinsicVolume, -1, resultStream);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "UpperEndDependentVolume", branchDecomposer.UpperEndDependentVolume, -1, resultStream);
        viskores::worklet::contourtree_augmented::PrintValues(
          "UpperEndValue", inArray, -1, resultStream);

        viskores::worklet::contourtree_augmented::PrintIndices(
          "LowerEndGRID", branchDecomposer.LowerEndGRId, -1, resultStream);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "LowerEndSuperarcID", branchDecomposer.LowerEndSuperarcId, -1, resultStream);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "LowerEndIntrinsicVolume", branchDecomposer.LowerEndIntrinsicVolume, -1, resultStream);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "LowerEndDependentVolume", branchDecomposer.LowerEndDependentVolume, -1, resultStream);
        viskores::worklet::contourtree_augmented::PrintValues(
          "LowerEndValue",
          branchDecomposer.LowerEndValue.AsArrayHandle<InArrayHandleType>(),
          -1,
          resultStream);
        resultStream << std::endl;
        VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
#endif
      };

      branchDecomposer.UpperEndValue
        .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(
          resolveValueType);
    }
  }

  for (int cc = 0; cc < rp.out_link().size(); ++cc)
  {
    auto target = rp.out_link().target(cc);
    if (target.gid != selfid)
    {
#ifdef DEBUG_PRINT_COMBINED_BLOCK_IDS
      rp.enqueue(target, b->GlobalBlockId);
#endif // DEBUG_PRINT_COMBINED_BLOCK_IDS

      rp.enqueue(target, branchDecomposer.BranchRootGRId);
      rp.enqueue(target, branchDecomposer.UpperEndGRId);
      rp.enqueue(target, branchDecomposer.LowerEndGRId);
      rp.enqueue(target, branchDecomposer.UpperEndValue);
      rp.enqueue(target, branchDecomposer.LowerEndValue);
      rp.enqueue(target, branchDecomposer.UpperEndSuperarcId);
      rp.enqueue(target, branchDecomposer.LowerEndSuperarcId);
      rp.enqueue(target, branchDecomposer.UpperEndIntrinsicVolume);
      rp.enqueue(target, branchDecomposer.LowerEndIntrinsicVolume);
      rp.enqueue(target, branchDecomposer.UpperEndDependentVolume);
      rp.enqueue(target, branchDecomposer.LowerEndDependentVolume);
    }
  }
}

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace viskores
