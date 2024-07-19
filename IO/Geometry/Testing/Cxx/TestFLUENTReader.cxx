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

  // TODO: add test for FLUENT Case files

  return EXIT_SUCCESS;
}
