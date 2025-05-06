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
#include <viskores/filter/vector_analysis/SurfaceNormals.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

void VerifyCellNormalValues(const viskores::cont::DataSet& ds)
{
  viskores::cont::ArrayHandle<viskores::Vec3f> normals;
  ds.GetCellField("Normals").GetData().AsArrayHandle(normals);

  viskores::Vec3f expected[8] = { { -0.707f, -0.500f, 0.500f }, { -0.707f, -0.500f, 0.500f },
                                  { 0.707f, 0.500f, -0.500f },  { 0.000f, -0.707f, -0.707f },
                                  { 0.000f, -0.707f, -0.707f }, { 0.000f, 0.707f, 0.707f },
                                  { -0.707f, 0.500f, -0.500f }, { 0.707f, -0.500f, 0.500f } };

  auto portal = normals.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == 8, "incorrect normals array length");
  for (viskores::Id i = 0; i < 8; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(portal.Get(i), expected[i], 0.001),
                         "result does not match expected value");
  }
}

void VerifyPointNormalValues(const viskores::cont::DataSet& ds)
{
  viskores::cont::ArrayHandle<viskores::Vec3f> normals;
  ds.GetPointField("Normals").GetData().AsArrayHandle(normals);

  viskores::Vec3f expected[8] = {
    { -0.8165f, -0.4082f, -0.4082f }, { -0.2357f, -0.9714f, 0.0286f },
    { 0.0000f, -0.1691f, 0.9856f },   { -0.8660f, 0.0846f, 0.4928f },
    { 0.0000f, -0.1691f, -0.9856f },  { 0.0000f, 0.9856f, -0.1691f },
    { 0.8165f, 0.4082f, 0.4082f },    { 0.8165f, -0.4082f, -0.4082f }
  };

  auto portal = normals.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == 8, "incorrect normals array length");
  for (viskores::Id i = 0; i < 8; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(portal.Get(i), expected[i], 0.001),
                         "result does not match expected value");
  }
}

void TestSurfaceNormals()
{
  viskores::cont::DataSet ds =
    viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSetPolygonal();

  viskores::filter::vector_analysis::SurfaceNormals filter;
  viskores::cont::DataSet result;

  std::cout << "testing default output (generate only point normals):\n";
  result = filter.Execute(ds);
  VISKORES_TEST_ASSERT(result.HasPointField("Normals"), "Point normals missing.");

  std::cout << "generate only cell normals:\n";
  filter.SetGenerateCellNormals(true);
  filter.SetGeneratePointNormals(false);
  result = filter.Execute(ds);
  VISKORES_TEST_ASSERT(result.HasCellField("Normals"), "Cell normals missing.");

  std::cout << "generate both cell and point normals:\n";
  filter.SetGeneratePointNormals(true);
  filter.SetAutoOrientNormals(true);
  result = filter.Execute(ds);
  VISKORES_TEST_ASSERT(result.HasPointField("Normals"), "Point normals missing.");
  VISKORES_TEST_ASSERT(result.HasCellField("Normals"), "Cell normals missing.");

  std::cout << "test result values:\n";
  VerifyPointNormalValues(result);
  VerifyCellNormalValues(result);
}

} // anonymous namespace


int UnitTestSurfaceNormalsFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestSurfaceNormals, argc, argv);
}
