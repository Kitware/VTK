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
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/filter/scalar_topology/internal/UpdateParentBranchFunctor.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/AssignValueWorklet.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/GetBranchHierarchyWorklet.h>

#include <viskores/Types.h>

#include <iomanip>

#ifdef DEBUG_PRINT
#define DEBUG_PRINT_UPDATE_PARENT_BRANCH_ISOVALUE
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#endif

namespace viskores
{
namespace filter
{
namespace scalar_topology
{
namespace internal
{

void UpdateParentBranchFunctor::operator()(
  SelectTopVolumeBranchesBlock* b,
  const viskoresdiy::ReduceProxy& rp,     // communication proxy
  const viskoresdiy::RegularSwapPartners& // partners of the current block (unused)
) const
{
  // Get our rank and DIY id
  const viskores::Id rank = viskores::cont::EnvironmentTracker::GetCommunicator().rank();
  const auto selfid = rp.gid();

  // Aliases to reduce verbosity
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

      // copy incoming to the block
#ifdef DEBUG_PRINT_UPDATE_PARENT_BRANCH_ISOVALUE
      int incomingGlobalBlockId;
      rp.dequeue(ingid, incomingGlobalBlockId);
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Combining local block " << b->GlobalBlockId << " with incoming block "
                                              << incomingGlobalBlockId);
#endif
      viskores::Id nSelfMaxBranch = b->TopVolumeData.ExtraMaximaBranchOrder.GetNumberOfValues();
      viskores::Id nSelfMinBranch = b->TopVolumeData.ExtraMinimaBranchOrder.GetNumberOfValues();

      // dequeue the data from other blocks.
      // nExtraMaximaBranches (incoming)
      // array of incoming maxima branch order
      // array of incoming maxima branch isovalue
      // array of incoming maxima branch saddle global regular ids
      // nExtraMinimaBranches (incoming)
      // array of incoming minima branch order
      // array of incoming minima branch isovalue
      // array of incoming minima branch saddle global regular ids

      // the dequeue'd nIncomingMaxBranch is an array
      // because viskoresdiy has bugs on communicating single variables
      IdArrayType nIncomingMaxBranchWrapper;
      rp.dequeue(ingid, nIncomingMaxBranchWrapper);
      viskores::Id nIncomingMaxBranch = viskores::cont::ArrayGetValue(0, nIncomingMaxBranchWrapper);

      IdArrayType incomingMaxBranchOrder;
      IdArrayType incomingMaxBranchSaddleGRId;
      viskores::cont::UnknownArrayHandle incomingMaxBranchIsoValue;

