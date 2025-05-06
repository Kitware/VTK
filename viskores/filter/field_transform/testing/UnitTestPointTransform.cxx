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

#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/field_transform/PointTransform.h>

#include <random>
#include <string>
#include <vector>

namespace
{
std::mt19937 randGenerator;

viskores::cont::DataSet MakePointTransformTestDataSet()
{
  viskores::cont::DataSet dataSet;

  std::vector<viskores::Vec3f> coordinates;
  const viskores::Id dim = 5;
  for (viskores::Id j = 0; j < dim; ++j)
  {
    viskores::FloatDefault z =
      static_cast<viskores::FloatDefault>(j) / static_cast<viskores::FloatDefault>(dim - 1);
    for (viskores::Id i = 0; i < dim; ++i)
    {
      viskores::FloatDefault x =
        static_cast<viskores::FloatDefault>(i) / static_cast<viskores::FloatDefault>(dim - 1);
      viskores::FloatDefault y = (x * x + z * z) / 2.0f;
      coordinates.push_back(viskores::make_Vec(x, y, z));
    }
  }

  viskores::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddCoordinateSystem(
    viskores::cont::make_CoordinateSystem("coordinates", coordinates, viskores::CopyFlag::On));

  viskores::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (viskores::Id j = 0; j < dim - 1; ++j)
  {
    for (viskores::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(viskores::CELL_SHAPE_QUAD,
                      4,
                      viskores::make_Vec<viskores::Id>(
                        j * dim + i, j * dim + i + 1, (j + 1) * dim + i + 1, (j + 1) * dim + i));
    }
  }
  cellSet.CompleteAddingCells(viskores::Id(coordinates.size()));

  dataSet.SetCellSet(cellSet);
  return dataSet;
}

void ValidatePointTransform(const viskores::cont::CoordinateSystem& coords,
                            const std::string& fieldName,
                            const viskores::cont::DataSet& result,
                            const viskores::Matrix<viskores::FloatDefault, 4, 4>& matrix)
{
  //verify the result
  VISKORES_TEST_ASSERT(result.HasField(fieldName, viskores::cont::Field::Association::Points),
                       "Output field missing.");

  viskores::cont::ArrayHandle<viskores::Vec3f> resultArrayHandle;
  result.GetField(fieldName, viskores::cont::Field::Association::Points)
    .GetData()
    .AsArrayHandle(resultArrayHandle);

  auto outPointsArrayHandle = result.GetCoordinateSystem().GetDataAsMultiplexer();

  auto points = coords.GetDataAsMultiplexer();
  VISKORES_TEST_ASSERT(points.GetNumberOfValues() == resultArrayHandle.GetNumberOfValues(),
                       "Incorrect number of points in point transform");

  auto pointsPortal = points.ReadPortal();
  auto resultsPortal = resultArrayHandle.ReadPortal();
  auto outPointsPortal = outPointsArrayHandle.ReadPortal();

  for (viskores::Id i = 0; i < points.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(
      test_equal(resultsPortal.Get(i), viskores::Transform3DPoint(matrix, pointsPortal.Get(i))),
      "Wrong result for PointTransform worklet");
    VISKORES_TEST_ASSERT(
      test_equal(outPointsPortal.Get(i), viskores::Transform3DPoint(matrix, pointsPortal.Get(i))),
      "Wrong result for PointTransform worklet");
  }
}


void TestPointTransformTranslation(const viskores::cont::DataSet& ds, const viskores::Vec3f& trans)
{
  viskores::filter::field_transform::PointTransform filter;

  filter.SetOutputFieldName("translation");
  filter.SetTranslation(trans);
  VISKORES_TEST_ASSERT(filter.GetChangeCoordinateSystem() == true);
  viskores::cont::DataSet result = filter.Execute(ds);

  ValidatePointTransform(
    ds.GetCoordinateSystem(), "translation", result, viskores::Transform3DTranslate(trans));
}

void TestPointTransformScale(const viskores::cont::DataSet& ds, const viskores::Vec3f& scale)
{
  viskores::filter::field_transform::PointTransform filter;

  filter.SetOutputFieldName("scale");
  filter.SetScale(scale);
  filter.SetChangeCoordinateSystem(true);
  viskores::cont::DataSet result = filter.Execute(ds);

  ValidatePointTransform(
    ds.GetCoordinateSystem(), "scale", result, viskores::Transform3DScale(scale));
}

void TestPointTransformRotation(const viskores::cont::DataSet& ds,
                                const viskores::FloatDefault& angle,
                                const viskores::Vec3f& axis)
{
  viskores::filter::field_transform::PointTransform filter;

  filter.SetOutputFieldName("rotation");
  filter.SetRotation(angle, axis);
  viskores::cont::DataSet result = filter.Execute(ds);

  ValidatePointTransform(
    ds.GetCoordinateSystem(), "rotation", result, viskores::Transform3DRotate(angle, axis));
}

void TestPointTransformGeneral(const viskores::cont::DataSet& ds,
                               const viskores::Matrix<viskores::FloatDefault, 4, 4>& transform)
{
  viskores::filter::field_transform::PointTransform filter;

  auto fieldName = filter.GetOutputFieldName();
  filter.SetTransform(transform);
  viskores::cont::DataSet result = filter.Execute(ds);

  ValidatePointTransform(ds.GetCoordinateSystem(), fieldName, result, transform);
}
}

