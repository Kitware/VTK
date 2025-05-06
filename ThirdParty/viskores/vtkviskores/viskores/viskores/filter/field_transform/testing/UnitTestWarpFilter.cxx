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
#include <viskores/filter/field_transform/Warp.h>

#include <vector>

namespace
{

constexpr viskores::Id DIM = 5;

template <typename T>
viskores::cont::DataSet MakeWarpTestDataSet()
{
  using vecType = viskores::Vec<T, 3>;
  viskores::cont::DataSet dataSet;

  std::vector<vecType> coordinates;
  std::vector<vecType> vec1;
  std::vector<T> scalarFactor;
  std::vector<vecType> vec2;
  for (viskores::Id j = 0; j < DIM; ++j)
  {
    T z = static_cast<T>(j) / static_cast<T>(DIM - 1);
    for (viskores::Id i = 0; i < DIM; ++i)
    {
      T x = static_cast<T>(i) / static_cast<T>(DIM - 1);
      T y = (x * x + z * z) / static_cast<T>(2.0);
      coordinates.push_back(viskores::make_Vec(x, y, z));
      vec1.push_back(viskores::make_Vec(x, y, y));
      scalarFactor.push_back(static_cast<T>(j * DIM + i));
      vec2.push_back(viskores::make_Vec<T>(0, 0, static_cast<T>(j * DIM + i)));
    }
  }

  dataSet.AddCoordinateSystem(
    viskores::cont::make_CoordinateSystem("coordinates", coordinates, viskores::CopyFlag::On));

  dataSet.AddPointField("vec1", vec1);
  dataSet.AddPointField("scalarfactor", scalarFactor);
  dataSet.AddPointField("vec2", vec2);

  vecType normal =
    viskores::make_Vec<T>(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0));
  viskores::cont::ArrayHandleConstant<vecType> vectorAH =
    viskores::cont::make_ArrayHandleConstant(normal, DIM * DIM);
  dataSet.AddPointField("normal", vectorAH);

  return dataSet;
}

void CheckResult(const viskores::filter::field_transform::Warp& filter,
                 const viskores::cont::DataSet& result)
{
  VISKORES_TEST_ASSERT(result.HasPointField(filter.GetOutputFieldName()));
  using vecType = viskores::Vec3f;
  viskores::cont::ArrayHandle<vecType> outputArray;
  result.GetPointField(filter.GetOutputFieldName()).GetData().AsArrayHandle(outputArray);
  auto outPortal = outputArray.ReadPortal();

  viskores::cont::ArrayHandle<viskores::FloatDefault> sfArray;
  result.GetPointField("scalarfactor").GetData().AsArrayHandle(sfArray);
  auto sfPortal = sfArray.ReadPortal();

  for (viskores::Id j = 0; j < DIM; ++j)
  {
    viskores::FloatDefault z =
      static_cast<viskores::FloatDefault>(j) / static_cast<viskores::FloatDefault>(DIM - 1);
    for (viskores::Id i = 0; i < DIM; ++i)
    {
      viskores::FloatDefault x =
        static_cast<viskores::FloatDefault>(i) / static_cast<viskores::FloatDefault>(DIM - 1);
      viskores::FloatDefault y = (x * x + z * z) / static_cast<viskores::FloatDefault>(2.0);
      viskores::FloatDefault targetZ = filter.GetUseCoordinateSystemAsField()
        ? z + static_cast<viskores::FloatDefault>(2 * sfPortal.Get(j * DIM + i))
        : y + static_cast<viskores::FloatDefault>(2 * sfPortal.Get(j * DIM + i));
      auto point = outPortal.Get(j * DIM + i);
      VISKORES_TEST_ASSERT(test_equal(point[0], x), "Wrong result of x value for warp scalar");
      VISKORES_TEST_ASSERT(test_equal(point[1], y), "Wrong result of y value for warp scalar");
      VISKORES_TEST_ASSERT(test_equal(point[2], targetZ),
                           "Wrong result of z value for warp scalar");
    }
  }
}

void TestWarpFilter()
{
  std::cout << "Testing Warp filter" << std::endl;
  viskores::cont::DataSet ds = MakeWarpTestDataSet<viskores::FloatDefault>();
  viskores::FloatDefault scale = 2;

  {
    std::cout << "   First field as coordinates" << std::endl;
    viskores::filter::field_transform::Warp filter;
    filter.SetScaleFactor(scale);
    filter.SetUseCoordinateSystemAsField(true);
    filter.SetDirectionField("normal");
    filter.SetScaleField("scalarfactor");
    viskores::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }

  {
    std::cout << "   First field as a vector" << std::endl;
    viskores::filter::field_transform::Warp filter;
    filter.SetScaleFactor(scale);
    filter.SetActiveField("vec1");
    filter.SetDirectionField("normal");
    filter.SetScaleField("scalarfactor");
    viskores::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }

  {
    std::cout << "   Constant direction (warp scalar)" << std::endl;
    viskores::filter::field_transform::Warp filter;
    filter.SetScaleFactor(scale);
    filter.SetUseCoordinateSystemAsField(true);
    filter.SetConstantDirection({ 0.0, 0.0, 1.0 });
    filter.SetScaleField("scalarfactor");
    viskores::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }

  {
    std::cout << "   Constant scale (warp vector)" << std::endl;
    viskores::filter::field_transform::Warp filter;
    filter.SetScaleFactor(scale);
    filter.SetActiveField("vec1");
    filter.SetDirectionField("vec2");
    viskores::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }
}
}

int UnitTestWarpFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestWarpFilter, argc, argv);
}
