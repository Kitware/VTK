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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/FileUtils.h>
#include <viskores/io/VTKDataSetReader.h>

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif

namespace
{

viskores::Id GetCurrentProcessId()
{
#ifdef _MSC_VER
  return _getpid();
#else
  return getpid();
#endif
}

std::string MakeScopedVTKTestFileName(const std::string& fileName)
{
  static std::atomic<viskores::Id> FileCounter{ 0 };

  std::size_t extensionPos = fileName.rfind('.');
  std::string stem =
    (extensionPos == std::string::npos) ? fileName : fileName.substr(0, extensionPos);
  std::string extension =
    (extensionPos == std::string::npos) ? std::string() : fileName.substr(extensionPos);

  std::string uniqueName = stem + "_" + std::to_string(GetCurrentProcessId()) + "_" +
    std::to_string(FileCounter.fetch_add(1, std::memory_order_relaxed)) + extension;

  const char* tempDir = std::getenv("TMPDIR");
  if ((tempDir == nullptr) || (tempDir[0] == '\0'))
  {
    tempDir = std::getenv("TEMP");
  }
  if ((tempDir == nullptr) || (tempDir[0] == '\0'))
  {
    tempDir = std::getenv("TMP");
  }
  if ((tempDir == nullptr) || (tempDir[0] == '\0'))
  {
    tempDir = ".";
  }

  return viskores::io::MergePaths(tempDir, uniqueName);
}

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

class ScopedVTKTestFile
{
public:
  ScopedVTKTestFile(const std::string& fileName, const std::string& contents)
    : FileName(MakeScopedVTKTestFileName(fileName))
  {
    // Keep generated fixtures out of the source tree and away from the data
    // submodule so these regressions stay self-contained.
    std::ofstream out(this->FileName);
    VISKORES_TEST_ASSERT(out.is_open(), "Could not create ", this->FileName);
    out << contents;
    out.close();
    VISKORES_TEST_ASSERT(out.good(), "Could not write ", this->FileName);
  }

  ~ScopedVTKTestFile() { std::remove(this->FileName.c_str()); }

  const std::string& GetFileName() const { return this->FileName; }

private:
  std::string FileName;
};

viskores::cont::ArrayHandle<viskores::Vec3f> GetCoordinateArray(const viskores::cont::DataSet& ds)
{
  viskores::cont::ArrayHandle<viskores::Vec3f> coords;
  viskores::cont::ArrayCopyShallowIfPossible(ds.GetCoordinateSystem().GetData(), coords);
  return coords;
}

void CheckStructuredCellSet(const viskores::cont::DataSet& ds,
                            viskores::IdComponent expectedDimension,
                            viskores::Id expectedPoints,
                            viskores::Id expectedCells)
{
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == expectedPoints, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == expectedPoints,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == expectedCells, "Incorrect number of cells");

  switch (expectedDimension)
  {
    case 1:
      VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<1>>(),
                           "Incorrect cellset type");
      break;
    case 2:
      VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<2>>(),
                           "Incorrect cellset type");
      break;
    case 3:
      VISKORES_TEST_ASSERT(ds.GetCellSet().IsType<viskores::cont::CellSetStructured<3>>(),
                           "Incorrect cellset type");
      break;
    default:
      VISKORES_TEST_FAIL("Unsupported expected structured dimension ", expectedDimension);
  }
}

void CheckStructuredQuadCell(const viskores::cont::DataSet& ds,
                             const std::array<viskores::Vec3f, 4>& expectedCoords)
{
  viskores::cont::CellSetStructured<2> cellSet;
  ds.GetCellSet().AsCellSet(cellSet);

  viskores::Id pointIds[4];
  cellSet.GetCellPointIds(0, pointIds);
  auto coordsArray = GetCoordinateArray(ds);
  auto coords = coordsArray.ReadPortal();
  for (std::size_t index = 0; index < expectedCoords.size(); ++index)
  {
    VISKORES_TEST_ASSERT(test_equal(coords.Get(pointIds[index]), expectedCoords[index]),
                         "Unexpected quad point coordinates");
  }
}

void CheckStructuredLineCell(const viskores::cont::DataSet& ds,
                             const std::array<viskores::Vec3f, 2>& expectedCoords)
{
  viskores::cont::CellSetStructured<1> cellSet;
  ds.GetCellSet().AsCellSet(cellSet);

  viskores::Id pointIds[2];
  cellSet.GetCellPointIds(0, pointIds);
  auto coordsArray = GetCoordinateArray(ds);
  auto coords = coordsArray.ReadPortal();
  for (std::size_t index = 0; index < expectedCoords.size(); ++index)
  {
    VISKORES_TEST_ASSERT(test_equal(coords.Get(pointIds[index]), expectedCoords[index]),
                         "Unexpected line point coordinates");
  }
}

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

