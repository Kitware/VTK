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

#ifndef viskores_worklet_contourtree_distributed_combinehypersweepblockfunctor_h
#define viskores_worklet_contourtree_distributed_combinehypersweepblockfunctor_h

#include <viskores/Types.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/HyperSweepBlock.h>

// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on

// #define DEBUG_PRINT_COMBINED_BLOCK_IDS

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{

template <typename ContourTreeDataFieldType>
struct CobmineHyperSweepBlockFunctor
{
  void operator()(
    viskores::worklet::contourtree_distributed::HyperSweepBlock<ContourTreeDataFieldType>* b,
    const viskoresdiy::ReduceProxy& rp,     // communication proxy
    const viskoresdiy::RegularSwapPartners& // partners of the current block (unused)
  ) const
  {
    // Get our rank and DIY id
    //const viskores::Id rank = viskores::cont::EnvironmentTracker::GetCommunicator().rank();
    const auto selfid = rp.gid();

    std::vector<int> incoming;
    rp.incoming(incoming);

    for (const int ingid : incoming)
    {
      auto roundNo = rp.round() - 1; // We are processing incoming data from the previous round

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
                       "Combining local block " << b->GlobalBlockId << " with incomoing block "
                                                << incomingGlobalBlockId);
#endif
        viskores::cont::ArrayHandle<viskores::Id> incomingIntrinsicVolume;
        rp.dequeue(ingid, incomingIntrinsicVolume);
        viskores::cont::ArrayHandle<viskores::Id> incomingDependentVolume;
        rp.dequeue(ingid, incomingDependentVolume);

        viskores::Id numSupernodesToProcess = viskores::cont::ArrayGetValue(
          viskores::Id{ 0 }, b->HierarchicalContourTree.FirstSupernodePerIteration[roundNo]);

        auto intrinsicVolumeView =
          make_ArrayHandleView(b->IntrinsicVolume, 0, numSupernodesToProcess);
        VISKORES_ASSERT(incomingIntrinsicVolume.GetNumberOfValues() ==
                        intrinsicVolumeView.GetNumberOfValues());

        viskores::cont::Algorithm::Transform(
          intrinsicVolumeView, incomingIntrinsicVolume, intrinsicVolumeView, viskores::Sum());

        auto dependentVolumeView =
          make_ArrayHandleView(b->DependentVolume, 0, numSupernodesToProcess);
        VISKORES_ASSERT(incomingDependentVolume.GetNumberOfValues() ==
                        dependentVolumeView.GetNumberOfValues());
        viskores::cont::Algorithm::Transform(
          dependentVolumeView, incomingDependentVolume, dependentVolumeView, viskores::Sum());
      }
    }

    for (int cc = 0; cc < rp.out_link().size(); ++cc)
    {
      auto target = rp.out_link().target(cc);
      if (target.gid != selfid)
      {
#ifdef DEBUG_PRINT_COMBINED_BLOCK_IDS
        rp.enqueue(target, b->GlobalBlockId);
#endif

        // Create views for data we need to send
        viskores::Id numSupernodesToProcess = viskores::cont::ArrayGetValue(
          0, b->HierarchicalContourTree.FirstSupernodePerIteration[rp.round()]);
        auto intrinsicVolumeView =
          make_ArrayHandleView(b->IntrinsicVolume, 0, numSupernodesToProcess);
        auto dependentVolumeView =
          make_ArrayHandleView(b->DependentVolume, 0, numSupernodesToProcess);
        // TODO/FIXME: Currently a copy is required, as ArrayHandleView does not
        // have a serialization function (and even serializing it would not avoid
        // sending portions outside the "view"). At the moment, copying the data
        // inside its view to an extra array seems to be the best approach. Possibly
        // revisit this, if viskores adds additional functions that can help avoiding the
        // extra copy.
        viskores::cont::ArrayHandle<viskores::Id> sendIntrinsicVolume;
        viskores::cont::ArrayCopy(intrinsicVolumeView, sendIntrinsicVolume);
        viskores::cont::ArrayHandle<viskores::Id> sendDependentVolume;
        viskores::cont::ArrayCopy(dependentVolumeView, sendDependentVolume);

        // Send necessary data portions
        rp.enqueue(target, sendIntrinsicVolume);
        rp.enqueue(target, sendDependentVolume);
      }
    }
  }
};

} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