      auto resolveMaxArray = [&](auto& inArray)
      {
        using InArrayHandleType = std::decay_t<decltype(inArray)>;
#ifdef DEBUG_PRINT_UPDATE_PARENT_BRANCH_ISOVALUE
        {
          using ValueType = typename InArrayHandleType::ValueType;
          std::stringstream rs;
          viskores::worklet::contourtree_augmented::PrintHeader(nIncomingMaxBranch, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "incomingMaxBranchOrder", incomingMaxBranchOrder, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "incomingMaxBranchVal", incomingMaxBranchIsoValue, -1, rs);

          viskores::worklet::contourtree_augmented::PrintHeader(nSelfMaxBranch, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "selfMaxBranchOrder", b->TopVolumeData.ExtraMaximaBranchOrder, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "selfMaxBranchVal", inArray, -1, rs);
          VISKORES_LOG_S(viskores::cont::LogLevel::Info, rs.str());
        }
#endif
        InArrayHandleType incomingMaxBranchIsoValueCast =
          incomingMaxBranchIsoValue.AsArrayHandle<InArrayHandleType>();

        // We sort both isovalue and saddle global regular IDs by order
        {
          IdArrayType incomingMaxBranchOrderDup;
          // Note: using two SortByKey is not the most efficient way to sort both arrays by branch order.
          // But, the array size should be pretty small, so it should not cause any efficiency issue.
          viskores::cont::Algorithm::Copy(incomingMaxBranchOrder, incomingMaxBranchOrderDup);
          viskores::cont::Algorithm::SortByKey(incomingMaxBranchOrder,
                                               incomingMaxBranchIsoValueCast);
          viskores::cont::Algorithm::SortByKey(incomingMaxBranchOrderDup,
                                               incomingMaxBranchSaddleGRId);
        }

        viskores::worklet::scalar_topology::select_top_volume_branches::UpdateOuterSaddle<true>
          updateValueOnMaxBranch;
        invoke(updateValueOnMaxBranch,
               b->TopVolumeData.ExtraMaximaBranchOrder,
               inArray,
               b->TopVolumeData.ExtraMaximaBranchSaddleGRId,
               incomingMaxBranchOrder,
               incomingMaxBranchIsoValueCast,
               incomingMaxBranchSaddleGRId);

#ifdef DEBUG_PRINT_UPDATE_PARENT_BRANCH_ISOVALUE
        {
          std::stringstream rs;
          rs << "After update, block " << b->LocalBlockNo << std::endl;
          viskores::worklet::contourtree_augmented::PrintHeader(nSelfMaxBranch, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "selfMaxBranchOrder", b->TopVolumeData.ExtraMaximaBranchOrder, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "selfMaxBranchVal", inArray, -1, rs);
          VISKORES_LOG_S(viskores::cont::LogLevel::Info, rs.str());
        }
#endif
      };

      if (nIncomingMaxBranch > 0)
      {
        rp.dequeue(ingid, incomingMaxBranchOrder);
        rp.dequeue(ingid, incomingMaxBranchIsoValue);
        rp.dequeue(ingid, incomingMaxBranchSaddleGRId);
        if (nSelfMaxBranch > 0)
          b->TopVolumeData.ExtraMaximaBranchIsoValue
            .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(
              resolveMaxArray);
      }

      // Apply the same pipeline for branches with minima to extract
      // the dequeue'd nIncomingMinBranch is an array
      // because viskoresdiy has bugs on communicating single variables
      IdArrayType nIncomingMinBranchWrapper;
      rp.dequeue(ingid, nIncomingMinBranchWrapper);
      viskores::Id nIncomingMinBranch = viskores::cont::ArrayGetValue(0, nIncomingMinBranchWrapper);

      IdArrayType incomingMinBranchOrder;
      IdArrayType incomingMinBranchSaddleGRId;
      viskores::cont::UnknownArrayHandle incomingMinBranchIsoValue;

