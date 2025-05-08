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
#include <viskores/io/VTKDataSetReader.h>

#include <string>

namespace
{

inline viskores::cont::DataSet readVTKDataSet(const std::string& fname)
{
  viskores::cont::DataSet ds;
  viskores::io::VTKDataSetReader reader(fname);
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VISKORES_TEST_FAIL(message.c_str());
  }

  return ds;
}

enum Format
{
  FORMAT_ASCII,
  FORMAT_BINARY
};

} // anonymous namespace

void TestReadingPolyData(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? viskores::cont::testing::Testing::DataPath("unstructured/simple_poly_ascii.vtk")
    : viskores::cont::testing::Testing::DataPath("unstructured/simple_poly_bin.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 6, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 8, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 8,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 6, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetSingleType<>>(),
                       "Incorrect cellset type");
}

void TestReadingPolyDataEmpty()
{
  viskores::cont::DataSet data =
    readVTKDataSet(viskores::cont::testing::Testing::DataPath("unstructured/empty_poly.vtk"));

  VISKORES_TEST_ASSERT(data.GetNumberOfPoints() == 8);
  VISKORES_TEST_ASSERT(data.GetNumberOfCells() == 0);
  VISKORES_TEST_ASSERT(data.GetCellSet().GetNumberOfPoints() == 8);
  VISKORES_TEST_ASSERT(data.GetNumberOfFields() == 2);
}

void TestReadingStructuredPoints(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? viskores::cont::testing::Testing::DataPath("uniform/simple_structured_points_ascii.vtk")
    : viskores::cont::testing::Testing::DataPath("uniform/simple_structured_points_bin.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 72, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 72,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 30, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>(),
                       "Incorrect cellset type");
}

void TestReadingStructuredPointsVisIt(Format format)
{
  VISKORES_TEST_ASSERT(format == FORMAT_ASCII);

  std::string testFileName =
    viskores::cont::testing::Testing::DataPath("uniform/simple_structured_points_visit_ascii.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 64, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 64,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 27, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>(),
                       "Incorrect cellset type");
}

void TestReadingUnstructuredGrid(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? viskores::cont::testing::Testing::DataPath("unstructured/simple_unstructured_ascii.vtk")
    : viskores::cont::testing::Testing::DataPath("unstructured/simple_unstructured_bin.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 26, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 26,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 15, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetExplicit<>>(),
                       "Incorrect cellset type");
}

void TestReadingV5Format(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? viskores::cont::testing::Testing::DataPath("unstructured/simple_unstructured_ascii_v5.vtk")
    : viskores::cont::testing::Testing::DataPath("unstructured/simple_unstructured_bin_v5.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 7, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 26, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 26,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 15, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetExplicit<>>(),
                       "Incorrect cellset type");

  for (viskores::IdComponent fieldIdx = 0; fieldIdx < ds.GetNumberOfFields(); ++fieldIdx)
  {
    viskores::cont::Field field = ds.GetField(fieldIdx);
    switch (field.GetAssociation())
    {
      case viskores::cont::Field::Association::Points:
        VISKORES_TEST_ASSERT(field.GetData().GetNumberOfValues() == ds.GetNumberOfPoints(),
                             "Field ",
                             field.GetName(),
                             " is the wrong size");
        break;
      case viskores::cont::Field::Association::Cells:
        VISKORES_TEST_ASSERT(field.GetData().GetNumberOfValues() == ds.GetNumberOfCells(),
                             "Field ",
                             field.GetName(),
                             " is the wrong size");
        break;
      default:
        // Could be any size.
        break;
    }
  }
}

void TestReadingUnstructuredGridEmpty()
{
  viskores::cont::DataSet data = readVTKDataSet(
    viskores::cont::testing::Testing::DataPath("unstructured/empty_unstructured.vtk"));

  VISKORES_TEST_ASSERT(data.GetNumberOfPoints() == 26);
  VISKORES_TEST_ASSERT(data.GetNumberOfCells() == 0);
  VISKORES_TEST_ASSERT(data.GetCellSet().GetNumberOfPoints() == 26);
  VISKORES_TEST_ASSERT(data.GetNumberOfFields() == 3);
}

