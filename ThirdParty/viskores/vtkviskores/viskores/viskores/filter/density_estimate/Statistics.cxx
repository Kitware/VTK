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
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/density_estimate/Statistics.h>
#include <viskores/thirdparty/diy/diy.h>
#include <viskores/worklet/DescriptiveStatistics.h>
#ifdef VISKORES_ENABLE_MPI
#include <mpi.h>
#include <viskores/thirdparty/diy/mpi-cast.h>
#endif

namespace viskores
{
namespace filter
{
namespace density_estimate
{
//refer to this paper https://www.osti.gov/servlets/purl/1028931
//for the math of computing distributed statistics
//using anonymous namespace
namespace
{
using StatValueType = viskores::worklet::DescriptiveStatistics::StatState<viskores::FloatDefault>;
class DistributedStatistics
{
  viskores::cont::ArrayHandle<StatValueType> localStatisticsValues;

public:
  DistributedStatistics(viskores::Id numLocalBlocks)
  {
    this->localStatisticsValues.Allocate(numLocalBlocks);
  }

  void SetLocalStatistics(viskores::Id index, StatValueType& value)
  {
    this->localStatisticsValues.WritePortal().Set(index, value);
  }

  StatValueType ReduceStatisticsDiy() const
  {
    using Algorithm = viskores::cont::Algorithm;
    // The StatValueType struct overloads the + operator. Reduce is using to properly
    // combine statistical measures such as mean, standard deviation, and others. So,
    // the Reduce is computing the global statistics over partitions rather than a
    // simple sum.
    StatValueType statePerRank = Algorithm::Reduce(this->localStatisticsValues, StatValueType{});
    StatValueType stateResult = statePerRank;
#ifdef VISKORES_ENABLE_MPI
    auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() == 1)
    {
      return statePerRank;
    }

    viskoresdiy::Master master(
      comm,
      1,
      -1,
      []() -> void* { return new StatValueType(); },
      [](void* ptr) { delete static_cast<StatValueType*>(ptr); });

