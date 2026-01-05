// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUnstructuredGridReader.h"

#include "vtkFileResourceStream.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>

int TestLegacyUnstructuredGridStream(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string filename = testing->GetDataRoot();
  filename += "/Data/blow.vtk";

  vtkNew<vtkFileResourceStream> fileStream;
  fileStream->Open(filename.c_str());

  vtkNew<vtkUnstructuredGridReader> reader;
  reader->SetStream(fileStream);
  reader->ReadFromInputStreamOn();
  reader->Update();

  vtkNew<vtkUnstructuredGridReader> reader2;
  reader2->SetFileName(filename.c_str());
  reader2->Update();

  if (reader->GetOutput()->GetNumberOfPoints() == 0)
  {
    std::cerr << "Unexpected empty output reading a stream of unstructured grid" << std::endl;
    return false;
  }

  if (!vtkTestUtilities::CompareDataObjects(reader->GetOutput(), reader2->GetOutput()))
  {
    std::cerr
      << "Unstructured grids with polyhedrons are not the same when reader from stream vs file"
      << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
