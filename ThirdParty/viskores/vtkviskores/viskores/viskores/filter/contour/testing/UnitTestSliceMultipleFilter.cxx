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
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/contour/SliceMultiple.h>
#include <viskores/io/VTKDataSetWriter.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>
namespace
{
class SetPointValuesWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4);
  template <typename CoordinatesType, typename ScalarType, typename V3Type, typename V4Type>
  VISKORES_EXEC void operator()(const CoordinatesType& coordinates,
                                ScalarType& scalar,
                                V3Type& vec3,
                                V4Type& vec4) const
  {
    scalar =
      static_cast<ScalarType>((coordinates[2] * 3 * 3 + coordinates[1] * 3 + coordinates[0]) * 0.1);
    vec3 = { coordinates[0] * 0.1, coordinates[1] * 0.1, coordinates[2] * 0.1 };
    vec4 = {
      coordinates[0] * 0.1, coordinates[1] * 0.1, coordinates[2] * 0.1, coordinates[0] * 0.1
    };
    return;
  }
};

class SetCellValuesWorklet : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn, FieldInPoint, FieldOutCell, FieldOutCell, FieldOutCell);
  using ExecutionSignature = void(_2, _3, _4, _5);
  using InputDomain = _1;
  template <typename PointFieldVecType, typename ScalarType, typename V3Type, typename V4Type>
  VISKORES_EXEC void operator()(const PointFieldVecType& pointFieldVec,
                                ScalarType& scalar,
                                V3Type& vec3,
                                V4Type& vec4) const
  {
    //pointFieldVec has 8 values
    scalar = static_cast<ScalarType>(pointFieldVec[0]);
    vec3 = { pointFieldVec[0] * 0.1, pointFieldVec[1] * 0.1, pointFieldVec[2] * 0.1 };
    vec4 = {
      pointFieldVec[0] * 0.1, pointFieldVec[1] * 0.1, pointFieldVec[2] * 0.1, pointFieldVec[3] * 0.1
    };
    return;
  }
};

viskores::cont::DataSet MakeTestDatasetStructured3D()
{
  static constexpr viskores::Id xdim = 3, ydim = 3, zdim = 3;
  static const viskores::Id3 dim(xdim, ydim, zdim);
  viskores::cont::DataSet ds;
  ds = viskores::cont::DataSetBuilderUniform::Create(
    dim, viskores::Vec3f(-1.0f, -1.0f, -1.0f), viskores::Vec3f(1, 1, 1));
  viskores::cont::ArrayHandle<viskores::Float64> pointScalars;
  viskores::cont::ArrayHandle<viskores::Vec3f_64> pointV3;
  //a customized vector which is not in viskores::TypeListCommon{}
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 4>> pointV4;
  pointScalars.Allocate(xdim * ydim * zdim);
  pointV3.Allocate(xdim * ydim * zdim);
  pointV4.Allocate(xdim * ydim * zdim);
  viskores::cont::Invoker invoker;
  invoker(
    SetPointValuesWorklet{}, ds.GetCoordinateSystem().GetData(), pointScalars, pointV3, pointV4);
  ds.AddPointField("pointScalars", pointScalars);
  ds.AddPointField("pointV3", pointV3);
  ds.AddPointField("pointV4", pointV4);
  //adding cell data
  viskores::cont::ArrayHandle<viskores::Float64> cellScalars;
  viskores::cont::ArrayHandle<viskores::Vec3f_64> cellV3;
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 4>> cellV4;
  viskores::Id NumCells = ds.GetNumberOfCells();
  cellScalars.Allocate(NumCells);
  cellV3.Allocate(NumCells);
  cellV4.Allocate(NumCells);
  invoker(SetCellValuesWorklet{}, ds.GetCellSet(), pointScalars, cellScalars, cellV3, cellV4);
  ds.AddCellField("cellScalars", cellScalars);
  ds.AddCellField("cellV3", cellV3);
  ds.AddCellField("cellV4", cellV4);
  return ds;
}

void TestSliceMultipleFilter()
{
  auto ds = MakeTestDatasetStructured3D();
  viskores::Plane plane1({ 0, 0, 0 }, { 0, 0, 1 });
  viskores::Plane plane2({ 0, 0, 0 }, { 0, 1, 0 });
  viskores::Plane plane3({ 0, 0, 0 }, { 1, 0, 0 });
  viskores::filter::contour::SliceMultiple sliceMultiple;
  sliceMultiple.AddImplicitFunction(plane1);
  sliceMultiple.AddImplicitFunction(plane2);
  sliceMultiple.AddImplicitFunction(plane3);
  auto result = sliceMultiple.Execute(ds);
  VISKORES_TEST_ASSERT(result.GetNumberOfPoints() == 27,
                       "wrong number of points in merged data set");
  VISKORES_TEST_ASSERT(result.GetCoordinateSystem().GetData().GetNumberOfValues() == 27,
                       "wrong number of scalars in merged data set");
  viskores::cont::ArrayHandle<viskores::Float64> CheckingScalars;
  viskores::cont::ArrayHandle<viskores::Vec3f_64> CheckingV3;
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float64, 4>> CheckingV4;
  viskores::cont::Invoker invoker;
  invoker(SetPointValuesWorklet{},
          result.GetCoordinateSystem().GetData(),
          CheckingScalars,
          CheckingV3,
          CheckingV4);
  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(CheckingScalars, result.GetField("pointScalars").GetData()),
    "wrong scalar values");
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(CheckingV3, result.GetField("pointV3").GetData()),
                       "wrong pointV3 values");
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(CheckingV4, result.GetField("pointV4").GetData()),
                       "wrong pointV4 values");
  VISKORES_TEST_ASSERT(result.GetNumberOfCells() == 24, "wrong number of cells in merged data set");
}
} // anonymous namespace
int UnitTestSliceMultipleFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestSliceMultipleFilter, argc, argv);
}