    viskoresdiy::ContiguousAssigner assigner(/*num ranks*/ comm.size(),
                                             /*global-num-blocks*/ comm.size());
    viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds> decomposer(
      /*dims*/ 1, viskoresdiy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
    decomposer.decompose(comm.rank(), assigner, master);
    VISKORES_ASSERT(static_cast<viskores::Id>(master.size()) == 1);

    //adding data into master
    *master.block<StatValueType>(0) = statePerRank;
    auto callback = [](StatValueType* result,
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
          StatValueType inData;
          srp.dequeue(gid, inData);
          *result = *result + inData;
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
    };

    viskoresdiy::RegularMergePartners partners(decomposer, /*k=*/2);
    viskoresdiy::reduce(master, assigner, partners, callback);

    //only rank 0 process returns the correct results
    if (master.local(0))
    {
      stateResult = *master.block<StatValueType>(0);
    }
    else
    {
      stateResult = StatValueType();
    }
#endif
    return stateResult;
  }
};
}

viskores::FloatDefault ExtractVariable(viskores::cont::DataSet dataset, const std::string& statName)
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> array;
  dataset.GetField(statName).GetData().AsArrayHandle(array);
  viskores::cont::ArrayHandle<viskores::FloatDefault>::ReadPortalType portal = array.ReadPortal();
  viskores::FloatDefault value = portal.Get(0);
  return value;
}

template <typename T>
VISKORES_CONT viskores::cont::ArrayHandle<viskores::FloatDefault> SaveDataIntoArray(const T value)
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> stat;
  stat.Allocate(1);
  stat.WritePortal().Set(0, static_cast<viskores::FloatDefault>(value));
  return stat;
}

VISKORES_CONT StatValueType GetStatValueFromDataSet(const viskores::cont::DataSet& data)
{
  viskores::FloatDefault N = ExtractVariable(data, "N");
  viskores::FloatDefault Min = ExtractVariable(data, "Min");
  viskores::FloatDefault Max = ExtractVariable(data, "Max");
  viskores::FloatDefault Sum = ExtractVariable(data, "Sum");
  viskores::FloatDefault Mean = ExtractVariable(data, "Mean");
  viskores::FloatDefault M2 = ExtractVariable(data, "M2");
  viskores::FloatDefault M3 = ExtractVariable(data, "M3");
  viskores::FloatDefault M4 = ExtractVariable(data, "M4");
  return StatValueType(N, Min, Max, Sum, Mean, M2, M3, M4);
}

template <typename DataSetType>
VISKORES_CONT void SaveIntoDataSet(StatValueType& statValue,
                                   DataSetType& output,
                                   viskores::cont::Field::Association association)
{
  output.AddField({ "N", association, SaveDataIntoArray(statValue.N()) });
  output.AddField({ "Min", association, SaveDataIntoArray(statValue.Min()) });
  output.AddField({ "Max", association, SaveDataIntoArray(statValue.Max()) });
  output.AddField({ "Sum", association, SaveDataIntoArray(statValue.Sum()) });
  output.AddField({ "Mean", association, SaveDataIntoArray(statValue.Mean()) });
  output.AddField({ "M2", association, SaveDataIntoArray(statValue.M2()) });
  output.AddField({ "M3", association, SaveDataIntoArray(statValue.M3()) });
  output.AddField({ "M4", association, SaveDataIntoArray(statValue.M4()) });
  output.AddField({ "SampleStddev", association, SaveDataIntoArray(statValue.SampleStddev()) });
  output.AddField(
    { "PopulationStddev", association, SaveDataIntoArray(statValue.PopulationStddev()) });
  output.AddField({ "SampleVariance", association, SaveDataIntoArray(statValue.SampleVariance()) });
  output.AddField(
    { "PopulationVariance", association, SaveDataIntoArray(statValue.PopulationVariance()) });
  output.AddField({ "Skewness", association, SaveDataIntoArray(statValue.Skewness()) });
  output.AddField({ "Kurtosis", association, SaveDataIntoArray(statValue.Kurtosis()) });
}

VISKORES_CONT viskores::cont::DataSet Statistics::DoExecute(const viskores::cont::DataSet& inData)
{
  viskores::worklet::DescriptiveStatistics worklet;
  viskores::cont::DataSet output;
  viskores::cont::ArrayHandle<viskores::FloatDefault> input;
  //TODO: GetFieldFromDataSet will throw an exception if the targeted Field does not exist in the data set
  ArrayCopyShallowIfPossible(this->GetFieldFromDataSet(inData).GetData(), input);
  StatValueType result = worklet.Run(input);
  SaveIntoDataSet<viskores::cont::DataSet>(
    result, output, viskores::cont::Field::Association::WholeDataSet);
  return output;
}

VISKORES_CONT viskores::cont::PartitionedDataSet Statistics::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  // This operation will create a partitioned data set with a partition matching each input partition
  // containing the local statistics. It will iterate through each partition in the input and call the
  // DoExecute function. This is the same behavior as if we did not implement `DoExecutePartitions`.
  // It has the added benefit of optimizations for concurrently executing small blocks.
  viskores::cont::PartitionedDataSet output = this->Filter::DoExecutePartitions(input);
  viskores::Id numPartitions = input.GetNumberOfPartitions();
  DistributedStatistics helper(numPartitions);
  for (viskores::Id i = 0; i < numPartitions; ++i)
  {
    const viskores::cont::DataSet& localDS = output.GetPartition(i);
    StatValueType localStatisticsValues = GetStatValueFromDataSet(localDS);
    helper.SetLocalStatistics(i, localStatisticsValues);
  }
  StatValueType result = helper.ReduceStatisticsDiy();
  SaveIntoDataSet<viskores::cont::PartitionedDataSet>(
    result, output, viskores::cont::Field::Association::Global);
  return output;
}
} // namespace density_estimate
} // namespace filter
} // namespace viskores
