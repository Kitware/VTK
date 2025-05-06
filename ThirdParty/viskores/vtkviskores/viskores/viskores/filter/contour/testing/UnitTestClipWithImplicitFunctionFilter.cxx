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

#include <viskores/filter/contour/ClipWithImplicitFunction.h>

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
namespace
{

using Coord3D = viskores::Vec3f;

viskores::cont::DataSet MakeTestDatasetStructured2D()
{
  static constexpr viskores::Id xdim = 3, ydim = 3;
  static const viskores::Id2 dim(xdim, ydim);
  static constexpr viskores::Id numVerts = xdim * ydim;

  viskores::Float32 scalars[numVerts];
  for (float& scalar : scalars)
  {
    scalar = 1.0f;
  }
  scalars[4] = 0.0f;

  viskores::cont::DataSet ds;
  ds = viskores::cont::DataSetBuilderUniform::Create(dim);

  ds.AddPointField("scalars", scalars, numVerts);

  return ds;
}

viskores::cont::DataSet MakeTestDatasetStructured3D()
{
  static constexpr viskores::Id xdim = 3, ydim = 3, zdim = 3;
  static const viskores::Id3 dim(xdim, ydim, zdim);
  static constexpr viskores::Id numVerts = xdim * ydim * zdim;
  viskores::Float32 scalars[numVerts];
  for (viskores::Id i = 0; i < numVerts; i++)
  {
    scalars[i] = static_cast<viskores::Float32>(i * 0.1);
  }
  scalars[13] = 0.0f;
  viskores::cont::DataSet ds;
  ds = viskores::cont::DataSetBuilderUniform::Create(
    dim, viskores::Vec3f(-1.0f, -1.0f, -1.0f), viskores::Vec3f(1, 1, 1));
  ds.AddPointField("scalars", scalars, numVerts);
  return ds;
}

void TestClipStructuredSphere(viskores::Float64 offset)
{
  std::cout << "Testing ClipWithImplicitFunction Filter on Structured data with Sphere function"
            << std::endl;

  viskores::cont::DataSet ds = MakeTestDatasetStructured2D();

  viskores::Vec3f center(1, 1, 0);

  // the `expected` results are based on radius = 0.5 and offset = 0.
  // for a given offset, compute the radius that would produce the same results
  auto radius = static_cast<viskores::FloatDefault>(viskores::Sqrt(0.25 - offset));

  std::cout << "offset = " << offset << ", radius = " << radius << std::endl;

  viskores::filter::contour::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(viskores::Sphere(center, radius));
  clip.SetOffset(offset);
  clip.SetFieldsToPass("scalars");

  viskores::cont::DataSet outputData = clip.Execute(ds);

  VISKORES_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");
  VISKORES_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                       "Wrong number of fields in the output dataset");
  VISKORES_TEST_ASSERT(outputData.GetNumberOfCells() == 8,
                       "Wrong number of cells in the output dataset");

  viskores::cont::UnknownArrayHandle temp = outputData.GetField("scalars").GetData();
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  temp.AsArrayHandle(resultArrayHandle);

  VISKORES_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 12,
                       "Wrong number of points in the output dataset");

  viskores::Float32 expected[12] = { 1, 1, 1, 1, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 12; ++i)
  {
    VISKORES_TEST_ASSERT(
      test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
      "Wrong result for ClipWithImplicitFunction filter on structured quads data");
  }
}

void TestClipStructuredInvertedSphere()
{
  std::cout
    << "Testing ClipWithImplicitFunctionInverted Filter on Structured data with Sphere function"
    << std::endl;

  viskores::cont::DataSet ds = MakeTestDatasetStructured2D();

  viskores::Vec3f center(1, 1, 0);
  viskores::FloatDefault radius(0.5);

  viskores::filter::contour::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(viskores::Sphere(center, radius));
  bool invert = true;
  clip.SetInvertClip(invert);
  clip.SetFieldsToPass("scalars");
  auto outputData = clip.Execute(ds);

  VISKORES_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                       "Wrong number of fields in the output dataset");
  VISKORES_TEST_ASSERT(outputData.GetNumberOfCells() == 4,
                       "Wrong number of cells in the output dataset");

  viskores::cont::UnknownArrayHandle temp = outputData.GetField("scalars").GetData();
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  temp.AsArrayHandle(resultArrayHandle);

  VISKORES_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 5,
                       "Wrong number of points in the output dataset");

  viskores::Float32 expected[5] = { 0, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 5; ++i)
  {
    VISKORES_TEST_ASSERT(
      test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
      "Wrong result for ClipWithImplicitFunction filter on sturctured quads data");
  }
}

void TestClipStructuredInvertedMultiPlane()
{
  std::cout << "Testing TestClipStructured Filter on Structured data with MultiPlane function"
            << std::endl;
  viskores::cont::DataSet ds = MakeTestDatasetStructured3D();
  viskores::filter::contour::ClipWithImplicitFunction clip;
  viskores::MultiPlane<3> TriplePlane;
  //set xy plane
  TriplePlane.AddPlane(viskores::Vec3f{ 1.0f, 1.0f, 0.0f }, viskores::Vec3f{ 0.0f, 0.0f, 1.0f });
  //set yz plane
  TriplePlane.AddPlane(viskores::Vec3f{ 0.0f, 1.0f, 1.0f }, viskores::Vec3f{ 1.0f, 0.0f, 0.0f });
  //set xz plane
  TriplePlane.AddPlane(viskores::Vec3f{ 1.0f, 0.0f, 1.0f }, viskores::Vec3f{ 0.0f, 1.0f, 0.0f });
  clip.SetInvertClip(true);
  clip.SetImplicitFunction(TriplePlane);
  clip.SetFieldsToPass("scalars");
  auto outputData = clip.Execute(ds);

  VISKORES_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");
  VISKORES_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                       "Wrong number of fields in the output dataset");
  VISKORES_TEST_ASSERT(outputData.GetNumberOfCells() == 1,
                       "Wrong number of cells in the output dataset");
  viskores::cont::UnknownArrayHandle temp = outputData.GetField("scalars").GetData();
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  temp.AsArrayHandle(resultArrayHandle);
  viskores::Float32 expected[4] = { 0.0f, 0.1f, 0.3f, 0.9f };
  for (int i = 0; i < 4; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for ClipWithImplicitFunction filter on sturctured data in "
                         "TestClipStructuredInvertedMultiPlane");
  }
}

void TestClip()
{
  //todo: add more clip tests
  TestClipStructuredSphere(-0.2);
  TestClipStructuredSphere(0.0);
  TestClipStructuredSphere(0.2);
  TestClipStructuredInvertedSphere();
  TestClipStructuredInvertedMultiPlane();
}

} // anonymous namespace

int UnitTestClipWithImplicitFunctionFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestClip, argc, argv);
}
