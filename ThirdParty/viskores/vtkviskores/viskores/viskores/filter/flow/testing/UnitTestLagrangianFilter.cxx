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
#include <viskores/cont/CellLocatorBoundingIntervalHierarchy.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/flow/Lagrangian.h>
#include <viskores/filter/flow/testing/GenerateTestDataSets.h>

namespace
{

std::vector<viskores::cont::DataSet> MakeDataSets()
{
  viskores::Bounds bounds(0, 10, 0, 10, 0, 10);
  const viskores::Id3 dims(8, 8, 8);
  auto dataSets = viskores::worklet::testing::CreateAllDataSets(bounds, dims, false);

  viskores::Id numPoints = dims[0] * dims[1] * dims[2];

  for (auto& ds : dataSets)
  {
    viskores::cont::ArrayHandle<viskores::Vec3f> velocityField;
    velocityField.Allocate(numPoints);

    auto velocityPortal = velocityField.WritePortal();
    viskores::Id count = 0;
    for (viskores::Id i = 0; i < dims[0]; i++)
      for (viskores::Id j = 0; j < dims[1]; j++)
        for (viskores::Id k = 0; k < dims[2]; k++)
        {
          viskores::FloatDefault val = static_cast<viskores::FloatDefault>(0.1);
          velocityPortal.Set(count, viskores::Vec3f(val, val, val));
          count++;
        }
    ds.AddPointField("velocity", velocityField);
  }

  return dataSets;
}

void TestLagrangianFilterMultiStepInterval()
{
  viskores::Id maxCycles = 5;
  viskores::Id write_interval = 5;
  viskores::filter::flow::Lagrangian lagrangianFilter2;
  lagrangianFilter2.SetResetParticles(true);
  lagrangianFilter2.SetStepSize(0.1f);
  lagrangianFilter2.SetWriteFrequency(write_interval);

  auto dataSets = MakeDataSets();
  for (auto& input : dataSets)
  {
    for (viskores::Id i = 1; i <= maxCycles; i++)
    {
      lagrangianFilter2.SetActiveField("velocity");
      viskores::cont::DataSet extractedBasisFlows = lagrangianFilter2.Execute(input);
      if (i % write_interval == 0)
      {
        VISKORES_TEST_ASSERT(extractedBasisFlows.GetNumberOfCoordinateSystems() == 1,
                             "Wrong number of coordinate systems in the output dataset.");
        VISKORES_TEST_ASSERT(extractedBasisFlows.GetNumberOfPoints() == 512,
                             "Wrong number of basis flows extracted.");
        VISKORES_TEST_ASSERT(extractedBasisFlows.GetNumberOfFields() == 3,
                             "Wrong number of fields.");
      }
      else
      {
        VISKORES_TEST_ASSERT(extractedBasisFlows.GetNumberOfPoints() == 0,
                             "Output dataset should have no points.");
        VISKORES_TEST_ASSERT(extractedBasisFlows.GetNumberOfCoordinateSystems() == 0,
                             "Wrong number of coordinate systems in the output dataset.");
        VISKORES_TEST_ASSERT(extractedBasisFlows.GetNumberOfFields() == 0,
                             "Wrong number of fields.");
      }
    }
  }
}

} //namespace

void TestLagrangian()
{
  TestLagrangianFilterMultiStepInterval();
}

int UnitTestLagrangianFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestLagrangian, argc, argv);
}
