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

#include <viskores/filter/density_estimate/Histogram.h>
#include <viskores/filter/density_estimate/worklet/FieldHistogram.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/AssignerPartitionedDataSet.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/FieldRangeGlobalCompute.h>
#include <viskores/cont/Serialization.h>

#include <viskores/thirdparty/diy/diy.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
namespace detail
{
class DistributedHistogram
{
  class Reducer
  {
  public:
    void operator()(viskores::cont::ArrayHandle<viskores::Id>* result,
                    const viskoresdiy::ReduceProxy& srp,
                    const viskoresdiy::RegularMergePartners&) const
    {
      const auto selfid = srp.gid();
      // 1. dequeue.
      std::vector<int> incoming;
      srp.incoming(incoming);
      for (const int gid : incoming)
      {
        if (gid != selfid)
        {
          viskores::cont::ArrayHandle<viskores::Id> in;
          srp.dequeue(gid, in);
          if (result->GetNumberOfValues() == 0)
          {
            *result = in;
          }
          else
          {
            viskores::cont::Algorithm::Transform(*result, in, *result, viskores::Add());
          }
        }
      }

      // 2. enqueue
      for (int cc = 0; cc < srp.out_link().size(); ++cc)
      {
        auto target = srp.out_link().target(cc);
        if (target.gid != selfid)
        {
          srp.enqueue(target, *result);
        }
      }
    }
  };

  std::vector<viskores::cont::ArrayHandle<viskores::Id>> LocalBlocks;

public:
  explicit DistributedHistogram(viskores::Id numLocalBlocks)
    : LocalBlocks(static_cast<size_t>(numLocalBlocks))
  {
  }

  void SetLocalHistogram(viskores::Id index, const viskores::cont::ArrayHandle<viskores::Id>& bins)
  {
    this->LocalBlocks[static_cast<size_t>(index)] = bins;
  }

  void SetLocalHistogram(viskores::Id index, const viskores::cont::Field& field)
  {
    this->SetLocalHistogram(
      index, field.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>());
  }

  viskores::cont::ArrayHandle<viskores::Id> ReduceAll() const
  {
    using ArrayType = viskores::cont::ArrayHandle<viskores::Id>;

    const viskores::Id numLocalBlocks = static_cast<viskores::Id>(this->LocalBlocks.size());
    auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() == 1 && numLocalBlocks <= 1)
    {
      // no reduction necessary.
      return numLocalBlocks == 0 ? ArrayType() : this->LocalBlocks[0];
    }

    viskoresdiy::Master master(
      comm,
      /*threads*/ 1,
      /*limit*/ -1,
      []() -> void* { return new viskores::cont::ArrayHandle<viskores::Id>(); },
      [](void* ptr) { delete static_cast<viskores::cont::ArrayHandle<viskores::Id>*>(ptr); });