void TestReadingUnstructuredPixels()
{
  // VTK has a special pixel cell type that is the same as a quad but with a different
  // vertex order. The reader must convert pixels to quads. Make sure this is happening
  // correctly. This file has only axis-aligned pixels.
  viskores::cont::DataSet ds =
    readVTKDataSet(viskores::cont::testing::Testing::DataPath("unstructured/pixel_cells.vtk"));

  viskores::cont::CellSetSingleType<> cellSet;
  ds.GetCellSet().AsCellSet(cellSet);
  viskores::cont::ArrayHandle<viskores::Vec3f_32> coords;
  ds.GetCoordinateSystem().GetData().AsArrayHandle(coords);

  for (viskores::Id cellIndex = 0; cellIndex < cellSet.GetNumberOfCells(); ++cellIndex)
  {
    VISKORES_TEST_ASSERT(cellSet.GetCellShape(cellIndex) == viskores::CELL_SHAPE_QUAD);

    constexpr viskores::IdComponent NUM_VERTS = 4;
    viskores::Vec<viskores::Id, NUM_VERTS> pointIndices;
    cellSet.GetIndices(cellIndex, pointIndices);
    viskores::Vec<viskores::Vec3f, NUM_VERTS> pointCoords;
    auto coordPortal = coords.ReadPortal();
    for (viskores::IdComponent vertIndex = 0; vertIndex < NUM_VERTS; ++vertIndex)
    {
      pointCoords[vertIndex] = coordPortal.Get(pointIndices[vertIndex]);
    }

    VISKORES_TEST_ASSERT(pointCoords[0][0] != pointCoords[1][0]);
    VISKORES_TEST_ASSERT(pointCoords[0][1] == pointCoords[1][1]);
    VISKORES_TEST_ASSERT(pointCoords[0][2] == pointCoords[1][2]);

    VISKORES_TEST_ASSERT(pointCoords[1][0] == pointCoords[2][0]);
    VISKORES_TEST_ASSERT(pointCoords[1][1] != pointCoords[2][1]);
    VISKORES_TEST_ASSERT(pointCoords[1][2] == pointCoords[2][2]);

    VISKORES_TEST_ASSERT(pointCoords[2][0] != pointCoords[3][0]);
    VISKORES_TEST_ASSERT(pointCoords[2][1] == pointCoords[3][1]);
    VISKORES_TEST_ASSERT(pointCoords[2][2] == pointCoords[3][2]);

    VISKORES_TEST_ASSERT(pointCoords[3][0] == pointCoords[0][0]);
    VISKORES_TEST_ASSERT(pointCoords[3][1] != pointCoords[0][1]);
    VISKORES_TEST_ASSERT(pointCoords[3][2] == pointCoords[0][2]);
  }
}

void TestReadingUnstructuredVoxels()
{
  // VTK has a special voxel cell type that is the same as a hexahedron but with a different
  // vertex order. The reader must convert voxels to hexahedra. Make sure this is happening
  // correctly. This file has only axis-aligned voxels.
  viskores::cont::DataSet ds =
    readVTKDataSet(viskores::cont::testing::Testing::DataPath("unstructured/voxel_cells.vtk"));

  viskores::cont::CellSetSingleType<> cellSet;
  ds.GetCellSet().AsCellSet(cellSet);
  viskores::cont::ArrayHandle<viskores::Vec3f_32> coords;
  ds.GetCoordinateSystem().GetData().AsArrayHandle(coords);

  for (viskores::Id cellIndex = 0; cellIndex < cellSet.GetNumberOfCells(); ++cellIndex)
  {
    VISKORES_TEST_ASSERT(cellSet.GetCellShape(cellIndex) == viskores::CELL_SHAPE_HEXAHEDRON);

    constexpr viskores::IdComponent NUM_VERTS = 8;
    viskores::Vec<viskores::Id, NUM_VERTS> pointIndices;
    cellSet.GetIndices(cellIndex, pointIndices);
    viskores::Vec<viskores::Vec3f, NUM_VERTS> pointCoords;
    auto coordPortal = coords.ReadPortal();
    for (viskores::IdComponent vertIndex = 0; vertIndex < NUM_VERTS; ++vertIndex)
    {
      pointCoords[vertIndex] = coordPortal.Get(pointIndices[vertIndex]);
    }

    VISKORES_TEST_ASSERT(pointCoords[0][0] != pointCoords[1][0]);
    VISKORES_TEST_ASSERT(pointCoords[0][1] == pointCoords[1][1]);
    VISKORES_TEST_ASSERT(pointCoords[0][2] == pointCoords[1][2]);

    VISKORES_TEST_ASSERT(pointCoords[1][0] == pointCoords[2][0]);
    VISKORES_TEST_ASSERT(pointCoords[1][1] != pointCoords[2][1]);
    VISKORES_TEST_ASSERT(pointCoords[1][2] == pointCoords[2][2]);

    VISKORES_TEST_ASSERT(pointCoords[2][0] != pointCoords[3][0]);
    VISKORES_TEST_ASSERT(pointCoords[2][1] == pointCoords[3][1]);
    VISKORES_TEST_ASSERT(pointCoords[2][2] == pointCoords[3][2]);

    VISKORES_TEST_ASSERT(pointCoords[3][0] == pointCoords[0][0]);
    VISKORES_TEST_ASSERT(pointCoords[3][1] != pointCoords[0][1]);
    VISKORES_TEST_ASSERT(pointCoords[3][2] == pointCoords[0][2]);

    VISKORES_TEST_ASSERT(pointCoords[0][0] == pointCoords[4][0]);
    VISKORES_TEST_ASSERT(pointCoords[0][1] == pointCoords[4][1]);
    VISKORES_TEST_ASSERT(pointCoords[0][2] != pointCoords[4][2]);

    VISKORES_TEST_ASSERT(pointCoords[1][0] == pointCoords[5][0]);
    VISKORES_TEST_ASSERT(pointCoords[1][1] == pointCoords[5][1]);
    VISKORES_TEST_ASSERT(pointCoords[1][2] != pointCoords[5][2]);

    VISKORES_TEST_ASSERT(pointCoords[2][0] == pointCoords[6][0]);
    VISKORES_TEST_ASSERT(pointCoords[2][1] == pointCoords[6][1]);
    VISKORES_TEST_ASSERT(pointCoords[2][2] != pointCoords[6][2]);

    VISKORES_TEST_ASSERT(pointCoords[3][0] == pointCoords[7][0]);
    VISKORES_TEST_ASSERT(pointCoords[3][1] == pointCoords[7][1]);
    VISKORES_TEST_ASSERT(pointCoords[3][2] != pointCoords[7][2]);
  }
}

