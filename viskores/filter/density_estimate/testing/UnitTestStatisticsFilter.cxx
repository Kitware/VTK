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
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/density_estimate/Statistics.h>
#include <viskores/thirdparty/diy/environment.h>

namespace
{

template <typename DataSetType>
viskores::FloatDefault getStatsFromDataSet(const DataSetType& dataset, const std::string& statName)
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> array;
  dataset.GetField(statName).GetData().AsArrayHandle(array);
  viskores::cont::ArrayHandle<viskores::FloatDefault>::ReadPortalType portal = array.ReadPortal();
  viskores::FloatDefault value = portal.Get(0);
  return value;
}

void TestStatisticsPartial()
{
  std::cout << "Test statistics for single viskores::cont::DataSet" << std::endl;
  viskores::cont::DataSet dataSet;
  constexpr viskores::FloatDefault N = 1000;
  auto scalarArrayCounting = viskores::cont::ArrayHandleCounting<viskores::FloatDefault>(
    0.0f, 1.0f, static_cast<viskores::Id>(N));
  viskores::cont::ArrayHandle<viskores::FloatDefault> scalarArray;
  viskores::cont::ArrayCopy(scalarArrayCounting, scalarArray);
  dataSet.AddPointField("scalarField", scalarArray);

  using STATS = viskores::filter::density_estimate::Statistics;
  STATS statisticsFilter;
  using AsscoType = viskores::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);
  viskores::cont::DataSet resultDataSet = statisticsFilter.Execute(dataSet);

  viskores::FloatDefault NValueFromFilter = getStatsFromDataSet(resultDataSet, "N");
  VISKORES_TEST_ASSERT(test_equal(NValueFromFilter, N));

  viskores::FloatDefault MinValueFromFilter = getStatsFromDataSet(resultDataSet, "Min");
  VISKORES_TEST_ASSERT(test_equal(MinValueFromFilter, 0));

  viskores::FloatDefault MaxValueFromFilter = getStatsFromDataSet(resultDataSet, "Max");
  VISKORES_TEST_ASSERT(test_equal(MaxValueFromFilter, N - 1));

  viskores::FloatDefault SumFromFilter = getStatsFromDataSet(resultDataSet, "Sum");
  VISKORES_TEST_ASSERT(test_equal(SumFromFilter, N * (N - 1) / 2));

  viskores::FloatDefault MeanFromFilter = getStatsFromDataSet(resultDataSet, "Mean");
  VISKORES_TEST_ASSERT(test_equal(MeanFromFilter, (N - 1) / 2));

  viskores::FloatDefault SVFromFilter = getStatsFromDataSet(resultDataSet, "SampleVariance");
  VISKORES_TEST_ASSERT(test_equal(SVFromFilter, 83416.66));

  viskores::FloatDefault SstddevFromFilter = getStatsFromDataSet(resultDataSet, "SampleStddev");
  VISKORES_TEST_ASSERT(test_equal(SstddevFromFilter, 288.819));

  viskores::FloatDefault SkewnessFromFilter = getStatsFromDataSet(resultDataSet, "Skewness");
  VISKORES_TEST_ASSERT(test_equal(SkewnessFromFilter, 0));

  // we use fisher=False when computing the Kurtosis value
  viskores::FloatDefault KurtosisFromFilter = getStatsFromDataSet(resultDataSet, "Kurtosis");
  VISKORES_TEST_ASSERT(test_equal(KurtosisFromFilter, 1.8));

  viskores::FloatDefault PopulationStddev = getStatsFromDataSet(resultDataSet, "PopulationStddev");
  VISKORES_TEST_ASSERT(test_equal(PopulationStddev, 288.675));

  viskores::FloatDefault PopulationVariance =
    getStatsFromDataSet(resultDataSet, "PopulationVariance");
  VISKORES_TEST_ASSERT(test_equal(PopulationVariance, 83333.3));
}

void TestStatisticsPartition()
{
  std::cout << "Test statistics for viskores::cont::PartitionedDataSet" << std::endl;

  std::vector<viskores::cont::DataSet> dataSetList;
  constexpr viskores::FloatDefault N = 1000;

  for (viskores::Id i = 0; i < 10; i++)
  {
    viskores::cont::DataSet dataSet;
    constexpr viskores::FloatDefault localN = N / 10;
    viskores::cont::ArrayHandle<viskores::FloatDefault> scalarArray;
    scalarArray.Allocate(static_cast<viskores::Id>(localN));
    auto writePortal = scalarArray.WritePortal();
    for (viskores::Id j = 0; j < static_cast<viskores::Id>(localN); j++)
    {
      writePortal.Set(j, static_cast<viskores::FloatDefault>(i * localN + j));
    }
    dataSet.AddPointField("scalarField", scalarArray);
    dataSetList.push_back(dataSet);
  }

  //adding data sets for testing edge cases
  viskores::cont::DataSet dataSetEmptyField;
  dataSetEmptyField.AddPointField("scalarField",
                                  viskores::cont::ArrayHandle<viskores::FloatDefault>());
  dataSetList.push_back(dataSetEmptyField);

  viskores::cont::PartitionedDataSet pds(dataSetList);
  using STATS = viskores::filter::density_estimate::Statistics;
  STATS statisticsFilter;
  using AsscoType = viskores::cont::Field::Association;
  statisticsFilter.SetActiveField("scalarField", AsscoType::Points);
  viskores::cont::PartitionedDataSet outputPDS = statisticsFilter.Execute(pds);

  std::cout << "  Check aggregate statistics" << std::endl;

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

  viskores::Id numOutPartitions = outputPDS.GetNumberOfPartitions();
  VISKORES_TEST_ASSERT(pds.GetNumberOfPartitions() == numOutPartitions);

  for (viskores::Id partitionId = 0; partitionId < numOutPartitions; ++partitionId)
  {
    std::cout << "  Check partition " << partitionId << std::endl;
    // Assume stats for a single `DataSet` is working.
    viskores::cont::DataSet inPartition = pds.GetPartition(partitionId);
    viskores::cont::DataSet inStats = statisticsFilter.Execute(inPartition);
    viskores::cont::DataSet outStats = outputPDS.GetPartition(partitionId);

    auto checkStats = [&](const std::string& statName)
    {
      viskores::FloatDefault inStat = getStatsFromDataSet(inStats, statName);
      viskores::FloatDefault outStat = getStatsFromDataSet(outStats, statName);
      VISKORES_TEST_ASSERT(test_equal(inStat, outStat));
    };

    checkStats("N");
    checkStats("Min");
    checkStats("Max");
    checkStats("Sum");
    checkStats("Mean");
    checkStats("SampleVariance");
    checkStats("SampleStddev");
    checkStats("Skewness");
    checkStats("Kurtosis");
    checkStats("PopulationStddev");
    checkStats("PopulationVariance");
  }
}

void TestStatistics()
{
  TestStatisticsPartial();
  TestStatisticsPartition();
}

} // anonymous namespace

int UnitTestStatisticsFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestStatistics, argc, argv);
}
