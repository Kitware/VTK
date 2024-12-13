#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkFLUENTReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkUnstructuredGrid.h"

#include "vtkLogger.h"
#include "vtkTestUtilities.h"

#define Check(expr, message)                                                                       \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      vtkErrorWithObjectMacro(nullptr, "Test failed: \n" << message);                              \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

namespace
{

bool TestFLUENTReaderMSH(const std::string& filename)
{
  vtkNew<vtkFLUENTReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkMultiBlockDataSet* set = reader->GetOutput();
  Check(set->GetNumberOfBlocks() == 6,
    "Wrong number of blocks: " << set->GetNumberOfBlocks() << ", expected 6");
  Check(set->GetNumberOfCells() == 34250,
    "Wrong number of cells: " << set->GetNumberOfCells() << ", expected 34250");
  Check(set->GetNumberOfPoints() == 11772,
    "Wrong number of points: " << set->GetNumberOfPoints() << ", expected 11772");

  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(set->GetBlock(1));
  Check(grid, "Failed to retrieve zone block");
  Check(grid->GetNumberOfPoints() == 1962,
    "Wrong number of points: " << grid->GetNumberOfPoints() << ", expected 1962");
  Check(grid->GetNumberOfCells() == 6850,
    "Wrong number of cells: " << grid->GetNumberOfCells() << ", expected 6850");

  return true;
}

bool TestFLUENTReaderMSHSurface(const std::string& filename)
{
  struct ExpectedBlockInfo
  {
    vtkIdType NumberOfPoints;
    vtkIdType NumberOfCells;
  };

  std::vector<ExpectedBlockInfo> blockInfos = {
    { 1441, 280 },
    { 1441, 302 },
    { 1441, 300 },
    { 1441, 2000 },
  };

  vtkNew<vtkFLUENTReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkMultiBlockDataSet* mbds = reader->GetOutput();
  Check(mbds->GetNumberOfBlocks() == 4,
    "Wrong number of blocks: " << mbds->GetNumberOfBlocks() << ", expected 4");

  int blockIdx = 0;
  for (auto& bInfo : blockInfos)
  {
    auto* block = vtkUnstructuredGrid::SafeDownCast(mbds->GetBlock(blockIdx));
    Check(block, "Block was expected to be a vtkUnstructuredGrid");

    Check(block->GetNumberOfPoints() == bInfo.NumberOfPoints,
      "Wrong number of points: " << block->GetNumberOfPoints() << ", expected "
                                 << bInfo.NumberOfPoints);
    Check(block->GetNumberOfCells() == bInfo.NumberOfCells,
      "Wrong number of cells: " << block->GetNumberOfCells() << ", expected "
                                << bInfo.NumberOfCells);
    blockIdx++;
  }

  return true;
}

bool TestFLUENTReaderMSHSurfaceAscii(const std::string& filename)
{
  vtkNew<vtkFLUENTReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkMultiBlockDataSet* set = reader->GetOutput();
  Check(set->GetNumberOfBlocks() == 1,
    "Wrong number of blocks: " << set->GetNumberOfBlocks() << ", expected 1");
  auto* grid = vtkUnstructuredGrid::SafeDownCast(set->GetBlock(0));
  Check(grid, "Wrong block");
  Check(grid->GetNumberOfPoints() == 4,
    "Wrong number of points: " << grid->GetNumberOfPoints() << ", expected 4");
  Check(grid->GetNumberOfCells() == 1,
    "Wrong number of cells: " << grid->GetNumberOfCells() << ", expected 1");

  return true;
}

bool TestFLUENTReaderZoneSelection(const std::string& filename)
{
  vtkNew<vtkFLUENTReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkMultiBlockDataSet* set = reader->GetOutput();
  Check(set->GetNumberOfBlocks() == 8,
    "Wrong number of blocks: " << set->GetNumberOfBlocks() << ", expected 8");
  Check(set->GetNumberOfCells() == 21690,
    "Wrong number of cells: " << set->GetNumberOfCells() << ", expected 21690");
  Check(set->GetNumberOfPoints() == 36520,
    "Wrong number of points: " << set->GetNumberOfPoints() << ", expected 36520");

  reader->GetZoneSectionSelection()->DisableArray("wall-5:wall");
  reader->SetCacheData(false);
  reader->Update();

  Check(set->GetNumberOfBlocks() == 7,
    "Wrong number of blocks: " << set->GetNumberOfBlocks() << ", expected 7");
  Check(set->GetNumberOfCells() == 21590,
    "Wrong number of cells: " << set->GetNumberOfCells() << ", expected 21590");
  Check(set->GetNumberOfPoints() == 31955,
    "Wrong number of points: " << set->GetNumberOfPoints() << ", expected 31955");

  reader->GetZoneSectionSelection()->DisableAllArrays();
  reader->SetCacheData(true);
  reader->GetZoneSectionSelection()->EnableArray("wall-5:wall");
  reader->Update();

  Check(set->GetNumberOfBlocks() == 1,
    "Wrong number of blocks: " << set->GetNumberOfBlocks() << ", expected 7");
  Check(set->GetNumberOfCells() == 100,
    "Wrong number of cells: " << set->GetNumberOfCells() << ", expected 100");
  Check(set->GetNumberOfPoints() == 4565,
    "Wrong number of points: " << set->GetNumberOfPoints() << ", expected 4565");
  Check(set->GetBlock(0)->GetAttributes(vtkDataObject::CELL)->GetNumberOfArrays() == 15,
    "Wrong number of cell data arrays: "
      << set->GetAttributes(vtkDataObject::CELL)->GetNumberOfArrays() << ", expected 15");
  Check(set->GetBlock(0)->GetAttributes(vtkDataObject::CELL)->HasArray("WALL_SHEAR"),
    "Could not find WALL_SHEAR cell data array !");
  Check(!set->GetBlock(0)->GetAttributes(vtkDataObject::CELL)->HasArray("DENSITY"),
    "DENSITY cell data array should not exist !");

  return true;
}

bool TestFLUENTReaderSelectiveParsing(const std::string& filename)
{
  vtkNew<vtkFLUENTReader> reader;
  reader->SetFileName(filename.c_str());
  reader->GetZoneSectionSelection()->DisableArray("solide:fluid");
  reader->Update();

  vtkMultiBlockDataSet* set = reader->GetOutput();
  Check(set->GetNumberOfBlocks() == 5,
    "Wrong number of blocks: " << set->GetNumberOfBlocks() << ", expected 5");
  Check(set->GetNumberOfCells() == 15141,
    "Wrong number of cells: " << set->GetNumberOfCells() << ", expected 15141");
  Check(set->GetNumberOfPoints() == 9810,
    "Wrong number of points: " << set->GetNumberOfPoints() << ", expected 9810");

  return true;
}

// std::string friendly wrapper for vtkTestUtilities::ExpandDataFileName
std::string GetFilePath(int argc, char* argv[], const char* path)
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, path);
  std::string filename = fname;
  delete[] fname;

  return filename;
}

}

int TestFLUENTReader(int argc, char* argv[])
{
  if (!::TestFLUENTReaderMSH(GetFilePath(argc, argv, "Data/3D_cylinder_vol.msh")))
  {
    return EXIT_FAILURE;
  }

  if (!::TestFLUENTReaderMSHSurface(GetFilePath(argc, argv, "Data/3D_cylinder_surf.msh")))
  {
    return EXIT_FAILURE;
  }

  // Note: fluent_quad.msh contains some variations in line formats so this is also a test
  // about whether we can robustly read different formats
  if (!::TestFLUENTReaderMSHSurfaceAscii(GetFilePath(argc, argv, "Data/fluent_quad.msh")))
  {
    return EXIT_FAILURE;
  }

  if (!::TestFLUENTReaderZoneSelection(GetFilePath(argc, argv, "Data/room.cas")))
  {
    return EXIT_FAILURE;
  }

  if (!::TestFLUENTReaderSelectiveParsing(GetFilePath(argc, argv, "Data/3D_cylinder_vol.msh")))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