void TestReadingUnstructuredGridVisIt(Format format)
{
  VISKORES_TEST_ASSERT(format == FORMAT_ASCII);

  std::string testFileName =
    viskores::cont::testing::Testing::DataPath("unstructured/simple_unstructured_visit_ascii.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 26, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 26,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 15, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetExplicit<>>(),
                       "Incorrect cellset type");
}

void TestReadingRectilinearGrid1(Format format)
{
  VISKORES_TEST_ASSERT(format == FORMAT_ASCII);

  std::string testFileName =
    viskores::cont::testing::Testing::DataPath("rectilinear/simple_rectilinear1_ascii.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 125, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 125,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 64, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>(),
                       "Incorrect cellset type");
}

void TestReadingRectilinearGrid2(Format format)
{
  VISKORES_TEST_ASSERT(format == FORMAT_ASCII);

  std::string testFileName =
    viskores::cont::testing::Testing::DataPath("rectilinear/simple_rectilinear2_ascii.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 24, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 24,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 6, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>(),
                       "Incorrect cellset type");
}

void TestReadingStructuredGridASCII()
{
  std::string testFileName =
    viskores::cont::testing::Testing::DataPath("curvilinear/simple_structured_ascii.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 6, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 6,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 2, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<2>>(),
                       "Incorrect cellset type");
}

void TestReadingStructuredGridBin()
{
  std::string testFileName =
    viskores::cont::testing::Testing::DataPath("curvilinear/simple_structured_bin.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 18, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 18,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 4, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>(),
                       "Incorrect cellset type");
}

