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
#include <viskores/cont/FieldRangeGlobalCompute.h>

#include <viskores/cont/EnvironmentTracker.h>

#include <viskores/thirdparty/diy/diy.h>

#include <algorithm>
#include <functional>

namespace viskores
{
namespace cont
{

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Range> FieldRangeGlobalCompute(
  const viskores::cont::DataSet& dataset,
  const std::string& name,
  viskores::cont::Field::Association assoc)
{
  auto lrange = viskores::cont::FieldRangeCompute(dataset, name, assoc);
  return viskores::cont::detail::MergeRangesGlobal(lrange);
}

//-----------------------------------------------------------------------------
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Range> FieldRangeGlobalCompute(
  const viskores::cont::PartitionedDataSet& pds,
  const std::string& name,
  viskores::cont::Field::Association assoc)
{
  auto lrange = viskores::cont::FieldRangeCompute(pds, name, assoc);
  return viskores::cont::detail::MergeRangesGlobal(lrange);
}

//-----------------------------------------------------------------------------
namespace detail
{
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Range> MergeRangesGlobal(
  const viskores::cont::ArrayHandle<viskores::Range>& ranges)
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  if (comm.size() == 1)
  {
    return ranges;
  }

  std::vector<viskores::Range> v_ranges(static_cast<size_t>(ranges.GetNumberOfValues()));
  std::copy(viskores::cont::ArrayPortalToIteratorBegin(ranges.ReadPortal()),
            viskores::cont::ArrayPortalToIteratorEnd(ranges.ReadPortal()),
            v_ranges.begin());

  using VectorOfRangesT = std::vector<viskores::Range>;

  viskoresdiy::Master master(
    comm,
    1,
    -1,
    []() -> void* { return new VectorOfRangesT(); },
    [](void* ptr) { delete static_cast<VectorOfRangesT*>(ptr); });

  viskoresdiy::ContiguousAssigner assigner(/*num ranks*/ comm.size(),
                                           /*global-num-blocks*/ comm.size());
  viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds> decomposer(
    /*dim*/ 1, viskoresdiy::interval(0, comm.size() - 1), comm.size());
  decomposer.decompose(comm.rank(), assigner, master);
  assert(master.size() == 1); // each rank will have exactly 1 block.
  *master.block<VectorOfRangesT>(0) = v_ranges;

  viskoresdiy::RegularAllReducePartners all_reduce_partners(decomposer, /*k*/ 2);

  auto callback = [](VectorOfRangesT* data,
                     const viskoresdiy::ReduceProxy& srp,
                     const viskoresdiy::RegularMergePartners&)
  {
    const auto selfid = srp.gid();
    // 1. dequeue.
    std::vector<int> incoming;
    srp.incoming(incoming);
    for (const int gid : incoming)
    {
      if (gid != selfid)
      {
        VectorOfRangesT message;
        srp.dequeue(gid, message);

        // if the number of components we've seen so far is less than those
        // in the received message, resize so we can accommodate all components
        // in the message. If the message has fewer components, it has no
        // effect.
        data->resize(std::max(data->size(), message.size()));

        std::transform(message.begin(),
                       message.end(),
                       data->begin(),
                       data->begin(),
                       std::plus<viskores::Range>());
      }
    }
    // 2. enqueue
    for (int cc = 0; cc < srp.out_link().size(); ++cc)
    {
      auto target = srp.out_link().target(cc);
      if (target.gid != selfid)
      {
        srp.enqueue(target, *data);
      }
    }
  };

  viskoresdiy::reduce(master, assigner, all_reduce_partners, callback);
  assert(master.size() == 1); // each rank will have exactly 1 block.

  return viskores::cont::make_ArrayHandle(*master.block<VectorOfRangesT>(0),
                                          viskores::CopyFlag::On);
}
} // namespace detail
}
} // namespace viskores::cont
