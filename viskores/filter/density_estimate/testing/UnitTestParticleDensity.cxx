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
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/density_estimate/ParticleDensityCloudInCell.h>
#include <viskores/filter/density_estimate/ParticleDensityNearestGridPoint.h>
#include <viskores/worklet/DescriptiveStatistics.h>

void TestNGP()
{
  const viskores::Id N = 1000;
  // This is a better way to create this array, but I am temporarily breaking this
  // functionality (November 2020) so that I can split up merge requests that move
  // ArrayHandles to the new buffer types. This should be restored once
  // ArrayHandleTransform is converted to the new type.
  auto composite = viskores::cont::make_ArrayHandleCompositeVector(
    viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(N, 0xceed),
    viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(N, 0xdeed),
    viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(N, 0xabba));
  viskores::cont::ArrayHandle<viskores::Vec3f> positions;
  viskores::cont::ArrayCopyDevice(composite, positions);

  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleIndex(N), connectivity);

  auto dataSet = viskores::cont::DataSetBuilderExplicit::Create(
    positions, viskores::CellShapeTagVertex{}, 1, connectivity);

  viskores::cont::ArrayHandle<viskores::FloatDefault> mass;
  viskores::cont::ArrayCopyDevice(
    viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>(N, 0xd1ce), mass);
  dataSet.AddCellField("mass", mass);

  viskores::Id3 cellDims = { 3, 3, 3 };
  viskores::Bounds bounds = { { 0, 1 }, { 0, 1 }, { 0, 1 } };
  viskores::filter::density_estimate::ParticleDensityNearestGridPoint filter;
  filter.SetDimension(cellDims);
  filter.SetBounds(bounds);
  filter.SetActiveField("mass");
  VISKORES_TEST_ASSERT(test_equal(filter.GetBounds(), bounds));
  VISKORES_TEST_ASSERT(test_equal(filter.GetOrigin(), viskores::make_Vec(0, 0, 0)));
  VISKORES_TEST_ASSERT(
    test_equal(filter.GetSpacing(), viskores::make_Vec(0.33333, 0.33333, 0.33333)));
  auto density = filter.Execute(dataSet);

  viskores::cont::ArrayHandle<viskores::FloatDefault> field;
  density.GetCellField("density").GetData().AsArrayHandle<viskores::FloatDefault>(field);

  auto mass_result = viskores::worklet::DescriptiveStatistics::Run(mass);
  auto density_result = viskores::worklet::DescriptiveStatistics::Run(field);
  // Unfortunately, floating point atomics suffer from precision error more than everything else.
  VISKORES_TEST_ASSERT(test_equal(density_result.Sum(), mass_result.Sum() * 27.0, 0.1));

  filter.SetComputeNumberDensity(true);
  filter.SetDivideByVolume(false);
  auto counts = filter.Execute(dataSet);

  viskores::cont::ArrayHandle<viskores::FloatDefault> field1;
  counts.GetCellField("density").GetData().AsArrayHandle<viskores::FloatDefault>(field1);

  auto counts_result = viskores::worklet::DescriptiveStatistics::Run(field1);
  VISKORES_TEST_ASSERT(test_equal(counts_result.Sum(), mass_result.N(), 0.1));
}

void TestCIC()
{
  const viskores::Id N = 1000;
  // This is a better way to create this array, but I am temporarily breaking this
  // functionality (November 2020) so that I can split up merge requests that move
  // ArrayHandles to the new buffer types. This should be restored once
  // ArrayHandleTransform is converted to the new type.
  auto composite = viskores::cont::make_ArrayHandleCompositeVector(
    viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(N, 0xceed),
    viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(N, 0xdeed),
    viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(N, 0xabba));
  viskores::cont::ArrayHandle<viskores::Vec3f> positions;
  viskores::cont::ArrayCopyDevice(composite, positions);

  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleIndex(N), connectivity);

  auto dataSet = viskores::cont::DataSetBuilderExplicit::Create(
    positions, viskores::CellShapeTagVertex{}, 1, connectivity);

  viskores::cont::ArrayHandle<viskores::Float32> mass;
  viskores::cont::ArrayCopyDevice(
    viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(N, 0xd1ce), mass);
  dataSet.AddCellField("mass", mass);

  viskores::Id3 cellDims = { 3, 3, 3 };
  viskores::Vec3f origin = { 0.f, 0.f, 0.f };
  viskores::Vec3f spacing = { 1.f / 3.f, 1.f / 3.f, 1.f / 3.f };
  viskores::Bounds bounds = { { 0, 1 }, { 0, 1 }, { 0, 1 } };
  viskores::filter::density_estimate::ParticleDensityCloudInCell filter;
  filter.SetDimension(cellDims);
  filter.SetOrigin(origin);
  filter.SetSpacing(spacing);
  VISKORES_TEST_ASSERT(test_equal(filter.GetOrigin(), origin));
  VISKORES_TEST_ASSERT(test_equal(filter.GetSpacing(), spacing));
  VISKORES_TEST_ASSERT(test_equal(filter.GetBounds(), bounds));
  filter.SetActiveField("mass");
  auto density = filter.Execute(dataSet);

  viskores::cont::ArrayHandle<viskores::Float32> field;
  density.GetPointField("density").GetData().AsArrayHandle<viskores::Float32>(field);

  auto mass_result = viskores::worklet::DescriptiveStatistics::Run(mass);
  auto density_result = viskores::worklet::DescriptiveStatistics::Run(field);
  // Unfortunately, floating point atomics suffer from precision error more than everything else.
  VISKORES_TEST_ASSERT(test_equal(density_result.Sum(), mass_result.Sum() * 27.0, 0.1));

  filter.SetComputeNumberDensity(true);
  filter.SetDivideByVolume(false);
  auto counts = filter.Execute(dataSet);

  viskores::cont::ArrayHandle<viskores::FloatDefault> field1;
  counts.GetPointField("density").GetData().AsArrayHandle<viskores::FloatDefault>(field1);

  auto counts_result = viskores::worklet::DescriptiveStatistics::Run(field1);
  VISKORES_TEST_ASSERT(test_equal(counts_result.Sum(), mass_result.N(), 0.1));
}

void TestParticleDensity()
{
  TestNGP();
  TestCIC();
}

int UnitTestParticleDensity(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestParticleDensity, argc, argv);
}