void TestReadingRotate()
{
  std::string fusion = viskores::cont::testing::Testing::DataPath("uniform/rotate-vectors.vtk");
  viskores::cont::DataSet ds = readVTKDataSet(fusion.c_str());

  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>(),
                       "Incorrect cellset type");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 33 * 33 * 33, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 33 * 33 * 33,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.HasField("rotate"),
                       "The vtk file has a field 'rotate', but the dataset does not.");

  // Taken from Paraview + clicking Data Axes Grid:
  const viskores::cont::CoordinateSystem& coordinateSystem = ds.GetCoordinateSystem();
  viskores::Vec<viskores::Range, 3> ranges = coordinateSystem.GetRange();
  viskores::Range xRange = ranges[0];
  VISKORES_TEST_ASSERT(xRange.Min == -1);
  VISKORES_TEST_ASSERT(xRange.Max == 1);
  viskores::Range yRange = ranges[1];
  VISKORES_TEST_ASSERT(yRange.Min == -1);
  VISKORES_TEST_ASSERT(yRange.Max == 1);
  viskores::Range zRange = ranges[2];
  VISKORES_TEST_ASSERT(zRange.Min == -1);
  VISKORES_TEST_ASSERT(zRange.Max == 1);

  // Paraview Information Panel of this file:
  // rotate double [-1.29845, 1.25443], [-1.34447, 1.22820], [-0.32387, 0.33180]
  viskores::cont::Field vec = ds.GetField("rotate");
  VISKORES_TEST_ASSERT(vec.GetName() == "rotate");
  VISKORES_TEST_ASSERT(vec.IsPointField());
  const viskores::cont::ArrayHandle<viskores::Range>& vecRanges = vec.GetRange();
  VISKORES_TEST_ASSERT(vecRanges.GetNumberOfValues() == 3);
  auto vecRangesReadPortal = vecRanges.ReadPortal();

  auto xVecRange = vecRangesReadPortal.Get(0);
  VISKORES_TEST_ASSERT(test_equal(xVecRange.Min, -1.29845));
  VISKORES_TEST_ASSERT(test_equal(xVecRange.Max, 1.25443));

  auto yVecRange = vecRangesReadPortal.Get(1);

  VISKORES_TEST_ASSERT(test_equal(yVecRange.Min, -1.34447));
  VISKORES_TEST_ASSERT(test_equal(yVecRange.Max, 1.22820));

  auto zVecRange = vecRangesReadPortal.Get(2);
  VISKORES_TEST_ASSERT(test_equal(zVecRange.Min, -0.32387));
  VISKORES_TEST_ASSERT(test_equal(zVecRange.Max, 0.33180));
}

void TestReadingKitchen()
{
  std::string fusion = viskores::cont::testing::Testing::DataPath("curvilinear/kitchen.vtk");
  viskores::cont::DataSet ds = readVTKDataSet(fusion.c_str());

  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>(),
                       "Incorrect cellset type");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 28 * 24 * 17, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 28 * 24 * 17,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.HasField("h1"),
                       "The vtk file has a field 'h1', but the dataset does not.");
  VISKORES_TEST_ASSERT(ds.HasField("velocity"),
                       "The vtk file has a field 'velocity', but the dataset does not.");

  // Paraview Information Panel of this file:
  // Bounds: [0.01, 7], [0.01, 5], [0.01, 2.5]
  const viskores::cont::CoordinateSystem& coordinateSystem = ds.GetCoordinateSystem();
  viskores::Vec<viskores::Range, 3> ranges = coordinateSystem.GetRange();
  viskores::Range xRange = ranges[0];
  VISKORES_TEST_ASSERT(test_equal(xRange.Min, 0.01));
  VISKORES_TEST_ASSERT(test_equal(xRange.Max, 7));
  viskores::Range yRange = ranges[1];
  VISKORES_TEST_ASSERT(test_equal(yRange.Min, 0.01));
  VISKORES_TEST_ASSERT(test_equal(yRange.Max, 5));
  viskores::Range zRange = ranges[2];
  VISKORES_TEST_ASSERT(test_equal(zRange.Min, 0.01));
  VISKORES_TEST_ASSERT(test_equal(zRange.Max, 2.5));

  // h1 float [0, 26823.6]
  viskores::cont::Field h1 = ds.GetField("h1");
  VISKORES_TEST_ASSERT(h1.GetName() == "h1");
  VISKORES_TEST_ASSERT(h1.IsPointField());
  const viskores::cont::ArrayHandle<viskores::Range>& h1Ranges = h1.GetRange();
  VISKORES_TEST_ASSERT(h1Ranges.GetNumberOfValues() == 1);
  auto h1RangesReadPortal = h1Ranges.ReadPortal();

  auto h1Range = h1RangesReadPortal.Get(0);
  VISKORES_TEST_ASSERT(test_equal(h1Range.Min, 0));
  VISKORES_TEST_ASSERT(test_equal(h1Range.Max, 26823.6));

  // velocity float [-0.34942, 0.26521], [-0.31407, 0.31543], [-0.45072, 0.28649]
  viskores::cont::Field vec = ds.GetField("velocity");
  VISKORES_TEST_ASSERT(vec.GetName() == "velocity");
  VISKORES_TEST_ASSERT(vec.IsPointField());
  const viskores::cont::ArrayHandle<viskores::Range>& vecRanges = vec.GetRange();
  VISKORES_TEST_ASSERT(vecRanges.GetNumberOfValues() == 3);
  auto vecRangesReadPortal = vecRanges.ReadPortal();

  auto xVecRange = vecRangesReadPortal.Get(0);
  VISKORES_TEST_ASSERT(test_equal(xVecRange.Min, -0.34942));
  VISKORES_TEST_ASSERT(test_equal(xVecRange.Max, 0.26521));

  auto yVecRange = vecRangesReadPortal.Get(1);

  VISKORES_TEST_ASSERT(test_equal(yVecRange.Min, -0.31407));
  VISKORES_TEST_ASSERT(test_equal(yVecRange.Max, 0.31543));

  auto zVecRange = vecRangesReadPortal.Get(2);
  VISKORES_TEST_ASSERT(test_equal(zVecRange.Min, -0.45072));
  VISKORES_TEST_ASSERT(test_equal(zVecRange.Max, 0.28649));
}

