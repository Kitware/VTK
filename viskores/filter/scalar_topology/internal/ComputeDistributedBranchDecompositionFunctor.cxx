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

#include <viskores/filter/scalar_topology/internal/ComputeDistributedBranchDecompositionFunctor.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/FindBestSupernodeWorklet.h>

#include <viskores/Types.h>

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

void ComputeDistributedBranchDecompositionFunctor::operator()(
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
                     "Combining local block " << b->GlobalBlockId << " with incomoing block "
                                              << incomingGlobalBlockId);
#endif

      // Receive data from swap partner
      viskores::cont::ArrayHandle<viskores::Id> incomingBestUpVolume;
      rp.dequeue(ingid, incomingBestUpVolume);
      viskores::cont::ArrayHandle<viskores::Id> incomingBestUpSupernode;
      rp.dequeue(ingid, incomingBestUpSupernode);

      viskores::cont::ArrayHandle<viskores::Id> incomingBestDownVolume;
      rp.dequeue(ingid, incomingBestDownVolume);
      viskores::cont::ArrayHandle<viskores::Id> incomingBestDownSupernode;
      rp.dequeue(ingid, incomingBestDownSupernode);

      std::stringstream dataSizeStream;
      // Log the amount of exchanged data
      dataSizeStream << "    " << std::setw(38) << std::left << "Incoming data size"
                     << ": " << incomingBestUpSupernode.GetNumberOfValues() << std::endl;

      VISKORES_LOG_S(
        this->TimingsLogLevel,
        std::endl
          << "    ---------------- Compute Branch Decomposition Step ---------------------"
          << std::endl
          << "    Rank    : " << rank << std::endl
          << "    DIY Id  : " << selfid << std::endl
          << "    Inc Id  : " << ingid << std::endl
          << dataSizeStream.str());

      viskores::Id prefixLength = b->FirstSupernodePerIteration.ReadPortal().Get(rp.round() - 1)[0];

#ifdef DEBUG_PRINT
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Prefix length is " << prefixLength);
      {
        std::stringstream rs;
        viskores::worklet::contourtree_augmented::PrintHeader(
          incomingBestUpSupernode.GetNumberOfValues(), rs);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "incomingBestUpSupernode", incomingBestUpSupernode, -1, rs);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "incomingBestDownSupernode", incomingBestDownSupernode, -1, rs);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "incomingBestUpVolume", incomingBestUpVolume, -1, rs);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "incomingBestDownVolume", incomingBestDownVolume, -1, rs);
        VISKORES_LOG_S(viskores::cont::LogLevel::Info, rs.str());
      }
#endif
      // NOTE: We are processing input data from the previous round, hence, get
      // first supernide per ieteration from previous round

      // Create 'views' to restrict worklet to relevant portion of arrays
      auto bestUpVolumeView = make_ArrayHandleView(branchDecomposer.BestUpVolume, 0, prefixLength);
      auto bestUpSupernodeView =
        make_ArrayHandleView(branchDecomposer.BestUpSupernode, 0, prefixLength);
      auto bestDownVolumeView =
        make_ArrayHandleView(branchDecomposer.BestDownVolume, 0, prefixLength);
      auto bestDownSupernodeView =
        make_ArrayHandleView(branchDecomposer.BestDownSupernode, 0, prefixLength);

      // Check if swap partner knows a better up /down and update
      viskores::cont::Invoker invoke;
      invoke(viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
               FindBestSupernodeWorklet<true>{},
             incomingBestUpVolume,
             incomingBestUpSupernode,
             bestUpVolumeView,
             bestUpSupernodeView);

      invoke(viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
               FindBestSupernodeWorklet<false>{},
             incomingBestDownVolume,
             incomingBestDownSupernode,
             bestDownVolumeView,
             bestDownSupernodeView);

#ifdef DEBUG_PRINT
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, "After round " << rp.round() - 1);
      {
        std::stringstream rs;
        viskores::worklet::contourtree_augmented::PrintHeader(
          branchDecomposer.BestUpSupernode.GetNumberOfValues(), rs);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "BestUpSupernode", branchDecomposer.BestUpSupernode, -1, rs);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "BestDownSupernode", branchDecomposer.BestDownSupernode, -1, rs);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "BestUpVolume", branchDecomposer.BestUpVolume, -1, rs);
        viskores::worklet::contourtree_augmented::PrintIndices(
          "BestDownVolume", branchDecomposer.BestDownVolume, -1, rs);
        VISKORES_LOG_S(viskores::cont::LogLevel::Info, rs.str());
      }
#endif
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
      // Determine which portion of up/down volume/supernode to send
      viskores::Id prefixLength = b->FirstSupernodePerIteration.ReadPortal().Get(rp.round())[0];

      // Create 'views' to restrict sending to relevant portion of arrays
      auto bestUpVolumeView = make_ArrayHandleView(branchDecomposer.BestUpVolume, 0, prefixLength);
      auto bestUpSupernodeView =
        make_ArrayHandleView(branchDecomposer.BestUpSupernode, 0, prefixLength);
      auto bestDownVolumeView =
        make_ArrayHandleView(branchDecomposer.BestDownVolume, 0, prefixLength);
      auto bestDownSupernodeView =
        make_ArrayHandleView(branchDecomposer.BestDownSupernode, 0, prefixLength);

      // TODO/FIXME: Currently a copy is required, as ArrayHandleView does not
      // have a serialization function (and even serializing it would not avoid
      // sending portions outside the "view"). At the moment, copying the data
      // inside its view to an extra array seems to be the best approach. Possibly
      // revisit this, if viskores adds additional functions that can help avoiding the
      // extra copy.
      viskores::cont::ArrayHandle<viskores::Id> sendBestUpVolume;
      viskores::cont::Algorithm::Copy(bestUpVolumeView, sendBestUpVolume);
      viskores::cont::ArrayHandle<viskores::Id> sendBestUpSupernode;
      viskores::cont::Algorithm::Copy(bestUpSupernodeView, sendBestUpSupernode);
      viskores::cont::ArrayHandle<viskores::Id> sendBestDownVolume;
      viskores::cont::Algorithm::Copy(bestDownVolumeView, sendBestDownVolume);
      viskores::cont::ArrayHandle<viskores::Id> sendBestDownSupernode;
      viskores::cont::Algorithm::Copy(bestDownSupernodeView, sendBestDownSupernode);

      rp.enqueue(target, sendBestUpVolume);
      rp.enqueue(target, sendBestUpSupernode);
      rp.enqueue(target, sendBestDownVolume);
      rp.enqueue(target, sendBestDownSupernode);
    }
  }
}

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace viskores
