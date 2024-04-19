// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObject.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkMappedUnstructuredGridGenerator.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridBase.h"

#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;             \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestLegacyMappedUnstructuredGrid(int argc, char* argv[])
{
  int rc = 0;

  vtkNew<vtkTest::ErrorObserver> errorObserver;

  vtkUnstructuredGridBase* mg;
  vtkMappedUnstructuredGridGenerator::GenerateMappedUnstructuredGrid(&mg);

  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string filename = testDirectory + std::string("/") + std::string("export.vtk");

  vtkNew<vtkGenericDataObjectWriter> w;
  w->SetFileName(filename.c_str());
  w->SetInputData(mg);
  w->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  rc = w->Write();
  vtk_assert(1 == rc);
  if (errorObserver->GetError())
  {
    cerr << errorObserver->GetErrorMessage() << endl;
  }
  vtk_assert(false == errorObserver->GetError());

  vtkNew<vtkGenericDataObjectReader> r;
  r->SetFileName(filename.c_str());
  r->Update();
  vtkDataObject* read = r->GetOutput();
  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(read);
  vtk_assert(nullptr != ug);

  vtk_assert(mg->GetNumberOfCells() == ug->GetNumberOfCells());

  mg->Delete();
  return EXIT_SUCCESS;
}
