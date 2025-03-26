#include "vtkFLUENTReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkUnstructuredGrid.h"

#include "vtkTestUtilities.h"

#include <array>

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
  Check(set->GetNumberOfBlocks() == 6, "Wrong number of blocks");
  Check(set->GetNumberOfCells() == 34250, "Wrong number of cells in dataset");
  Check(set->GetNumberOfPoints() == 11772, "Wrong number of points in dataset");

  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(set->GetBlock(1));
  Check(grid, "Failed to retrieve zone block");
  Check(grid->GetNumberOfPoints() == 1962, "Wrong number of points in zone block");
  Check(grid->GetNumberOfCells() == 6850, "Wrong number of cells in zone block");

  return true;
}

bool TestFLUENTReaderMSHSurface(const std::string& filename)
{
  vtkNew<vtkFLUENTReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkMultiBlockDataSet* set = reader->GetOutput();
  Check(set->GetNumberOfBlocks() == 1, "Wrong number of blocks");
  auto* grid = vtkUnstructuredGrid::SafeDownCast(set->GetBlock(0));
  Check(grid, "Wrong block");
  Check(grid->GetNumberOfPoints() == 1441, "Wrong number of points");
  Check(grid->GetNumberOfCells() == 2882, "Wrong number of cells");

  return true;
}

bool TestFLUENTReaderMSHSurfaceAscii(const std::string& filename)
{
  vtkNew<vtkFLUENTReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkMultiBlockDataSet* set = reader->GetOutput();
  Check(set->GetNumberOfBlocks() == 1, "Wrong number of blocks");
  auto* grid = vtkUnstructuredGrid::SafeDownCast(set->GetBlock(0));
  Check(grid, "Wrong block");
  Check(grid->GetNumberOfPoints() == 4, "Wrong number of points");
  Check(grid->GetNumberOfCells() == 1, "Wrong number of cells");

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
