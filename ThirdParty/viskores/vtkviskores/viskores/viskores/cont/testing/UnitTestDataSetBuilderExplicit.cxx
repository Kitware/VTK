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
#include <viskores/cont/testing/ExplicitTestData.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace DataSetBuilderExplicitNamespace
{

template <typename T>
viskores::Bounds ComputeBounds(std::size_t numPoints, const T* coords)
{
  viskores::Bounds bounds;

  for (std::size_t i = 0; i < numPoints; i++)
  {
    bounds.Include(viskores::Vec<T, 3>(coords[i * 3 + 0], coords[i * 3 + 1], coords[i * 3 + 2]));
  }

  return bounds;
}

void ValidateDataSet(const viskores::cont::DataSet& ds,
                     viskores::Id numPoints,
                     viskores::Id numCells,
                     const viskores::Bounds& bounds)
{
  //Verify basics..
  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Wrong number of fields.");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems.");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == numPoints, "Wrong number of coordinates.");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == numCells, "Wrong number of cells.");

  // test various field-getting methods and associations
  try
  {
    ds.GetCellField("cellvar");
  }
  catch (...)
  {
    VISKORES_TEST_FAIL("Failed to get field 'cellvar' with Association::Cells.");
  }

  try
  {
    ds.GetPointField("pointvar");
  }
  catch (...)
  {
    VISKORES_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  //Make sure bounds are correct.
  viskores::Bounds computedBounds = ds.GetCoordinateSystem().GetBounds();
  VISKORES_TEST_ASSERT(test_equal(bounds, computedBounds), "Bounds of coordinates do not match");
}

template <typename T>
std::vector<T> createVec(std::size_t n, const T* data)
{
  std::vector<T> vec(n);
  for (std::size_t i = 0; i < n; i++)
  {
    vec[i] = data[i];
  }
  return vec;
}

template <typename T>
viskores::cont::ArrayHandle<T> createAH(std::size_t n, const T* data)
{
  return viskores::cont::make_ArrayHandle(
    data, static_cast<viskores::Id>(n), viskores::CopyFlag::On);
}

template <typename T>
viskores::cont::DataSet CreateDataSetArr(bool useSeparatedCoords,
                                         std::size_t numPoints,
                                         const T* coords,
                                         std::size_t numCells,
                                         std::size_t numConn,
                                         const viskores::Id* conn,
                                         const viskores::IdComponent* indices,
                                         const viskores::UInt8* shape)
{
  viskores::cont::DataSet dataSet;
  viskores::cont::DataSetBuilderExplicit dsb;
  float f = 0.0f;
  if (useSeparatedCoords)
  {
    std::vector<T> xvals(numPoints), yvals(numPoints), zvals(numPoints);
    std::vector<T> varP(numPoints), varC(numCells);
    std::vector<viskores::UInt8> shapevals(numCells);
    std::vector<viskores::IdComponent> indicesvals(numCells);
    std::vector<viskores::Id> connvals(numConn);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      xvals[i] = coords[i * 3 + 0];
      yvals[i] = coords[i * 3 + 1];
      zvals[i] = coords[i * 3 + 2];
      varP[i] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i] = static_cast<T>(f * 1.1f);
      shapevals[i] = shape[i];
      indicesvals[i] = indices[i];
    }
    for (std::size_t i = 0; i < numConn; i++)
    {
      connvals[i] = conn[i];
    }
    dataSet = dsb.Create(xvals, yvals, zvals, shapevals, indicesvals, connvals);

    viskores::cont::ArrayHandle<T> P =
      viskores::cont::make_ArrayHandle(varP, viskores::CopyFlag::On);
    viskores::cont::ArrayHandle<T> C =
      viskores::cont::make_ArrayHandle(varC, viskores::CopyFlag::On);
    dataSet.AddPointField("pointvar", P);
    dataSet.AddCellField("cellvar", C);
    return dataSet;
  }
  else
  {
    std::vector<viskores::Vec<T, 3>> tmp(numPoints);
    std::vector<viskores::Vec<T, 1>> varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      tmp[i][0] = coords[i * 3 + 0];
      tmp[i][1] = coords[i * 3 + 1];
      tmp[i][2] = coords[i * 3 + 2];
      varP[i][0] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i][0] = static_cast<T>(f * 1.1f);
    }
    viskores::cont::ArrayHandle<viskores::Vec<T, 3>> pts =
      viskores::cont::make_ArrayHandle(tmp, viskores::CopyFlag::On);
    dataSet = dsb.Create(
      pts, createAH(numCells, shape), createAH(numCells, indices), createAH(numConn, conn));
    dataSet.AddPointField("pointvar", varP);
    dataSet.AddCellField("cellvar", varC);
    return dataSet;
  }
}