void TestPointTransform()
{
  std::cout << "Testing PointTransform Worklet" << std::endl;

  viskores::cont::DataSet ds = MakePointTransformTestDataSet();
  int N = 41;

  //Test translation
  TestPointTransformTranslation(ds, viskores::Vec3f(0, 0, 0));
  TestPointTransformTranslation(ds, viskores::Vec3f(1, 1, 1));
  TestPointTransformTranslation(ds, viskores::Vec3f(-1, -1, -1));

  std::uniform_real_distribution<viskores::FloatDefault> transDist(-100, 100);
  for (int i = 0; i < N; i++)
    TestPointTransformTranslation(ds,
                                  viskores::Vec3f(transDist(randGenerator),
                                                  transDist(randGenerator),
                                                  transDist(randGenerator)));

  //Test scaling
  TestPointTransformScale(ds, viskores::Vec3f(1, 1, 1));
  TestPointTransformScale(ds, viskores::Vec3f(.23f, .23f, .23f));
  TestPointTransformScale(ds, viskores::Vec3f(1, 2, 3));
  TestPointTransformScale(ds, viskores::Vec3f(3.23f, 9.23f, 4.23f));

  std::uniform_real_distribution<viskores::FloatDefault> scaleDist(0.0001f, 100);
  for (int i = 0; i < N; i++)
  {
    TestPointTransformScale(ds, viskores::Vec3f(scaleDist(randGenerator)));
    TestPointTransformScale(ds,
                            viskores::Vec3f(scaleDist(randGenerator),
                                            scaleDist(randGenerator),
                                            scaleDist(randGenerator)));
  }

  //Test rotation
  std::vector<viskores::FloatDefault> angles;
  std::uniform_real_distribution<viskores::FloatDefault> angleDist(0, 360);
  for (int i = 0; i < N; i++)
    angles.push_back(angleDist(randGenerator));

  std::vector<viskores::Vec3f> axes;
  axes.emplace_back(1.f, 0.f, 0.f);
  axes.emplace_back(0.f, 1.f, 0.f);
  axes.emplace_back(0.f, 0.f, 1.f);
  axes.emplace_back(1.f, 1.f, 1.f);
  axes.push_back(-axes[0]);
  axes.push_back(-axes[1]);
  axes.push_back(-axes[2]);
  axes.push_back(-axes[3]);

  std::uniform_real_distribution<viskores::FloatDefault> axisDist(-1, 1);
  for (int i = 0; i < N; i++)
    axes.emplace_back(axisDist(randGenerator), axisDist(randGenerator), axisDist(randGenerator));

  for (auto& angle : angles)
    for (auto& axe : axes)
      TestPointTransformRotation(ds, angle, axe);

  //Test general
  auto transform = viskores::Transform3DTranslate(viskores::Vec3f{ 1, 1, 1 });
  transform = viskores::MatrixMultiply(
    transform, viskores::Transform3DScale(static_cast<viskores::FloatDefault>(1.5f)));
  transform = viskores::MatrixMultiply(
    transform, viskores::Transform3DRotateX(static_cast<viskores::FloatDefault>(90.f)));
  TestPointTransformGeneral(ds, transform);
}


int UnitTestPointTransform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestPointTransform, argc, argv);
}
