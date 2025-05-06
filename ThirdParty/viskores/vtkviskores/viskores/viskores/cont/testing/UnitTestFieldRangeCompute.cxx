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

#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/FieldRangeCompute.h>
#include <viskores/cont/testing/Testing.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

static unsigned int uid = 1;

template <typename T>
viskores::cont::ArrayHandle<T> CreateArray(T min,
                                           T max,
                                           viskores::Id numVals,
                                           viskores::TypeTraitsScalarTag)
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

template <typename T>
viskores::cont::ArrayHandle<T> CreateArray(const T& min,
                                           const T& max,
                                           viskores::Id numVals,
                                           viskores::TypeTraitsVectorTag)
{
  constexpr int size = T::NUM_COMPONENTS;
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
                  T val;
                  for (int cc = 0; cc < size; ++cc)
                  {
                    val[cc] = static_cast<typename T::ComponentType>(dis[cc](gen));
                  }
                  return val;
                });
  return handle;
}

static constexpr viskores::Id ARRAY_SIZE = 1025;

template <typename ValueType>
void Validate(const viskores::cont::ArrayHandle<viskores::Range>& ranges,
              const ValueType& min,
              const ValueType& max)
{
  VISKORES_TEST_ASSERT(ranges.GetNumberOfValues() == 1, "Wrong number of ranges");

  auto portal = ranges.ReadPortal();
  auto range = portal.Get(0);
  std::cout << "  expecting [" << min << ", " << max << "], got [" << range.Min << ", " << range.Max
            << "]" << std::endl;
  VISKORES_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<ValueType>(min) &&
                         range.Max <= static_cast<ValueType>(max),
                       "Got wrong range.");
}

template <typename T, int size>
void Validate(const viskores::cont::ArrayHandle<viskores::Range>& ranges,
              const viskores::Vec<T, size>& min,
              const viskores::Vec<T, size>& max)
{
  VISKORES_TEST_ASSERT(ranges.GetNumberOfValues() == size, "Wrong number of ranges");

  auto portal = ranges.ReadPortal();
  for (int cc = 0; cc < size; ++cc)
  {
    auto range = portal.Get(cc);
    std::cout << "  [0] expecting [" << min[cc] << ", " << max[cc] << "], got [" << range.Min
              << ", " << range.Max << "]" << std::endl;
    VISKORES_TEST_ASSERT(range.IsNonEmpty() && range.Min >= static_cast<T>(min[cc]) &&
                           range.Max <= static_cast<T>(max[cc]),
                         "Got wrong range.");
  }
}

template <typename ValueType>
void TryRangeComputeDS(const ValueType& min, const ValueType& max)
{
  std::cout << "Trying type (dataset): " << viskores::testing::TypeName<ValueType>::Name()
            << std::endl;
  // let's create a dummy dataset with a bunch of fields.
  viskores::cont::DataSet dataset;
  dataset.AddPointField(
    "pointvar",
    CreateArray(
      min, max, ARRAY_SIZE, typename viskores::TypeTraits<ValueType>::DimensionalityTag()));

  viskores::cont::ArrayHandle<viskores::Range> ranges =
    viskores::cont::FieldRangeCompute(dataset, "pointvar");
  Validate(ranges, min, max);
}

template <typename ValueType>
void TryRangeComputePDS(const ValueType& min, const ValueType& max)
{
  std::cout << "Trying type (PartitionedDataSet): "
            << viskores::testing::TypeName<ValueType>::Name() << std::endl;

  viskores::cont::PartitionedDataSet mb;
  for (int cc = 0; cc < 5; cc++)
  {
    // let's create a dummy dataset with a bunch of fields.
    viskores::cont::DataSet dataset;
    dataset.AddPointField(
      "pointvar",
      CreateArray(
        min, max, ARRAY_SIZE, typename viskores::TypeTraits<ValueType>::DimensionalityTag()));
    mb.AppendPartition(dataset);
  }

  viskores::cont::ArrayHandle<viskores::Range> ranges =
    viskores::cont::FieldRangeCompute(mb, "pointvar");
  Validate(ranges, min, max);
}

static void TestFieldRangeCompute()
{
  // init random seed.
  std::srand(100);

  TryRangeComputeDS<viskores::Float64>(0, 1000);
  TryRangeComputeDS<viskores::Int32>(-1024, 1024);
  TryRangeComputeDS<viskores::Vec3f_32>(viskores::make_Vec(1024, 0, -1024),
                                        viskores::make_Vec(2048, 2048, 2048));
  TryRangeComputePDS<viskores::Float64>(0, 1000);
  TryRangeComputePDS<viskores::Int32>(-1024, 1024);
  TryRangeComputePDS<viskores::Vec3f_32>(viskores::make_Vec(1024, 0, -1024),
                                         viskores::make_Vec(2048, 2048, 2048));
};

int UnitTestFieldRangeCompute(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestFieldRangeCompute, argc, argv);
}
