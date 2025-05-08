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
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/entity_extraction/ThresholdPoints.h>
#include <viskores/filter/resampling/HistSampling.h>
#include <viskores/worklet/WorkletMapField.h>
namespace
{

struct CreateFieldValueWorklet : public viskores::worklet::WorkletMapField
{
  viskores::Id SizePerDim;
  VISKORES_CONT CreateFieldValueWorklet(viskores::Id sizePerDim)
    : SizePerDim(sizePerDim)
  {
  }
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(_1, InputIndex);
  template <typename OutPutType>
  VISKORES_EXEC void operator()(OutPutType& val, viskores::Id idx) const
  {
    viskores::Id x = idx % this->SizePerDim;
    viskores::Id y = (idx / this->SizePerDim) % this->SizePerDim;
    viskores::Id z = idx / this->SizePerDim / this->SizePerDim;
    viskores::FloatDefault center =
      static_cast<viskores::FloatDefault>((1.0 * this->SizePerDim) / 2.0);
    viskores::FloatDefault v = viskores::Pow((static_cast<viskores::FloatDefault>(x) - center), 2) +
      viskores::Pow((static_cast<viskores::FloatDefault>(y) - center), 2) +
      viskores::Pow((static_cast<viskores::FloatDefault>(z) - center), 2);
    if (v < 0.5)
    {
      val = static_cast<viskores::FloatDefault>(10.0);
      return;
    }
    val = static_cast<viskores::FloatDefault>(10.0 / viskores::Sqrt(v));
  };
};

void TestHistSamplingSingleBlock()
{
  //creating data set for testing
  viskores::cont::DataSetBuilderUniform dsb;
  constexpr viskores::Id sizePerDim = 20;
  constexpr viskores::Id3 dimensions(sizePerDim, sizePerDim, sizePerDim);
  viskores::cont::DataSet dataSet = dsb.Create(dimensions);

  //creating data set for testing
  viskores::cont::ArrayHandle<viskores::FloatDefault> scalarArray;
  scalarArray.Allocate(sizePerDim * sizePerDim * sizePerDim);
  viskores::cont::Invoker invoker;
  invoker(CreateFieldValueWorklet{ sizePerDim }, scalarArray);
  dataSet.AddPointField("scalarField", scalarArray);

  //calling filter
  using AsscoType = viskores::cont::Field::Association;
  viskores::filter::resampling::HistSampling histsample;
  histsample.SetNumberOfBins(10);
  histsample.SetActiveField("scalarField", AsscoType::Points);
  auto outputDataSet = histsample.Execute(dataSet);

  //checking data sets to make sure all rared region are sampled
  //are there better way to test it?
  viskores::filter::entity_extraction::ThresholdPoints threshold;
  threshold.SetActiveField("scalarField");
  threshold.SetCompactPoints(true);
  threshold.SetThresholdAbove(9.9);
  viskores::cont::DataSet thresholdDataSet = threshold.Execute(outputDataSet);
  //There are 7 points that have the scalar value of 10
  VISKORES_TEST_ASSERT(thresholdDataSet.GetField("scalarField").GetNumberOfValues() == 7);
}

void TestHistSampling()
{
  TestHistSamplingSingleBlock();
} // TestHistSampling

}

int UnitTestHistSampling(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestHistSampling, argc, argv);
}
