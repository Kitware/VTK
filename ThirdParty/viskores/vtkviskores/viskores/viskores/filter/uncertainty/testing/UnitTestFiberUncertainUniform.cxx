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

#include <iostream>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/uncertainty/FiberUncertainUniform.h>

namespace
{
template <typename T>
viskores::cont::DataSet MakeFiberUncertainUniformDataSet()
{
  const viskores::Id3 dims(20, 20, 20);
  viskores::Id numPoints = dims[0] * dims[1] * dims[2];
  viskores::cont::DataSetBuilderUniform dsBuilder;
  viskores::cont::DataSet ds = dsBuilder.Create(dims);

  std::vector<T> ensembleMin1;
  std::vector<T> ensembleMax1;
  std::vector<T> ensembleMin2;
  std::vector<T> ensembleMax2;

  viskores::IdComponent k = 0;
  viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault> randomArray(numPoints * 4,
                                                                                   { 0xceed });
  auto portal = randomArray.ReadPortal();

  for (viskores::Id i = 0; i < numPoints; ++i)
  {
    viskores::FloatDefault value1 = 10 + (20 * portal.Get(k));
    viskores::FloatDefault value2 = 10 + (20 * portal.Get(k + 1));
    viskores::FloatDefault value3 = 10 + (20 * portal.Get(k + 2));
    viskores::FloatDefault value4 = 10 + (20 * portal.Get(k + 3));
    ensembleMin1.push_back(static_cast<T>(viskores::Min(value1, value2)));
    ensembleMax1.push_back(static_cast<T>(viskores::Max(value1, value2)));
    ensembleMin2.push_back(static_cast<T>(viskores::Min(value3, value4)));
    ensembleMax2.push_back(static_cast<T>(viskores::Max(value3, value4)));
    k += 4;
  }

  ds.AddPointField("ensemble_min_1", ensembleMin1);
  ds.AddPointField("ensemble_max_1", ensembleMax1);
  ds.AddPointField("ensemble_min_2", ensembleMin2);
  ds.AddPointField("ensemble_max_2", ensembleMax2);

  return ds;
}

void TestFiberUncertainUniform()
{
  viskores::cont::DataSet ds = MakeFiberUncertainUniformDataSet<viskores::FloatDefault>();

  viskores::Range rangeAxis1(15.0, 15.0);
  viskores::Range rangeAxis2(25.0, 25.0);

  const viskores::FloatDefault delta = 0.05f;

  viskores::filter::uncertainty::FiberUncertainUniform closedFormFilter;
  closedFormFilter.SetRange1(rangeAxis1);
  closedFormFilter.SetRange2(rangeAxis2);
  closedFormFilter.SetField1Min("ensemble_min_1");
  closedFormFilter.SetField1Max("ensemble_max_1");
  closedFormFilter.SetField2Min("ensemble_min_2");
  closedFormFilter.SetField2Max("ensemble_max_2");
  closedFormFilter.SetApproach(
    viskores::filter::uncertainty::FiberUncertainUniform::ApproachEnum::ClosedForm);

  viskores::cont::DataSet outputClosed = closedFormFilter.Execute(ds);
  viskores::cont::Field closedField = outputClosed.GetField("ClosedForm");
  viskores::cont::UnknownArrayHandle unknownClosed = closedField.GetData();

  viskores::cont::ArrayHandle<viskores::FloatDefault> closedArray;
  unknownClosed.AsArrayHandle(closedArray);
  auto closedPortal = closedArray.ReadPortal();

  viskores::filter::uncertainty::FiberUncertainUniform monteCarloFilter;
  monteCarloFilter.SetRange1(rangeAxis1);
  monteCarloFilter.SetRange2(rangeAxis2);
  monteCarloFilter.SetField1Min("ensemble_min_1");
  monteCarloFilter.SetField1Max("ensemble_max_1");
  monteCarloFilter.SetField2Min("ensemble_min_2");
  monteCarloFilter.SetField2Max("ensemble_max_2");
  monteCarloFilter.SetApproach(
    viskores::filter::uncertainty::FiberUncertainUniform::ApproachEnum::MonteCarlo);

  monteCarloFilter.SetNumberOfSamples(5000);
  viskores::cont::DataSet outputMC = monteCarloFilter.Execute(ds);
  viskores::cont::Field monteField = outputMC.GetField("MonteCarlo");
  viskores::cont::UnknownArrayHandle unknownMC = monteField.GetData();
  viskores::cont::ArrayHandle<viskores::FloatDefault> monteArray;
  unknownMC.AsArrayHandle(monteArray);
  auto montePortal = monteArray.ReadPortal();
  viskores::Id numValues = closedArray.GetNumberOfValues();
  std::cout << "Comparing outputs for " << numValues << " values." << std::endl;
  for (viskores::Id i = 0; i < numValues; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(closedPortal.Get(i), montePortal.Get(i), 0.05),
                         "Difference between ClosedForm and MonteCarlo value too large.");
  }
}
}

int UnitTestFiberUncertainUniform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestFiberUncertainUniform, argc, argv);
}
