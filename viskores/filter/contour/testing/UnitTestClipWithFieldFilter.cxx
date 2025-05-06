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

#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/contour/ClipWithField.h>

namespace
{

using Coord3D = viskores::Vec3f;

viskores::cont::DataSet MakeTestDatasetExplicit()
{
  std::vector<Coord3D> coords;
  coords.push_back(Coord3D(0.0f, 0.0f, 0.0f));
  coords.push_back(Coord3D(1.0f, 0.0f, 0.0f));
  coords.push_back(Coord3D(1.0f, 1.0f, 0.0f));
  coords.push_back(Coord3D(0.0f, 1.0f, 0.0f));

  std::vector<viskores::Id> connectivity;
  connectivity.push_back(0);
  connectivity.push_back(1);
  connectivity.push_back(3);
  connectivity.push_back(3);
  connectivity.push_back(1);
  connectivity.push_back(2);

  viskores::cont::DataSet ds;
  viskores::cont::DataSetBuilderExplicit builder;
  ds = builder.Create(coords, viskores::CellShapeTagTriangle(), 3, connectivity, "coords");

  std::vector<viskores::Float32> values;
  values.push_back(1.0);
  values.push_back(2.0);
  values.push_back(1.0);
  values.push_back(0.0);
  ds.AddPointField("scalars", values);

  return ds;
}

void TestClipExplicit()
{
  std::cout << "Testing Clip Filter on Explicit data" << std::endl;

  viskores::cont::DataSet ds = MakeTestDatasetExplicit();

  viskores::filter::contour::ClipWithField clip;
  clip.SetClipValue(0.5);
  clip.SetActiveField("scalars");
  clip.SetFieldsToPass("scalars", viskores::cont::Field::Association::Points);

  const viskores::cont::DataSet outputData = clip.Execute(ds);

  VISKORES_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");
  VISKORES_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                       "Wrong number of fields in the output dataset");

  auto temp = outputData.GetField("scalars").GetData();
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  temp.AsArrayHandle(resultArrayHandle);

  viskores::Float32 expected[6] = { 1, 2, 1, 0.5, 0.5, 0.5 };
  for (int i = 0; i < 6; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for Clip filter on triangle explicit data");
  }
}

// Adding for testing cases like Bug #329
// Other tests cover the specific cases of clipping, this test
// is to execute the clipping filter for a larger dataset.
// In this case the output is not verified against a sample.
void TestClipVolume()
{
  std::cout << "Testing Clip Filter on volumetric data" << std::endl;

  viskores::Id3 dims(10, 10, 10);
  viskores::cont::testing::MakeTestDataSet maker;
  viskores::cont::DataSet ds = maker.Make3DUniformDataSet3(dims);

  viskores::filter::contour::ClipWithField clip;
  clip.SetClipValue(0.0);
  clip.SetActiveField("pointvar");
  clip.SetFieldsToPass("pointvar", viskores::cont::Field::Association::Points);

  const viskores::cont::DataSet outputData = clip.Execute(ds);
}

void TestClip()
{
  //todo: add more clip tests
  TestClipExplicit();
  TestClipVolume();
}
}

int UnitTestClipWithFieldFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestClip, argc, argv);
}