      auto resolveMinArray = [&](auto& inArray)
      {
        using InArrayHandleType = std::decay_t<decltype(inArray)>;
#ifdef DEBUG_PRINT_UPDATE_PARENT_BRANCH_ISOVALUE
        {
          using ValueType = typename InArrayHandleType::ValueType;
          std::stringstream rs;
          viskores::worklet::contourtree_augmented::PrintHeader(nIncomingMinBranch, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "incomingMinBranchOrder", incomingMinBranchOrder, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "incomingMinBranchVal", incomingMinBranchIsoValue, -1, rs);

          viskores::worklet::contourtree_augmented::PrintHeader(nSelfMinBranch, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "selfMinBranchOrder", b->TopVolumeData.ExtraMinimaBranchOrder, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "selfMinBranchVal", inArray, -1, rs);
          VISKORES_LOG_S(viskores::cont::LogLevel::Info, rs.str());
        }
#endif
        InArrayHandleType incomingMinBranchIsoValueCast =
          incomingMinBranchIsoValue.AsArrayHandle<InArrayHandleType>();

        // We sort both isovalue and saddle global regular IDs by order
        {
          IdArrayType incomingMinBranchOrderDup;
          // Note: using two SortByKey is not the most efficient way to sort both arrays by branch order.
          // But, the array size should be pretty small, so it should not cause any efficiency issue.
          viskores::cont::Algorithm::Copy(incomingMinBranchOrder, incomingMinBranchOrderDup);
          viskores::cont::Algorithm::SortByKey(incomingMinBranchOrder,
                                               incomingMinBranchIsoValueCast);
          viskores::cont::Algorithm::SortByKey(incomingMinBranchOrderDup,
                                               incomingMinBranchSaddleGRId);
        }

        viskores::worklet::scalar_topology::select_top_volume_branches::UpdateOuterSaddle<false>
          updateValueOnMinBranch;
        invoke(updateValueOnMinBranch,
               b->TopVolumeData.ExtraMinimaBranchOrder,
               inArray,
               b->TopVolumeData.ExtraMinimaBranchSaddleGRId,
               incomingMinBranchOrder,
               incomingMinBranchIsoValueCast,
               incomingMinBranchSaddleGRId);

#ifdef DEBUG_PRINT_UPDATE_PARENT_BRANCH_ISOVALUE
        {
          std::stringstream rs;
          rs << "After update, block " << b->LocalBlockNo << std::endl;
          viskores::worklet::contourtree_augmented::PrintHeader(nSelfMinBranch, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "selfMinBranchOrder", b->TopVolumeData.ExtraMinimaBranchOrder, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "selfMinBranchVal", inArray, -1, rs);
          VISKORES_LOG_S(viskores::cont::LogLevel::Info, rs.str());
        }
#endif
      };
      if (nIncomingMinBranch > 0)
      {
        rp.dequeue(ingid, incomingMinBranchOrder);
        rp.dequeue(ingid, incomingMinBranchIsoValue);
        rp.dequeue(ingid, incomingMinBranchSaddleGRId);
        if (nSelfMinBranch > 0)
          b->TopVolumeData.ExtraMinimaBranchIsoValue
            .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(
              resolveMinArray);
      }

      IdArrayType incomingBranchRootGRId;
      rp.dequeue(ingid, incomingBranchRootGRId);
      viskores::cont::ArrayHandle<bool> incomingIsParentBranch;
      rp.dequeue(ingid, incomingIsParentBranch);

      VISKORES_ASSERT(incomingBranchRootGRId.GetNumberOfValues() ==
                      incomingIsParentBranch.GetNumberOfValues());

      // one way is to combine the incoming array with local array
      // and compare the adjacent entries

      // Or, we sort the incomingBranchRootGRId and utilize LowerBounds (or BinarySearch)
      viskores::cont::Algorithm::SortByKey(incomingBranchRootGRId, incomingIsParentBranch);

      // search the index of local branches in the incoming array
      IdArrayType tempAlignedIndexFromIncoming;
      viskores::cont::Algorithm::LowerBounds(
        incomingBranchRootGRId, b->TopVolumeData.BranchRootGRId, tempAlignedIndexFromIncoming);

      // permute the information based on the index from the previous step
      IdArrayType projectBranchRootGRId;
      viskores::cont::ArrayHandle<bool> projectIsParentBranch;
      viskores::worklet::contourtree_augmented::PermuteArrayWithRawIndex(
        incomingBranchRootGRId, tempAlignedIndexFromIncoming, projectBranchRootGRId);
      viskores::worklet::contourtree_augmented::PermuteArrayWithRawIndex(
        incomingIsParentBranch, tempAlignedIndexFromIncoming, projectIsParentBranch);

      // to update b->TopVolumeData.IsParentBranch, we need LogicalOr between blocks
      viskores::cont::ArrayHandle<bool> combinedIsParentBranch;
      viskores::cont::Algorithm::Transform(projectIsParentBranch,
                                           b->TopVolumeData.IsParentBranch,
                                           combinedIsParentBranch,
                                           viskores::LogicalOr());

