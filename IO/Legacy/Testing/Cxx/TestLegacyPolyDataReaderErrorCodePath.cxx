// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtkTestErrorObserver.h>
#include <vtkTesting.h>

// A test for different error code path in the polydata reader
int TestLegacyPolyDataReaderErrorCodePath(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  // ==========================================================
  // Test for https://gitlab.kitware.com/vtk/vtk/-/issues/18689
  std::string filename = testing->GetDataRoot();
  filename += "/Data/invalid_polydata.vtk";

  // Create the reader
  vtkNew<vtkPolyDataReader> reader;
  reader->SetFileName(filename.c_str());

  // Create an error observer
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  // Read the file
  reader->Update();

  // Check error message
  int status = errorObserver->CheckErrorMessage("Error reading ascii cell data! for file");
  if (status != 0)
  {
    std::cerr << "Expecting specific error messages but could not find them" << std::endl;
    return EXIT_FAILURE;
  }

  // Check output
  vtkPolyData* pd = reader->GetOutput();
  if (pd->GetNumberOfPoints() != 4469)
  {
    std::cerr << "Expecting 4460 points after reading an invalid polydata, but got "
              << pd->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }
  if (pd->GetNumberOfCells() != 0)
  {
    std::cerr << "Expecting no cells after reading an invalid polydata, but got "
              << pd->GetNumberOfCells() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
