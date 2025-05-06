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

#include <viskores/Particle.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/flow/StreamSurface.h>

namespace
{

viskores::cont::DataSet CreateDataSet(const viskores::Id3& dims, const viskores::Vec3f& vec)
{
  viskores::Id numPoints = dims[0] * dims[1] * dims[2];

  std::vector<viskores::Vec3f> vectorField(static_cast<std::size_t>(numPoints));
  for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    vectorField[i] = vec;

  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet ds = dataSetBuilder.Create(dims);
  ds.AddPointField("vector", vectorField);

  return ds;
}

void TestStreamSurface()
{
  std::cout << "Testing Stream Surface Filter" << std::endl;

  const viskores::Id3 dims(5, 5, 5);
  const viskores::Vec3f vecX(1, 0, 0);

  viskores::cont::DataSet ds = CreateDataSet(dims, vecX);
  viskores::cont::ArrayHandle<viskores::Particle> seedArray =
    viskores::cont::make_ArrayHandle({ viskores::Particle(viskores::Vec3f(.1f, 1.0f, .2f), 0),
                                       viskores::Particle(viskores::Vec3f(.1f, 2.0f, .1f), 1),
                                       viskores::Particle(viskores::Vec3f(.1f, 3.0f, .3f), 2),
                                       viskores::Particle(viskores::Vec3f(.1f, 3.5f, .2f), 3) });

  viskores::filter::flow::StreamSurface streamSrf;

  streamSrf.SetStepSize(0.1f);
  streamSrf.SetNumberOfSteps(20);
  streamSrf.SetSeeds(seedArray);
  streamSrf.SetActiveField("vector");

  auto output = streamSrf.Execute(ds);

  //Validate the result is correct.
  VISKORES_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");

  viskores::cont::CoordinateSystem coords = output.GetCoordinateSystem();
  VISKORES_TEST_ASSERT(coords.GetNumberOfPoints() == 84, "Wrong number of coordinates");

  viskores::cont::UnknownCellSet dcells = output.GetCellSet();
  VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == 120, "Wrong number of cells");
}
}

int UnitTestStreamSurfaceFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestStreamSurface, argc, argv);
}
