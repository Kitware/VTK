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

#ifndef viskores_worklet_contourtree_distributed_hierarchicalaugmenterfunctor_h
#define viskores_worklet_contourtree_distributed_hierarchicalaugmenterfunctor_h

#include <viskores/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/DistributedContourTreeBlockData.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/PrintGraph.h>

// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{

/// Functor used by DIY reduce the merge data blocks in parallel
template <typename FieldType>
class HierarchicalAugmenterFunctor
{
public:
  /// Create the functor
  /// @param[in] timingsLogLevel Set the viskores::cont:LogLevel to be used to record timings information
  ///                            specific to the computation of the hierachical contour tree
  HierarchicalAugmenterFunctor(
    viskores::cont::LogLevel timingsLogLevel = viskores::cont::LogLevel::Perf)
    : TimingsLogLevel(timingsLogLevel)
  {
  }


  void operator()(
    viskores::worklet::contourtree_distributed::DistributedContourTreeBlockData<FieldType>*
      blockData,                            // local Block.
    const viskoresdiy::ReduceProxy& rp,     // communication proxy
    const viskoresdiy::RegularSwapPartners& // partners of the current block (unused)
  ) const
  {
    // Track timing of main steps
    viskores::cont::Timer totalTimer; // Total time for each call
    totalTimer.Start();
    viskores::cont::Timer timer; // Time individual steps
    timer.Start();
    std::stringstream timingsStream;

    const viskores::Id rank = viskores::cont::EnvironmentTracker::GetCommunicator().rank();
    auto round = rp.round();
    const auto selfid = rp.gid();

    for (int i = 0; i < rp.in_link().size(); ++i)
    {
      int ingid = rp.in_link().target(i).gid;
      if (ingid != selfid)
      { // Receive and augment
        rp.dequeue(ingid, blockData->HierarchicalAugmenter.InData);

        viskores::Id exchangeSize =
          blockData->HierarchicalAugmenter.InData.Superparents.GetNumberOfValues();
        exchangeSize =
          std::max(exchangeSize,
                   blockData->HierarchicalAugmenter.InData.GlobalRegularIds.GetNumberOfValues());
        timingsStream << "    " << std::setw(38) << std::left << "Retrieved Attachment Points"
                      << ": " << exchangeSize << std::endl;

        blockData->HierarchicalAugmenter.RetrieveInAttachmentPoints();
      }
    }

    // log the time for getting the data from DIY
    timingsStream << "    " << std::setw(38) << std::left << "Retrieve In Attachment Points"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    timer.Start();

    for (int i = 0; i < rp.out_link().size(); ++i)
    {
      auto target = rp.out_link().target(i);
      if (target.gid != selfid)
      { // Send to partner
        blockData->HierarchicalAugmenter.PrepareOutAttachmentPoints(round);
        rp.enqueue(target, blockData->HierarchicalAugmenter.OutData);
        // Note: HierarchicalAugmenter.ReleaseSwapArrays() does not necessarily delete the
        // arrays. Rather, it releases the reference to them. If, for example, the data
        // for HierarchicalAugmenter.OutData is still in flight, the data will continue to
        // exist until it is sent.
        blockData->HierarchicalAugmenter.ReleaseSwapArrays();
      }
    }

    // Log the time for enqueue the data for sending via DIY
    timingsStream << "    " << std::setw(38) << std::left
                  << "Prepare and Enqueue Out Attachment Points"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    // Log the total this functor call step took
    timingsStream << "    " << std::setw(38) << std::left << "Total Time Functor Step"
                  << ": " << totalTimer.GetElapsedTime() << " seconds" << std::endl;
    // Record the times we logged
    VISKORES_LOG_S(
      this->TimingsLogLevel,
      std::endl
        << "    ---------------- Hierarchical Augmenter Functor Step ---------------------"
        << std::endl
        << "    Rank    : " << rank << std::endl
        << "    DIY Id  : " << selfid << std::endl
        << "    Round   : " << rp.round() << std::endl
        << timingsStream.str());
  }

private:
  /// Log level to be used for outputting timing information. Default is viskores::cont::LogLevel::Perf
  viskores::cont::LogLevel TimingsLogLevel = viskores::cont::LogLevel::Perf;
};

} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