      // it is not guaranteed that a local branch must exist in the incoming branch
      // so we validate it by checking the alignment of branch root global regular IDs after permute
      viskores::cont::ArrayHandle<bool> projectIsCorrect;
      viskores::cont::Algorithm::Transform(projectBranchRootGRId,
                                           b->TopVolumeData.BranchRootGRId,
                                           projectIsCorrect,
                                           viskores::Equal());

      // for validated entries, we update the IsParentBranch information
      viskores::worklet::scalar_topology::select_top_volume_branches::AssignValueWithStencil
        assignIsParent;
      invoke(
        assignIsParent, projectIsCorrect, combinedIsParentBranch, b->TopVolumeData.IsParentBranch);

      // The logging is commented because the size of exchange is limited by K,
      //    the number of top-volume branches, which is usually small
      std::stringstream dataSizeStream;
      // Log the amount of exchanged data
      dataSizeStream << "    " << std::setw(38) << std::left << "Incoming branch size"
                     << ": " << nIncomingMaxBranch + nIncomingMinBranch << std::endl;

      VISKORES_LOG_S(this->TimingsLogLevel,
                     std::endl
                       << "    ---------------- Exchange Parent Branch Step ---------------------"
                       << std::endl
                       << "    Rank    : " << rank << std::endl
                       << "    DIY Id  : " << selfid << std::endl
                       << "    Inc Id  : " << ingid << std::endl
                       << dataSizeStream.str());
    }
  }

  for (int cc = 0; cc < rp.out_link().size(); ++cc)
  {
    auto target = rp.out_link().target(cc);
    if (target.gid != selfid)
    {
#ifdef DEBUG_PRINT_UPDATE_PARENT_BRANCH_ISOVALUE
      rp.enqueue(target, b->GlobalBlockId);
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Block " << b->GlobalBlockId << " enqueue to Block " << target.gid);
#endif
      // We enqueue the array of nExtraMaxBranches instead of the variable itself
      // Because there is a bug when nExtraMaxBranches=0. The dequeue'd value is not 0, but a random number
      // viskoresdiy seems to perform better on enqueue/dequeue with containers than single variables
      viskores::Id nExtraMaxBranches = b->TopVolumeData.ExtraMaximaBranchOrder.GetNumberOfValues();
      rp.enqueue(target, viskores::cont::make_ArrayHandle<viskores::Id>({ nExtraMaxBranches }));

      if (nExtraMaxBranches)
      {
        rp.enqueue(target, b->TopVolumeData.ExtraMaximaBranchOrder);
        rp.enqueue(target, b->TopVolumeData.ExtraMaximaBranchIsoValue);
        rp.enqueue(target, b->TopVolumeData.ExtraMaximaBranchSaddleGRId);
      }

      // We enqueue the array of nExtraMinBranches instead of the variable itself
      // Because there is a bug when nExtraMinBranches=0. The dequeue'd value is not 0, but a random number
      // viskoresdiy seems to perform better on enqueue/dequeue with containers than single variables
      viskores::Id nExtraMinBranches = b->TopVolumeData.ExtraMinimaBranchOrder.GetNumberOfValues();
      rp.enqueue(target, viskores::cont::make_ArrayHandle<viskores::Id>({ nExtraMinBranches }));

      if (nExtraMinBranches)
      {
        rp.enqueue(target, b->TopVolumeData.ExtraMinimaBranchOrder);
        rp.enqueue(target, b->TopVolumeData.ExtraMinimaBranchIsoValue);
        rp.enqueue(target, b->TopVolumeData.ExtraMinimaBranchSaddleGRId);
      }

      // Aside from extra contours, we also need to communicate whether the top-volume branch is a parent branch
      // If inconsistent between blocks, it may lead to chaos in the label of contours
      // BranchGRId is used to locate the shared branches
      rp.enqueue(target, b->TopVolumeData.BranchRootGRId);
      rp.enqueue(target, b->TopVolumeData.IsParentBranch);
    }
  }
}

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace viskores
