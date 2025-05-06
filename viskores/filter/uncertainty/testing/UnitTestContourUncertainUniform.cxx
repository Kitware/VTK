//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <iostream>
#include <random>
#include <viskores/Math.h>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/uncertainty/ContourUncertainUniform.h>
#include <viskores/filter/uncertainty/ContourUncertainUniformMonteCarlo.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

namespace
{
template <typename T>
viskores::cont::DataSet MakeContourUncertainUniformTestDataSet()
{
  const viskores::Id3 dims(25, 25, 25);

  viskores::Id numPoints = dims[0] * dims[1] * dims[2];
  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet dataSet = dataSetBuilder.Create(dims);

  std::vector<T> ensemble_max;
  std::vector<T> ensemble_min;

  viskores::IdComponent k = 0;
  viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault> randomArray(25 * 25 * 25 * 2,
                                                                                   { 0xceed });
  auto portal = randomArray.ReadPortal();

  for (viskores::Id i = 0; i < numPoints; ++i)
  {
    viskores::FloatDefault value1 = -20 + (40 * portal.Get(k));
    viskores::FloatDefault value2 = -20 + (40 * portal.Get(k + 1));
    ensemble_max.push_back(static_cast<T>(viskores::Max(value1, value2)));
    ensemble_min.push_back(static_cast<T>(viskores::Min(value1, value2)));
    k += 2;
  }

  dataSet.AddPointField("ensemble_max", ensemble_max);
  dataSet.AddPointField("ensemble_min", ensemble_min);
  return dataSet;
}

void TestUncertaintyGeneral(viskores::FloatDefault isoValue)
{
  // Isosurface uncertainty computation using closed form solution
  viskores::cont::DataSet input = MakeContourUncertainUniformTestDataSet<viskores::FloatDefault>();
  viskores::filter::uncertainty::ContourUncertainUniform filter;
  filter.SetIsoValue(isoValue);
  filter.SetCrossProbabilityName("CrossProbablity");
  filter.SetNumberNonzeroProbabilityName("NonzeroProbablity");
  filter.SetEntropyName("Entropy");
  filter.SetMinField("ensemble_min");
  filter.SetMaxField("ensemble_max");
  viskores::cont::DataSet output = filter.Execute(input);

  // Isosurface uncertainty computation using Monte Carlo solution
  viskores::filter::uncertainty::ContourUncertainUniformMonteCarlo filter_mc;
  filter_mc.SetIsoValue(isoValue);
  filter_mc.SetNumSample(1000);
  filter_mc.SetCrossProbabilityName("CrossProbablityMC");
  filter_mc.SetNumberNonzeroProbabilityName("NonzeroProbablityMC");
  filter_mc.SetEntropyName("EntropyMC");
  filter_mc.SetMinField("ensemble_min");
  filter_mc.SetMaxField("ensemble_max");
  viskores::cont::DataSet output_mc = filter_mc.Execute(input);

  // Crossing Probablity field (closed form)
  viskores::cont::Field CrossProb = output.GetField("CrossProbablity");
  viskores::cont::ArrayHandle<viskores::FloatDefault> crossProbArray;
  CrossProb.GetData().AsArrayHandle(crossProbArray);
  viskores::cont::ArrayHandle<viskores::FloatDefault>::ReadPortalType CrossPortal =
    crossProbArray.ReadPortal();

  // Nonzero Value field (closed form)
  viskores::cont::Field NonzeroProb = output.GetField("NonzeroProbablity");
  viskores::cont::ArrayHandle<viskores::Id> NonzeroProbArray;
  NonzeroProb.GetData().AsArrayHandle(NonzeroProbArray);
  viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType NonzeroPortal =
    NonzeroProbArray.ReadPortal();

  // Entropy field (closed form)
  viskores::cont::Field entropy = output.GetField("Entropy");
  viskores::cont::ArrayHandle<viskores::FloatDefault> EntropyArray;
  entropy.GetData().AsArrayHandle(EntropyArray);
  viskores::cont::ArrayHandle<viskores::FloatDefault>::ReadPortalType EntropyPortal =
    EntropyArray.ReadPortal();

  // Crossing Probablity field (Marching Cube)
  viskores::cont::Field CrossProbMC = output_mc.GetField("CrossProbablityMC");
  viskores::cont::ArrayHandle<viskores::FloatDefault> crossProbMCArray;
  CrossProbMC.GetData().AsArrayHandle(crossProbMCArray);
  viskores::cont::ArrayHandle<viskores::FloatDefault>::ReadPortalType CrossMCPortal =
    crossProbMCArray.ReadPortal();

  // Nonzero Value field (Marching Cube)
  viskores::cont::Field NonzeroMCProb = output_mc.GetField("NonzeroProbablityMC");
  viskores::cont::ArrayHandle<viskores::FloatDefault> NonzeroProbMCArray;
  NonzeroMCProb.GetData().AsArrayHandle(NonzeroProbMCArray);
  viskores::cont::ArrayHandle<viskores::FloatDefault>::ReadPortalType NonzeroMCPortal =
    NonzeroProbMCArray.ReadPortal();

  // Entropy field (Marching Cube)
  viskores::cont::Field entropyMC = output_mc.GetField("EntropyMC");
  viskores::cont::ArrayHandle<viskores::FloatDefault> EntropyMCArray;
  entropyMC.GetData().AsArrayHandle(EntropyMCArray);
  viskores::cont::ArrayHandle<viskores::FloatDefault>::ReadPortalType EntropyMCPortal =
    EntropyMCArray.ReadPortal();


  // Comparision of Closed-form and Marching Cube
  for (viskores::Id i = 0; i < crossProbArray.GetNumberOfValues(); ++i)
  {
    viskores::FloatDefault CrossProbValue = CrossPortal.Get(i);
    viskores::Id NonzeroProbValue = NonzeroPortal.Get(i); // long long not float
    viskores::FloatDefault EntropyValue = EntropyPortal.Get(i);

    viskores::FloatDefault CrossProbMCValue = CrossMCPortal.Get(i);
    viskores::FloatDefault NonzeroProbMCValue = NonzeroMCPortal.Get(i);
    viskores::FloatDefault EntropyMCValue = EntropyMCPortal.Get(i);

    // Maximum Entropy Difference: 8
    // Maximum Cross Probability Difference: 1
    // Maximum Nonzero Difference: 256
    VISKORES_TEST_ASSERT((viskores::Abs(CrossProbMCValue - CrossProbValue) < 0.1) ||
                           (viskores::Abs(NonzeroProbMCValue - NonzeroProbValue) < 50) ||
                           (viskores::Abs(EntropyMCValue - EntropyValue) < 0.5),
                         viskores::Abs(CrossProbMCValue - CrossProbValue),
                         ' ',
                         viskores::Abs(NonzeroProbMCValue - NonzeroProbValue) < 50,
                         ' ',
                         viskores::Abs(EntropyMCValue - EntropyValue),
                         " No Match With Monte Carlo Sampling");
  }
}

void TestContourUncertainUniform()
{
  viskores::FloatDefault isoValue = 0;
  TestUncertaintyGeneral(isoValue);
}
}
int UnitTestContourUncertainUniform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestContourUncertainUniform, argc, argv);
}