void TestSkppingStringFields(Format format)
{
  std::string testFileName = (format == FORMAT_ASCII)
    ? viskores::cont::testing::Testing::DataPath(
        "uniform/simple_structured_points_strings_ascii.vtk")
    : viskores::cont::testing::Testing::DataPath(
        "uniform/simple_structured_points_strings_bin.vtk");

  viskores::cont::DataSet ds = readVTKDataSet(testFileName);

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 72, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 72,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 30, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>(),
                       "Incorrect cellset type");
}

void TestReadingVTKDataSet()
{
  std::cout << "Test reading VTK Polydata file in ASCII" << std::endl;
  TestReadingPolyData(FORMAT_ASCII);
  std::cout << "Test reading VTK Polydata file in BINARY" << std::endl;
  TestReadingPolyData(FORMAT_BINARY);
  std::cout << "Test reading VTK Polydata with no cells" << std::endl;
  TestReadingPolyDataEmpty();
  std::cout << "Test reading VTK StructuredPoints file in ASCII" << std::endl;
  TestReadingStructuredPoints(FORMAT_ASCII);

  std::cout << "Test reading VTK StructuredPoints file in BINARY" << std::endl;
  TestReadingStructuredPoints(FORMAT_BINARY);
  std::cout << "Test reading VTK UnstructuredGrid file in ASCII" << std::endl;
  TestReadingUnstructuredGrid(FORMAT_ASCII);
  std::cout << "Test reading VTK UnstructuredGrid file in BINARY" << std::endl;
  TestReadingUnstructuredGrid(FORMAT_BINARY);
  std::cout << "Test reading VTK UnstructuredGrid with no cells" << std::endl;
  TestReadingUnstructuredGridEmpty();
  std::cout << "Test reading VTK UnstructuredGrid with pixels" << std::endl;
  TestReadingUnstructuredPixels();
  std::cout << "Test reading VTK UnstructuredGrid with voxels" << std::endl;
  TestReadingUnstructuredVoxels();

  std::cout << "Test reading VTK RectilinearGrid file in ASCII" << std::endl;
  TestReadingRectilinearGrid1(FORMAT_ASCII);
  TestReadingRectilinearGrid2(FORMAT_ASCII);

  std::cout << "Test reading VTK/VisIt StructuredPoints file in ASCII" << std::endl;
  TestReadingStructuredPointsVisIt(FORMAT_ASCII);
  std::cout << "Test reading VTK/VisIt UnstructuredGrid file in ASCII" << std::endl;
  TestReadingUnstructuredGridVisIt(FORMAT_ASCII);

  std::cout << "Test reading VTK StructuredGrid file in ASCII" << std::endl;
  TestReadingStructuredGridASCII();
  std::cout << "Test reading VTK StructuredGrid file in BINARY" << std::endl;
  TestReadingStructuredGridBin();
  std::cout << "Test reading rotate" << std::endl;
  TestReadingRotate();
  std::cout << "Test reading kitchen" << std::endl;
  TestReadingKitchen();

  std::cout << "Test skipping string fields in ASCII files" << std::endl;
  TestSkppingStringFields(FORMAT_ASCII);
  std::cout << "Test skipping string fields in BINARY files" << std::endl;
  TestSkppingStringFields(FORMAT_BINARY);

  std::cout << "Test reading v5 file format in ASCII" << std::endl;
  TestReadingV5Format(FORMAT_ASCII);
  std::cout << "Test reading v5 file format in BINARY" << std::endl;
  TestReadingV5Format(FORMAT_BINARY);
}

int UnitTestVTKDataSetReader(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestReadingVTKDataSet, argc, argv);
}
