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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/density_estimate/Statistics.h>
#include <viskores/thirdparty/diy/diy.h>
#include <viskores/thirdparty/diy/environment.h>

namespace
{
viskores::FloatDefault getStatsFromDataSet(const viskores::cont::PartitionedDataSet& dataset,
                                           const std::string statName)
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> array;
  dataset.GetField(statName).GetData().AsArrayHandle(array);
  viskores::cont::ArrayHandle<viskores::FloatDefault>::ReadPortalType portal = array.ReadPortal();
  viskores::FloatDefault value = portal.Get(0);
  return value;
}

void checkResulst(const viskores::cont::PartitionedDataSet& outputPDS, viskores::FloatDefault N)
{
  viskores::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
  VISKORES_TEST_ASSERT(test_equal(NValueFromFilter, N));

  viskores::FloatDefault MinValueFromFilter = getStatsFromDataSet(outputPDS, "Min");
  VISKORES_TEST_ASSERT(test_equal(MinValueFromFilter, 0));

  viskores::FloatDefault MaxValueFromFilter = getStatsFromDataSet(outputPDS, "Max");
  VISKORES_TEST_ASSERT(test_equal(MaxValueFromFilter, N - 1));

  viskores::FloatDefault SumFromFilter = getStatsFromDataSet(outputPDS, "Sum");
  VISKORES_TEST_ASSERT(test_equal(SumFromFilter, N * (N - 1) / 2));

  viskores::FloatDefault MeanFromFilter = getStatsFromDataSet(outputPDS, "Mean");
  VISKORES_TEST_ASSERT(test_equal(MeanFromFilter, (N - 1) / 2));

  viskores::FloatDefault SVFromFilter = getStatsFromDataSet(outputPDS, "SampleVariance");
  VISKORES_TEST_ASSERT(test_equal(SVFromFilter, 83416.66));

  viskores::FloatDefault SstddevFromFilter = getStatsFromDataSet(outputPDS, "SampleStddev");
  VISKORES_TEST_ASSERT(test_equal(SstddevFromFilter, 288.819));

  viskores::FloatDefault SkewnessFromFilter = getStatsFromDataSet(outputPDS, "Skewness");
  VISKORES_TEST_ASSERT(test_equal(SkewnessFromFilter, 0));

  // we use fisher=False when computing the Kurtosis value
  viskores::FloatDefault KurtosisFromFilter = getStatsFromDataSet(outputPDS, "Kurtosis");
  VISKORES_TEST_ASSERT(test_equal(KurtosisFromFilter, 1.8));

  viskores::FloatDefault PopulationStddev = getStatsFromDataSet(outputPDS, "PopulationStddev");
  VISKORES_TEST_ASSERT(test_equal(PopulationStddev, 288.675));

  viskores::FloatDefault PopulationVariance = getStatsFromDataSet(outputPDS, "PopulationVariance");
  VISKORES_TEST_ASSERT(test_equal(PopulationVariance, 83333.3));
}

void TestStatisticsMPISingleDataSet()
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();

  constexpr viskores::FloatDefault N = 1000;
  viskores::Id numProcs = comm.size();

  viskores::Id workloadBase = static_cast<viskores::Id>(N / numProcs);
  viskores::Id workloadActual = workloadBase;
  if (static_cast<viskores::Id>(N) % numProcs != 0)
  {
    //updating the workload for last one
    if (comm.rank() == numProcs - 1)
    {
      workloadActual = workloadActual + (static_cast<viskores::Id>(N) % numProcs);
    }
  }

  viskores::cont::ArrayHandle<viskores::FloatDefault> scalarArray;
  scalarArray.Allocate(static_cast<viskores::Id>(workloadActual));
  auto writePortal = scalarArray.WritePortal();
  for (viskores::Id i = 0; i < static_cast<viskores::Id>(workloadActual); i++)
  {
    writePortal.Set(i, static_cast<viskores::FloatDefault>(workloadBase * comm.rank() + i));
  }

  viskores::cont::DataSet dataSet;
  dataSet.AddPointField("scalarField", scalarArray);

  using STATS = viskores::filter::density_estimate::Statistics;
  STATS statisticsFilter;

  using AsscoType = viskores::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);
  std::vector<viskores::cont::DataSet> dataSetList;
  dataSetList.push_back(dataSet);
  auto pds = viskores::cont::PartitionedDataSet(dataSetList);
  viskores::cont::PartitionedDataSet outputPDS = statisticsFilter.Execute(pds);

  if (comm.rank() == 0)
  {
    checkResulst(outputPDS, N);
  }
  else
  {
    viskores::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
    VISKORES_TEST_ASSERT(test_equal(NValueFromFilter, 0));
  }
}

