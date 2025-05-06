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

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/vector_analysis/DotProduct.h>

#include <random>
#include <vector>

namespace
{
std::mt19937 randGenerator;

template <typename T>
void createVectors(std::size_t numPts,
                   int vecType,
                   std::vector<viskores::Vec<T, 3>>& vecs1,
                   std::vector<viskores::Vec<T, 3>>& vecs2)
{
  if (vecType == 0) // X x Y
  {
    vecs1.resize(numPts, viskores::make_Vec(1, 0, 0));
    vecs2.resize(numPts, viskores::make_Vec(0, 1, 0));
  }
  else if (vecType == 1) // Y x Z
  {
    vecs1.resize(numPts, viskores::make_Vec(0, 1, 0));
    vecs2.resize(numPts, viskores::make_Vec(0, 0, 1));
  }
  else if (vecType == 2) // Z x X
  {
    vecs1.resize(numPts, viskores::make_Vec(0, 0, 1));
    vecs2.resize(numPts, viskores::make_Vec(1, 0, 0));
  }
  else if (vecType == 3) // Y x X
  {
    vecs1.resize(numPts, viskores::make_Vec(0, 1, 0));
    vecs2.resize(numPts, viskores::make_Vec(1, 0, 0));
  }
  else if (vecType == 4) // Z x Y
  {
    vecs1.resize(numPts, viskores::make_Vec(0, 0, 1));
    vecs2.resize(numPts, viskores::make_Vec(0, 1, 0));
  }
  else if (vecType == 5) // X x Z
  {
    vecs1.resize(numPts, viskores::make_Vec(1, 0, 0));
    vecs2.resize(numPts, viskores::make_Vec(0, 0, 1));
  }
  else if (vecType == 6)
  {
    //Test some other vector combinations
    std::uniform_real_distribution<viskores::Float64> randomDist(-10.0, 10.0);

    vecs1.resize(numPts);
    vecs2.resize(numPts);
    for (std::size_t i = 0; i < numPts; i++)
    {
      vecs1[i] = viskores::make_Vec(
        randomDist(randGenerator), randomDist(randGenerator), randomDist(randGenerator));
      vecs2[i] = viskores::make_Vec(
        randomDist(randGenerator), randomDist(randGenerator), randomDist(randGenerator));
    }
  }
}

void CheckResult(const viskores::cont::ArrayHandle<viskores::Vec3f>& field1,
                 const viskores::cont::ArrayHandle<viskores::Vec3f>& field2,
                 const viskores::cont::DataSet& result)
{
  VISKORES_TEST_ASSERT(result.HasPointField("dotproduct"), "Output field is missing.");

  viskores::cont::ArrayHandle<viskores::FloatDefault> outputArray;
  result.GetPointField("dotproduct").GetData().AsArrayHandle(outputArray);

  auto v1Portal = field1.ReadPortal();
  auto v2Portal = field2.ReadPortal();
  auto outPortal = outputArray.ReadPortal();

  VISKORES_TEST_ASSERT(outputArray.GetNumberOfValues() == field1.GetNumberOfValues(),
                       "Field sizes wrong");
  VISKORES_TEST_ASSERT(outputArray.GetNumberOfValues() == field2.GetNumberOfValues(),
                       "Field sizes wrong");

  for (viskores::Id j = 0; j < outputArray.GetNumberOfValues(); j++)
  {
    viskores::Vec3f v1 = v1Portal.Get(j);
    viskores::Vec3f v2 = v2Portal.Get(j);
    viskores::FloatDefault res = outPortal.Get(j);

    VISKORES_TEST_ASSERT(test_equal(viskores::Dot(v1, v2), res), "Wrong result for dot product");
  }
}

void TestDotProduct()
{
  std::cout << "Testing DotProduct Filter" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;

  const int numCases = 7;
  for (int i = 0; i < numCases; i++)
  {
    std::cout << "Case " << i << std::endl;

    viskores::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();
    viskores::Id nVerts = dataSet.GetCoordinateSystem(0).GetNumberOfPoints();

    std::vector<viskores::Vec3f> vecs1, vecs2;
    createVectors(static_cast<std::size_t>(nVerts), i, vecs1, vecs2);

    viskores::cont::ArrayHandle<viskores::Vec3f> field1, field2;
    field1 = viskores::cont::make_ArrayHandle(vecs1, viskores::CopyFlag::On);
    field2 = viskores::cont::make_ArrayHandle(vecs2, viskores::CopyFlag::On);

    dataSet.AddPointField("vec1", field1);
    dataSet.AddPointField("vec2", field2);
    dataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem("vecA", field1));
    dataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem("vecB", field2));

    {
      std::cout << "  Both vectors as normal fields" << std::endl;
      viskores::filter::vector_analysis::DotProduct filter;
      filter.SetPrimaryField("vec1");
      filter.SetSecondaryField("vec2");
      viskores::cont::DataSet result = filter.Execute(dataSet);
      CheckResult(field1, field2, result);
    }

    {
      std::cout << "  First field as coordinates" << std::endl;
      viskores::filter::vector_analysis::DotProduct filter;
      filter.SetUseCoordinateSystemAsPrimaryField(true);
      filter.SetPrimaryCoordinateSystem(1);
      filter.SetSecondaryField("vec2");
      viskores::cont::DataSet result = filter.Execute(dataSet);
      CheckResult(field1, field2, result);
    }

    {
      std::cout << "  Second field as coordinates" << std::endl;
      viskores::filter::vector_analysis::DotProduct filter;
      filter.SetPrimaryField("vec1");
      filter.SetUseCoordinateSystemAsSecondaryField(true);
      filter.SetSecondaryCoordinateSystem(2);
      viskores::cont::DataSet result = filter.Execute(dataSet);
      CheckResult(field1, field2, result);
    }
  }
}
} // anonymous namespace

int UnitTestDotProductFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestDotProduct, argc, argv);
}