void TestPermutingMultiComponentCellData()
{
  ScopedVTKTestFile vtkFile("vtk_reader_triangle_strip_cell_vectors.vtk",
                            "# vtk DataFile Version 3.0\n"
                            "triangle strip cell vectors\n"
                            "ASCII\n"
                            "DATASET POLYDATA\n"
                            "POINTS 4 float\n"
                            "0 0 0 1 0 0 0 1 0 1 1 0\n"
                            "TRIANGLE_STRIPS 1 5\n"
                            "4 0 1 2 3\n"
                            "CELL_DATA 1\n"
                            "VECTORS cellvec float\n"
                            "1 2 3\n");

  viskores::cont::DataSet ds = readVTKDataSet(vtkFile.GetFileName());

  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 2, "Triangle strip should expand into two cells");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetCellShape(0) == viskores::CELL_SHAPE_TRIANGLE,
                       "Triangle strip should expand into triangles");
  VISKORES_TEST_ASSERT(ds.HasField("cellvec"), "Missing expanded cell field");

  auto field = ds.GetField("cellvec");
  VISKORES_TEST_ASSERT(field.IsCellField(), "Expanded field should be a cell field");
  VISKORES_TEST_ASSERT(field.GetData().GetNumberOfValues() == ds.GetNumberOfCells(),
                       "Expanded field has the wrong tuple count");
  VISKORES_TEST_ASSERT(field.GetData().GetNumberOfComponentsFlat() == 3,
                       "Expanded field has the wrong number of components");

  auto vectors = field.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f_32>>();
  auto portal = vectors.ReadPortal();
  VISKORES_TEST_ASSERT(test_equal(portal.Get(0), viskores::Vec3f_32(1.0f, 2.0f, 3.0f)),
                       "First expanded vector is incorrect");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(1), viskores::Vec3f_32(1.0f, 2.0f, 3.0f)),
                       "Second expanded vector is incorrect");
}

void TestReadingPolyLineCell()
{
  ScopedVTKTestFile vtkFile("vtk_reader_polyline_cell.vtk",
                            "# vtk DataFile Version 3.0\n"
                            "polyline cell\n"
                            "ASCII\n"
                            "DATASET POLYDATA\n"
                            "POINTS 3 float\n"
                            "0 0 0 1 0 0 2 0 0\n"
                            "LINES 1 4\n"
                            "3 0 1 2\n"
                            "CELL_DATA 1\n"
                            "SCALARS cellval float 1\n"
                            "LOOKUP_TABLE default\n"
                            "7\n");

  viskores::cont::DataSet ds = readVTKDataSet(vtkFile.GetFileName());

  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 1, "Polyline should be preserved as one cell");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetCellShape(0) == viskores::CELL_SHAPE_POLY_LINE,
                       "Incorrect polyline cell shape");
  VISKORES_TEST_ASSERT(ds.HasField("cellval"), "Missing polyline cell field");
  VISKORES_TEST_ASSERT(ds.GetField("cellval").GetData().GetNumberOfValues() == 1,
                       "Polyline cell field has the wrong tuple count");
}

void TestSkippingMismatchedFieldData()
{
  ScopedVTKTestFile vtkFile("vtk_reader_field_mismatch.vtk",
                            "# vtk DataFile Version 3.0\n"
                            "field mismatch\n"
                            "ASCII\n"
                            "DATASET STRUCTURED_POINTS\n"
                            "DIMENSIONS 2 2 1\n"
                            "ORIGIN 0 0 0\n"
                            "SPACING 1 1 1\n"
                            "POINT_DATA 4\n"
                            "FIELD FieldData 2\n"
                            "bad 1 3 float\n"
                            "1 2 3\n"
                            "good 1 4 float\n"
                            "10 20 30 40\n");

  viskores::cont::DataSet ds = readVTKDataSet(vtkFile.GetFileName());

  VISKORES_TEST_ASSERT(!ds.HasField("bad"), "Mismatched field should be skipped");
  VISKORES_TEST_ASSERT(ds.HasField("good"), "Valid field after skipped field was not read");

  viskores::cont::ArrayHandle<viskores::Float32> values =
    ds.GetField("good").GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();
  auto portal = values.ReadPortal();
  VISKORES_TEST_ASSERT(values.GetNumberOfValues() == 4, "Valid field has the wrong size");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(0), 10.0f) && test_equal(portal.Get(1), 20.0f) &&
                         test_equal(portal.Get(2), 30.0f) && test_equal(portal.Get(3), 40.0f),
                       "Valid field values are incorrect");
}