void TestStatisticsMPIPartitionDataSets()
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  constexpr viskores::FloatDefault N = 1000;
  viskores::Id numProcs = comm.size();

  viskores::Id workloadPerRankBase = static_cast<viskores::Id>(N / numProcs);
  viskores::Id workloadPerRankActual = workloadPerRankBase;

  if (static_cast<viskores::Id>(N) % numProcs != 0)
  {
    //updating the workload for last one
    if (comm.rank() == numProcs - 1)
    {
      workloadPerRankActual = workloadPerRankActual + (static_cast<viskores::Id>(N) % numProcs);
    }
  }

  viskores::Id numPartitions = 2;
  viskores::Id workloadPerPartition0 = workloadPerRankActual / numPartitions;
  viskores::Id workloadPerPartition1 = workloadPerRankActual - workloadPerPartition0;

  viskores::Id offsetRank = workloadPerRankBase * comm.rank();
  std::vector<viskores::cont::DataSet> dataSetList;

  viskores::cont::ArrayHandle<viskores::FloatDefault> scalarArray0;
  scalarArray0.Allocate(static_cast<viskores::Id>(workloadPerPartition0));
  auto writePortal0 = scalarArray0.WritePortal();
  viskores::cont::DataSet dataSet0;

  for (viskores::Id i = 0; i < workloadPerPartition0; i++)
  {
    writePortal0.Set(i, static_cast<viskores::FloatDefault>(offsetRank + i));
  }

  dataSet0.AddPointField("scalarField", scalarArray0);
  dataSetList.push_back(dataSet0);

  viskores::cont::ArrayHandle<viskores::FloatDefault> scalarArray1;
  scalarArray1.Allocate(static_cast<viskores::Id>(workloadPerPartition1));
  auto writePortal1 = scalarArray1.WritePortal();
  viskores::cont::DataSet dataSet1;

  for (viskores::Id i = 0; i < workloadPerPartition1; i++)
  {
    writePortal1.Set(i,
                     static_cast<viskores::FloatDefault>(offsetRank + workloadPerPartition0 + i));
  }

  dataSet1.AddPointField("scalarField", scalarArray1);
  dataSetList.push_back(dataSet1);

  auto pds = viskores::cont::PartitionedDataSet(dataSetList);

  using STATS = viskores::filter::density_estimate::Statistics;
  STATS statisticsFilter;
  using AsscoType = viskores::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);

  viskores::cont::PartitionedDataSet outputPDS = statisticsFilter.Execute(pds);
  if (comm.rank() == 0)
  {
    checkResulst(outputPDS, N);
  }
  else
  {
    viskores::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
    VISKORES_TEST_ASSERT(test_equal(NValueFromFilter, 0));
  }
}

void TestStatisticsMPIDataSetEmpty()
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  constexpr viskores::FloatDefault N = 1000;
  viskores::Id numProcs = comm.size();
  viskores::Id numEmptyBlock = 1;
  viskores::Id numProcsWithWork = numProcs;
  if (numProcs > 1)
  {
    numProcsWithWork = numProcsWithWork - numEmptyBlock;
  }

  viskores::Id workloadBase = static_cast<viskores::Id>(N / (numProcsWithWork));
  viskores::Id workloadActual = workloadBase;
  if (static_cast<viskores::Id>(N) % numProcsWithWork != 0)
  {
    //updating the workload for last one
    if (comm.rank() == numProcsWithWork - 1)
    {
      workloadActual = workloadActual + (static_cast<viskores::Id>(N) % numProcsWithWork);
    }
  }

  viskores::cont::DataSet dataSet;
  viskores::cont::ArrayHandle<viskores::FloatDefault> scalarArray;
  //for the proc with actual work
  if (comm.rank() != numProcs - 1)
  {
    scalarArray.Allocate(static_cast<viskores::Id>(workloadActual));
    auto writePortal = scalarArray.WritePortal();
    for (viskores::Id i = 0; i < static_cast<viskores::Id>(workloadActual); i++)
    {
      writePortal.Set(i, static_cast<viskores::FloatDefault>(workloadBase * comm.rank() + i));
    }
  }
  dataSet.AddPointField("scalarField", scalarArray);

  using STATS = viskores::filter::density_estimate::Statistics;
  STATS statisticsFilter;

  using AsscoType = viskores::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);
  std::vector<viskores::cont::DataSet> dataSetList;
  dataSetList.push_back(dataSet);
  auto pds = viskores::cont::PartitionedDataSet(dataSetList);
  viskores::cont::PartitionedDataSet outputPDS = statisticsFilter.Execute(pds);

  if (comm.size() == 1)
  {
    viskores::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
    VISKORES_TEST_ASSERT(test_equal(NValueFromFilter, 0));
    return;
  }

  if (comm.rank() == 0)
  {
    checkResulst(outputPDS, N);
  }
  else
  {
    viskores::FloatDefault NValueFromFilter = getStatsFromDataSet(outputPDS, "N");
    VISKORES_TEST_ASSERT(test_equal(NValueFromFilter, 0));
  }
}

void TestStatistics()
{
  TestStatisticsMPISingleDataSet();
  TestStatisticsMPIPartitionDataSets();
  TestStatisticsMPIDataSetEmpty();
} // TestFieldStatistics
}

//More deatiled tests can be found in the UnitTestStatisticsFilter
int UnitTestStatisticsFilterMPI(int argc, char* argv[])
{
  viskoresdiy::mpi::environment env(argc, argv);
  viskoresdiy::mpi::communicator world;
  return viskores::cont::testing::Testing::Run(TestStatistics, argc, argv);
}
