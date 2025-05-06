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

#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <ctime>
#include <random>
#include <vector>

namespace DataSetBuilderRectilinearNamespace
{

std::mt19937 g_RandomGenerator;

void ValidateDataSet(const viskores::cont::DataSet& ds,
                     int dim,
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

  //Make sure the bounds are correct.
  viskores::Bounds res = ds.GetCoordinateSystem().GetBounds();
  VISKORES_TEST_ASSERT(test_equal(bounds, res), "Bounds of coordinates do not match");
  if (dim == 1)
  {
    viskores::cont::CellSetStructured<1> cellSet;
    ds.GetCellSet().AsCellSet(cellSet);
    viskores::IdComponent shape = cellSet.GetCellShape();
    VISKORES_TEST_ASSERT(shape == viskores::CELL_SHAPE_LINE, "Wrong element type");
  }
  else if (dim == 2)
  {
    viskores::cont::CellSetStructured<2> cellSet;
    ds.GetCellSet().AsCellSet(cellSet);
    viskores::IdComponent shape = cellSet.GetCellShape();
    VISKORES_TEST_ASSERT(shape == viskores::CELL_SHAPE_QUAD, "Wrong element type");
  }
  else if (dim == 3)
  {
    viskores::cont::CellSetStructured<3> cellSet;
    ds.GetCellSet().AsCellSet(cellSet);
    viskores::IdComponent shape = cellSet.GetCellShape();
    VISKORES_TEST_ASSERT(shape == viskores::CELL_SHAPE_HEXAHEDRON, "Wrong element type");
  }
}

template <typename T>
void FillArray(std::vector<T>& arr, viskores::Id size, viskores::IdComponent fillMethod)
{
  arr.resize(static_cast<std::size_t>(size));
  arr[0] = T(0);
  for (size_t i = 1; i < static_cast<std::size_t>(size); i++)
  {
    T xi = static_cast<T>(i);

    switch (fillMethod)
    {
      case 0:
        break;
      case 1:
        xi /= static_cast<viskores::Float32>(size - 1);
        break;
      case 2:
        xi *= 2;
        break;
      case 3:
        xi *= 0.1f;
        break;
      case 4:
        xi *= xi;
        break;
      default:
        VISKORES_TEST_FAIL("Bad internal test state: invalid fill method.");
    }
    arr[i] = xi;
  }
}

template <typename T>
void RectilinearTests()
{
  const viskores::Id NUM_TRIALS = 10;
  const viskores::Id MAX_DIM_SIZE = 20;
  const viskores::Id NUM_FILL_METHODS = 5;

  viskores::cont::DataSetBuilderRectilinear dataSetBuilder;

  std::uniform_int_distribution<viskores::Id> randomDim(1, MAX_DIM_SIZE);
  std::uniform_int_distribution<viskores::IdComponent> randomFill(0, NUM_FILL_METHODS - 1);

  for (viskores::Id trial = 0; trial < NUM_TRIALS; trial++)
  {
    std::cout << "Trial " << trial << std::endl;

    viskores::cont::DataSet dataSet;

    viskores::Id3 dimensions(
      randomDim(g_RandomGenerator), randomDim(g_RandomGenerator), randomDim(g_RandomGenerator));
    std::cout << "Dimensions: " << dimensions << std::endl;

    viskores::IdComponent fillMethodX = randomFill(g_RandomGenerator);
    viskores::IdComponent fillMethodY = randomFill(g_RandomGenerator);
    viskores::IdComponent fillMethodZ = randomFill(g_RandomGenerator);
    std::cout << "Fill methods: [" << fillMethodX << "," << fillMethodY << "," << fillMethodZ << "]"
              << std::endl;

    std::vector<T> xCoordinates;
    std::vector<T> yCoordinates;
    std::vector<T> zCoordinates;
    FillArray(xCoordinates, dimensions[0], fillMethodX);
    FillArray(yCoordinates, dimensions[1], fillMethodY);
    FillArray(zCoordinates, dimensions[2], fillMethodZ);

    viskores::Id numPoints = 1, numCells = 1;
    viskores::Bounds bounds(0, 0, 0, 0, 0, 0);
    int ndims = 0;

    std::cout << "1D parameters" << std::endl;
    bounds.X = viskores::Range(xCoordinates.front(), xCoordinates.back());
    numPoints *= dimensions[0];
    if (dimensions[0] > 1)
    {
      numCells = dimensions[0] - 1;
      ndims += 1;
    }
    if (ndims)
    {
      std::vector<T> varP1D(static_cast<unsigned long>(numPoints));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numPoints); i++)
      {
        float fi = static_cast<float>(i);
        varP1D[i] = static_cast<T>(fi * 1.1f);
      }
      std::vector<T> varC1D(static_cast<unsigned long>(numCells));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numCells); i++)
      {
        float fi = static_cast<float>(i);
        varC1D[i] = static_cast<T>(fi * 1.1f);
      }
      std::cout << "  Create with std::vector" << std::endl;
      dataSet = dataSetBuilder.Create(xCoordinates);
      dataSet.AddPointField("pointvar", varP1D);
      dataSet.AddCellField("cellvar", varC1D);
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);
    }

    std::cout << "2D parameters" << std::endl;
    bounds.Y = viskores::Range(yCoordinates.front(), yCoordinates.back());
    numPoints *= dimensions[1];
    if (dimensions[1] > 1)
    {
      numCells *= dimensions[1] - 1;
      ndims += 1;
    }
    if (ndims)
    {
      std::vector<T> varP2D(static_cast<unsigned long>(numPoints));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numPoints); i++)
      {
        float fi = static_cast<float>(i);
        varP2D[i] = static_cast<T>(fi * 1.1f);
      }
      std::vector<T> varC2D(static_cast<unsigned long>(numCells));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numCells); i++)
      {
        float fi = static_cast<float>(i);
        varC2D[i] = static_cast<T>(fi * 1.1f);
      }
      std::cout << "  Create with std::vector" << std::endl;
      dataSet = dataSetBuilder.Create(xCoordinates, yCoordinates);
      dataSet.AddPointField("pointvar", varP2D);
      dataSet.AddCellField("cellvar", varC2D);
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);

      std::cout << "  Create with C array" << std::endl;
      dataSet = dataSetBuilder.Create(
        dimensions[0], dimensions[1], xCoordinates.data(), yCoordinates.data());
      dataSet.AddPointField("pointvar", varP2D.data(), numPoints);
      dataSet.AddCellField("cellvar", varC2D.data(), numCells);
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);

      std::cout << "  Create with ArrayHandle" << std::endl;
      dataSet = dataSetBuilder.Create(
        viskores::cont::make_ArrayHandle(xCoordinates, viskores::CopyFlag::Off),
        viskores::cont::make_ArrayHandle(yCoordinates, viskores::CopyFlag::Off));
      dataSet.AddPointField("pointvar",
                            viskores::cont::make_ArrayHandle(varP2D, viskores::CopyFlag::Off));
      dataSet.AddCellField("cellvar",
                           viskores::cont::make_ArrayHandle(varC2D, viskores::CopyFlag::Off));
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);
    }

    std::cout << "3D parameters" << std::endl;
    bounds.Z = viskores::Range(zCoordinates.front(), zCoordinates.back());
    numPoints *= dimensions[2];
    if (dimensions[2] > 1)
    {
      numCells *= dimensions[2] - 1;
      ndims += 1;
    }
    if (ndims)
    {
      std::vector<T> varP3D(static_cast<unsigned long>(numPoints));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numPoints); i++)
      {
        float fi = static_cast<float>(i);
        varP3D[i] = static_cast<T>(fi * 1.1f);
      }
      std::vector<T> varC3D(static_cast<unsigned long>(numCells));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numCells); i++)
      {
        float fi = static_cast<float>(i);
        varC3D[i] = static_cast<T>(fi * 1.1f);
      }

      std::cout << "  Create with std::vector" << std::endl;
      dataSet = dataSetBuilder.Create(xCoordinates, yCoordinates, zCoordinates);
      dataSet.AddPointField("pointvar", varP3D);
      dataSet.AddCellField("cellvar", varC3D);
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);

      std::cout << "  Create with C array" << std::endl;
      dataSet = dataSetBuilder.Create(dimensions[0],
                                      dimensions[1],
                                      dimensions[2],
                                      xCoordinates.data(),
                                      yCoordinates.data(),
                                      zCoordinates.data());
      dataSet.AddPointField("pointvar",
                            viskores::cont::make_ArrayHandle(varP3D, viskores::CopyFlag::Off));
      dataSet.AddCellField("cellvar",
                           viskores::cont::make_ArrayHandle(varC3D, viskores::CopyFlag::Off));
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);

      std::cout << "  Create with ArrayHandle" << std::endl;
      dataSet = dataSetBuilder.Create(
        viskores::cont::make_ArrayHandle(xCoordinates, viskores::CopyFlag::Off),
        viskores::cont::make_ArrayHandle(yCoordinates, viskores::CopyFlag::Off),
        viskores::cont::make_ArrayHandle(zCoordinates, viskores::CopyFlag::Off));
      dataSet.AddPointField("pointvar",
                            viskores::cont::make_ArrayHandle(varP3D, viskores::CopyFlag::Off));
      dataSet.AddCellField("cellvar",
                           viskores::cont::make_ArrayHandle(varC3D, viskores::CopyFlag::Off));
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);
    }
  }
}

void TestDataSetBuilderRectilinear()
{
  viskores::UInt32 seed = static_cast<viskores::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  RectilinearTests<viskores::Float32>();
  std::cout << "======== Float64 ==========================" << std::endl;
  RectilinearTests<viskores::Float64>();
}

} // namespace DataSetBuilderRectilinearNamespace

int UnitTestDataSetBuilderRectilinear(int argc, char* argv[])
{
  using namespace DataSetBuilderRectilinearNamespace;
  return viskores::cont::testing::Testing::Run(TestDataSetBuilderRectilinear, argc, argv);
}
