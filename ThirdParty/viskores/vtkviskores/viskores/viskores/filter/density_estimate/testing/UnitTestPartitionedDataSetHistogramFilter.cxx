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

#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/thirdparty/diy/environment.h>

#include <algorithm>
#include <numeric>
#include <random>

namespace
{

unsigned int uid = 1;

template <typename T>
viskores::cont::ArrayHandle<T> CreateArrayHandle(T min, T max, viskores::Id numVals)
{
  std::mt19937 gen(uid++);
  std::uniform_real_distribution<double> dis(static_cast<double>(min), static_cast<double>(max));

  viskores::cont::ArrayHandle<T> handle;
  handle.Allocate(numVals);

  auto portal = handle.WritePortal();
  std::generate(viskores::cont::ArrayPortalToIteratorBegin(portal),
                viskores::cont::ArrayPortalToIteratorEnd(portal),
                [&]() { return static_cast<T>(dis(gen)); });
  return handle;
}

template <typename T, int size>
viskores::cont::ArrayHandle<viskores::Vec<T, size>> CreateArrayHandle(
  const viskores::Vec<T, size>& min,
  const viskores::Vec<T, size>& max,
  viskores::Id numVals)
{
  std::mt19937 gen(uid++);
  std::uniform_real_distribution<double> dis[size];
  for (int cc = 0; cc < size; ++cc)
  {
    dis[cc] = std::uniform_real_distribution<double>(static_cast<double>(min[cc]),
                                                     static_cast<double>(max[cc]));
  }
  viskores::cont::ArrayHandle<T> handle;
  handle.Allocate(numVals);
  auto portal = handle.WritePortal();
  std::generate(viskores::cont::ArrayPortalToIteratorBegin(portal),
                viskores::cont::ArrayPortalToIteratorEnd(portal),
                [&]()
                {
                  viskores::Vec<T, size> val;
                  for (int cc = 0; cc < size; ++cc)
                  {
                    val[cc] = static_cast<T>(dis[cc](gen));
                  }
                  return val;
                });
  return handle;
}


template <typename T>
void AddField(viskores::cont::DataSet& dataset,
              const T& min,
              const T& max,
              viskores::Id numVals,
              const std::string& name,
              viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Points)
{
  auto ah = CreateArrayHandle(min, max, numVals);
  dataset.AddField(viskores::cont::Field(name, assoc, ah));
}
}

static void TestPartitionedDataSetHistogram()
{
  // init random seed.
  std::srand(100);

  viskores::cont::PartitionedDataSet mb;

  viskores::cont::DataSet partition0;
  AddField<double>(partition0, 0.0, 100.0, 1024, "double");
  mb.AppendPartition(partition0);

  viskores::cont::DataSet partition1;
  AddField<int>(partition1, 100, 1000, 1024, "double");
  mb.AppendPartition(partition1);

  viskores::cont::DataSet partition2;
  AddField<double>(partition2, 100.0, 500.0, 1024, "double");
  mb.AppendPartition(partition2);

  viskores::filter::density_estimate::Histogram histogram;
  histogram.SetActiveField("double");
  auto result = histogram.Execute(mb);
  VISKORES_TEST_ASSERT(result.GetNumberOfPartitions() == 1, "Expecting 1 partition.");

  auto bins = result.GetPartition(0)
                .GetField("histogram")
                .GetData()
                .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  VISKORES_TEST_ASSERT(bins.GetNumberOfValues() == 10, "Expecting 10 bins.");
  auto binsPortal = bins.ReadPortal();
  auto count = std::accumulate(viskores::cont::ArrayPortalToIteratorBegin(binsPortal),
                               viskores::cont::ArrayPortalToIteratorEnd(binsPortal),
                               viskores::Id(0),
                               viskores::Add());
  VISKORES_TEST_ASSERT(count == 1024 * 3, "Expecting 3072 values");

  std::cout << "Values [" << count << "] =";
  for (int cc = 0; cc < 10; ++cc)
  {
    std::cout << " " << binsPortal.Get(cc);
  }
  std::cout << std::endl;
}

int UnitTestPartitionedDataSetHistogramFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestPartitionedDataSetHistogram, argc, argv);
}