template <typename T>
viskores::cont::DataSet CreateDataSetVec(bool useSeparatedCoords,
                                         std::size_t numPoints,
                                         const T* coords,
                                         std::size_t numCells,
                                         std::size_t numConn,
                                         const viskores::Id* conn,
                                         const viskores::IdComponent* indices,
                                         const viskores::UInt8* shape)
{
  viskores::cont::DataSet dataSet;
  viskores::cont::DataSetBuilderExplicit dsb;

  float f = 0.0f;
  if (useSeparatedCoords)
  {
    std::vector<T> X(numPoints), Y(numPoints), Z(numPoints), varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      X[i] = coords[i * 3 + 0];
      Y[i] = coords[i * 3 + 1];
      Z[i] = coords[i * 3 + 2];
      varP[i] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i] = static_cast<T>(f * 1.1f);
    }
    dataSet = dsb.Create(
      X, Y, Z, createVec(numCells, shape), createVec(numCells, indices), createVec(numConn, conn));
    dataSet.AddPointField("pointvar", varP);
    dataSet.AddCellField("cellvar", varC);
    return dataSet;
  }
  else
  {
    std::vector<viskores::Vec<T, 3>> pts(numPoints);
    std::vector<viskores::Vec<T, 1>> varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      pts[i][0] = coords[i * 3 + 0];
      pts[i][1] = coords[i * 3 + 1];
      pts[i][2] = coords[i * 3 + 2];
      varP[i][0] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i][0] = static_cast<T>(f * 1.1f);
    }
    dataSet = dsb.Create(
      pts, createVec(numCells, shape), createVec(numCells, indices), createVec(numConn, conn));
    dataSet.AddPointField("pointvar", varP);
    dataSet.AddCellField("cellvar", varC);
    return dataSet;
  }
}

#define TEST_DATA(num)                                      \
  viskores::cont::testing::ExplicitData##num::numPoints,    \
    viskores::cont::testing::ExplicitData##num::coords,     \
    viskores::cont::testing::ExplicitData##num::numCells,   \
    viskores::cont::testing::ExplicitData##num::numConn,    \
    viskores::cont::testing::ExplicitData##num::conn,       \
    viskores::cont::testing::ExplicitData##num::numIndices, \
    viskores::cont::testing::ExplicitData##num::shapes
#define TEST_NUMS(num)                                   \
  viskores::cont::testing::ExplicitData##num::numPoints, \
    viskores::cont::testing::ExplicitData##num::numCells
#define TEST_BOUNDS(num)                                 \
  viskores::cont::testing::ExplicitData##num::numPoints, \
    viskores::cont::testing::ExplicitData##num::coords

void TestDataSetBuilderExplicit()
{
  viskores::cont::DataSet ds;
  viskores::Bounds bounds;

  //Iterate over organization of coordinates.
  for (int i = 0; i < 2; i++)
  {
    //Test ExplicitData0
    bounds = ComputeBounds(TEST_BOUNDS(0));
    ds = CreateDataSetArr(i == 0, TEST_DATA(0));
    ValidateDataSet(ds, TEST_NUMS(0), bounds);
    ds = CreateDataSetVec(i == 0, TEST_DATA(0));
    ValidateDataSet(ds, TEST_NUMS(0), bounds);

    //Test ExplicitData1
    bounds = ComputeBounds(TEST_BOUNDS(1));
    ds = CreateDataSetArr(i == 0, TEST_DATA(1));
    ValidateDataSet(ds, TEST_NUMS(1), bounds);
    ds = CreateDataSetVec(i == 0, TEST_DATA(1));
    ValidateDataSet(ds, TEST_NUMS(1), bounds);

    //Test ExplicitData2
    bounds = ComputeBounds(TEST_BOUNDS(2));
    ds = CreateDataSetArr(i == 0, TEST_DATA(2));
    ValidateDataSet(ds, TEST_NUMS(2), bounds);
    ds = CreateDataSetVec(i == 0, TEST_DATA(2));
    ValidateDataSet(ds, TEST_NUMS(2), bounds);
  }
}

} // namespace DataSetBuilderExplicitNamespace

int UnitTestDataSetBuilderExplicit(int argc, char* argv[])
{
  using namespace DataSetBuilderExplicitNamespace;
  return viskores::cont::testing::Testing::Run(TestDataSetBuilderExplicit, argc, argv);
}
