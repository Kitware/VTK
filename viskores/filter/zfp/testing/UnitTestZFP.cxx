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

#include <viskores/cont/DataSet.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/zfp/ZFPCompressor1D.h>
#include <viskores/filter/zfp/ZFPCompressor2D.h>
#include <viskores/filter/zfp/ZFPCompressor3D.h>
#include <viskores/filter/zfp/ZFPDecompressor1D.h>
#include <viskores/filter/zfp/ZFPDecompressor2D.h>
#include <viskores/filter/zfp/ZFPDecompressor3D.h>

namespace
{

void TestZFP1DFilter(viskores::Float64 rate)
{
  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataset = testDataSet.Make1DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();
  viskores::cont::ArrayHandle<viskores::Float64> field;
  dynField.AsArrayHandle(field);
  auto oport = field.ReadPortal();

  viskores::filter::zfp::ZFPCompressor1D compressor;
  viskores::filter::zfp::ZFPDecompressor1D decompressor;

  compressor.SetActiveField("pointvar");
  compressor.SetRate(rate);
  auto compressed = compressor.Execute(dataset);

  decompressor.SetActiveField("compressed");
  decompressor.SetRate(rate);
  auto decompress = decompressor.Execute(compressed);
  dynField = decompress.GetField("decompressed").GetData();

  dynField.AsArrayHandle(field);
  auto port = field.ReadPortal();

  for (int i = 0; i < field.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(oport.Get(i), port.Get(i), 0.8));
  }
}

void TestZFP2DFilter(viskores::Float64 rate)
{
  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataset = testDataSet.Make2DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();

  viskores::cont::ArrayHandle<viskores::Float64> field;
  dynField.AsArrayHandle(field);
  auto oport = field.ReadPortal();

  viskores::filter::zfp::ZFPCompressor2D compressor;
  viskores::filter::zfp::ZFPDecompressor2D decompressor;

  compressor.SetActiveField("pointvar");
  compressor.SetRate(rate);
  auto compressed = compressor.Execute(dataset);

  decompressor.SetActiveField("compressed");
  decompressor.SetRate(rate);
  auto decompress = decompressor.Execute(compressed);
  dynField = decompress.GetField("decompressed").GetData();

  dynField.AsArrayHandle(field);
  auto port = field.ReadPortal();

  for (int i = 0; i < dynField.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(oport.Get(i), port.Get(i), 0.8));
  }
}

void TestZFP3DFilter(viskores::Float64 rate)
{
  const viskores::Id3 dims(4, 4, 4);
  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataset = testDataSet.Make3DUniformDataSet3(dims);
  auto dynField = dataset.GetField("pointvar").GetData();
  viskores::cont::ArrayHandle<viskores::Float64> field;
  dynField.AsArrayHandle(field);
  auto oport = field.ReadPortal();

  viskores::filter::zfp::ZFPCompressor3D compressor;
  viskores::filter::zfp::ZFPDecompressor3D decompressor;

  compressor.SetActiveField("pointvar");
  compressor.SetRate(rate);
  auto compressed = compressor.Execute(dataset);

  decompressor.SetActiveField("compressed");
  decompressor.SetRate(rate);
  auto decompress = decompressor.Execute(compressed);
  dynField = decompress.GetField("decompressed").GetData();

  dynField.AsArrayHandle(field);
  auto port = field.ReadPortal();

  for (int i = 0; i < dynField.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(oport.Get(i), port.Get(i), 0.8));
  }
}

void TestZFPFilter()
{
  TestZFP1DFilter(4);
  TestZFP2DFilter(4);
  TestZFP2DFilter(4);
}
} // anonymous namespace

int UnitTestZFP(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestZFPFilter, argc, argv);
}