    viskores::cont::AssignerPartitionedDataSet assigner(numLocalBlocks);
    viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds> decomposer(
      /*dims*/ 1, viskoresdiy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
    decomposer.decompose(comm.rank(), assigner, master);

    assert(static_cast<viskores::Id>(master.size()) == numLocalBlocks);
    for (viskores::Id cc = 0; cc < numLocalBlocks; ++cc)
    {
      *master.block<ArrayType>(static_cast<int>(cc)) = this->LocalBlocks[static_cast<size_t>(cc)];
    }

    viskoresdiy::RegularMergePartners partners(decomposer, /*k=*/2);
    // reduce to block-0.
    viskoresdiy::reduce(master, assigner, partners, Reducer());

    ArrayType result;
    if (master.local(0))
    {
      result = *master.block<ArrayType>(master.lid(0));
    }

    this->Broadcast(result);
    return result;
  }

private:
  static void Broadcast(viskores::cont::ArrayHandle<viskores::Id>& data)
  {
    // broadcast to all ranks (and not blocks).
    auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() > 1)
    {
      using ArrayType = viskores::cont::ArrayHandle<viskores::Id>;
      viskoresdiy::Master master(
        comm,
        /*threads*/ 1,
        /*limit*/ -1,
        []() -> void* { return new viskores::cont::ArrayHandle<viskores::Id>(); },
        [](void* ptr) { delete static_cast<viskores::cont::ArrayHandle<viskores::Id>*>(ptr); });

      viskoresdiy::ContiguousAssigner assigner(comm.size(), comm.size());
      viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds> decomposer(
        1, viskoresdiy::interval(0, comm.size() - 1), comm.size());
      decomposer.decompose(comm.rank(), assigner, master);
      assert(master.size() == 1); // number of local blocks should be 1 per rank.
      *master.block<ArrayType>(0) = data;
      viskoresdiy::RegularBroadcastPartners partners(decomposer, /*k=*/2);
      viskoresdiy::reduce(master, assigner, partners, Reducer());
      data = *master.block<ArrayType>(0);
    }
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
VISKORES_CONT Histogram::Histogram()
{
  this->SetOutputFieldName("histogram");
}

VISKORES_CONT viskores::cont::DataSet Histogram::DoExecute(const viskores::cont::DataSet& input)
{
  const auto& fieldArray = this->GetFieldFromDataSet(input).GetData();

  if (!this->InExecutePartitions)
  {
    // Handle initialization that would be done in PreExecute if the data set had partitions.
    if (this->Range.IsNonEmpty())
    {
      this->ComputedRange = this->Range;
    }
    else
    {
      auto handle = viskores::cont::FieldRangeGlobalCompute(
        input, this->GetActiveFieldName(), this->GetActiveFieldAssociation());
      if (handle.GetNumberOfValues() != 1)
      {
        throw viskores::cont::ErrorFilterExecution("expecting scalar field.");
      }
      this->ComputedRange = handle.ReadPortal().Get(0);
    }
  }

  viskores::cont::ArrayHandle<viskores::Id> binArray;

  auto resolveType = [&](const auto& concrete)
  {
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    T delta;

    viskores::worklet::FieldHistogram worklet;
    worklet.Run(concrete,
                this->NumberOfBins,
                static_cast<T>(this->ComputedRange.Min),
                static_cast<T>(this->ComputedRange.Max),
                delta,
                binArray);

    this->BinDelta = static_cast<viskores::Float64>(delta);
  };

  fieldArray.CastAndCallForTypesWithFloatFallback<viskores::TypeListFieldScalar,
                                                  VISKORES_DEFAULT_STORAGE_LIST>(resolveType);

  viskores::cont::DataSet output;
  output.AddField(
    { this->GetOutputFieldName(), viskores::cont::Field::Association::WholeDataSet, binArray });

  // The output is a "summary" of the input, no need to map fields
  return output;
}

VISKORES_CONT viskores::cont::PartitionedDataSet Histogram::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  this->PreExecute(input);
  auto result = this->Filter::DoExecutePartitions(input);
  this->PostExecute(input, result);
  return result;
}

//-----------------------------------------------------------------------------
VISKORES_CONT void Histogram::PreExecute(const viskores::cont::PartitionedDataSet& input)
{
  if (this->Range.IsNonEmpty())
  {
    this->ComputedRange = this->Range;
  }
  else
  {
    auto handle = viskores::cont::FieldRangeGlobalCompute(
      input, this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    if (handle.GetNumberOfValues() != 1)
    {
      throw viskores::cont::ErrorFilterExecution("expecting scalar field.");
    }
    this->ComputedRange = handle.ReadPortal().Get(0);
  }
  this->InExecutePartitions = true;
}

//-----------------------------------------------------------------------------
VISKORES_CONT void Histogram::PostExecute(const viskores::cont::PartitionedDataSet&,
                                          viskores::cont::PartitionedDataSet& result)
{
  this->InExecutePartitions = false;
  // iterate and compute histogram for each local block.
  detail::DistributedHistogram helper(result.GetNumberOfPartitions());
  for (viskores::Id cc = 0; cc < result.GetNumberOfPartitions(); ++cc)
  {
    auto& ablock = result.GetPartition(cc);
    helper.SetLocalHistogram(cc, ablock.GetField(this->GetOutputFieldName()));
  }

  viskores::cont::DataSet output;
  viskores::cont::Field rfield(this->GetOutputFieldName(),
                               viskores::cont::Field::Association::WholeDataSet,
                               helper.ReduceAll());
  output.AddField(rfield);

  result = viskores::cont::PartitionedDataSet(output);
}
} // namespace density_estimate
} // namespace filter
} // namespace viskores