void TestStructuredPointsDegenerateDimensions()
{
  {
    ScopedVTKTestFile vtkFile("vtk_reader_structured_points_yz.vtk",
                              "# vtk DataFile Version 3.0\n"
                              "structured points yz\n"
                              "ASCII\n"
                              "DATASET STRUCTURED_POINTS\n"
                              "DIMENSIONS 1 2 2\n"
                              "ORIGIN 0 0 0\n"
                              "SPACING 1 1 1\n"
                              "POINT_DATA 4\n"
                              "SCALARS vals float 1\n"
                              "LOOKUP_TABLE default\n"
                              "0 1 2 3\n");

    viskores::cont::DataSet ds = readVTKDataSet(vtkFile.GetFileName());
    CheckStructuredCellSet(ds, 2, 4, 1);
    CheckStructuredQuadCell(ds,
                            std::array<viskores::Vec3f, 4>{ viskores::Vec3f(0.0f, 0.0f, 0.0f),
                                                            viskores::Vec3f(0.0f, 1.0f, 0.0f),
                                                            viskores::Vec3f(0.0f, 1.0f, 1.0f),
                                                            viskores::Vec3f(0.0f, 0.0f, 1.0f) });
  }

  {
    ScopedVTKTestFile vtkFile("vtk_reader_structured_points_z.vtk",
                              "# vtk DataFile Version 3.0\n"
                              "structured points z\n"
                              "ASCII\n"
                              "DATASET STRUCTURED_POINTS\n"
                              "DIMENSIONS 1 1 4\n"
                              "ORIGIN 0 0 0\n"
                              "SPACING 1 1 1\n"
                              "POINT_DATA 4\n"
                              "SCALARS vals float 1\n"
                              "LOOKUP_TABLE default\n"
                              "0 1 2 3\n");

    viskores::cont::DataSet ds = readVTKDataSet(vtkFile.GetFileName());
    CheckStructuredCellSet(ds, 1, 4, 3);
    CheckStructuredLineCell(ds,
                            std::array<viskores::Vec3f, 2>{ viskores::Vec3f(0.0f, 0.0f, 0.0f),
                                                            viskores::Vec3f(0.0f, 0.0f, 1.0f) });
  }
}

void TestRectilinearGridDegenerateDimensions()
{
  ScopedVTKTestFile vtkFile("vtk_reader_rectilinear_y.vtk",
                            "# vtk DataFile Version 3.0\n"
                            "rectilinear y\n"
                            "ASCII\n"
                            "DATASET RECTILINEAR_GRID\n"
                            "DIMENSIONS 1 4 1\n"
                            "X_COORDINATES 1 float\n"
                            "0\n"
                            "Y_COORDINATES 4 float\n"
                            "10 20 30 40\n"
                            "Z_COORDINATES 1 float\n"
                            "5\n"
                            "POINT_DATA 4\n"
                            "SCALARS vals float 1\n"
                            "LOOKUP_TABLE default\n"
                            "0 1 2 3\n");

  viskores::cont::DataSet ds = readVTKDataSet(vtkFile.GetFileName());
  CheckStructuredCellSet(ds, 1, 4, 3);
  CheckStructuredLineCell(ds,
                          std::array<viskores::Vec3f, 2>{ viskores::Vec3f(0.0f, 10.0f, 5.0f),
                                                          viskores::Vec3f(0.0f, 20.0f, 5.0f) });
}

void TestStructuredGridDegenerateDimensions()
{
  ScopedVTKTestFile vtkFile("vtk_reader_structured_grid_xz.vtk",
                            "# vtk DataFile Version 3.0\n"
                            "structured grid xz\n"
                            "ASCII\n"
                            "DATASET STRUCTURED_GRID\n"
                            "DIMENSIONS 3 1 2\n"
                            "POINTS 6 float\n"
                            "0 0 0 1 0 0 2 0 0 0 0 1 1 0 1 2 0 1\n"
                            "POINT_DATA 6\n"
                            "SCALARS vals float 1\n"
                            "LOOKUP_TABLE default\n"
                            "0 1 2 3 4 5\n");

  viskores::cont::DataSet ds = readVTKDataSet(vtkFile.GetFileName());
  CheckStructuredCellSet(ds, 2, 6, 2);
  CheckStructuredQuadCell(ds,
                          std::array<viskores::Vec3f, 4>{ viskores::Vec3f(0.0f, 0.0f, 0.0f),
                                                          viskores::Vec3f(1.0f, 0.0f, 0.0f),
                                                          viskores::Vec3f(1.0f, 0.0f, 1.0f),
                                                          viskores::Vec3f(0.0f, 0.0f, 1.0f) });
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

  std::cout << "Test permuting multi-component expanded cell data" << std::endl;
  TestPermutingMultiComponentCellData();
  std::cout << "Test reading polyline cell" << std::endl;
  TestReadingPolyLineCell();
  std::cout << "Test skipping mismatched FIELD arrays" << std::endl;
  TestSkippingMismatchedFieldData();
  std::cout << "Test reading structured points with degenerate dimensions" << std::endl;
  TestStructuredPointsDegenerateDimensions();
  std::cout << "Test reading rectilinear grids with degenerate dimensions" << std::endl;
  TestRectilinearGridDegenerateDimensions();
  std::cout << "Test reading structured grids with degenerate dimensions" << std::endl;
  TestStructuredGridDegenerateDimensions();
}

int UnitTestVTKDataSetReader(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestReadingVTKDataSet, argc, argv);
}
