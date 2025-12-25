// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLUnstructuredGridReader.h"

#include "vtkFileResourceStream.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>

int TestXMLUnstructuredGridReaderStream(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/polyhedron2pieces.vtu");
  if (fileName == nullptr)
  {
    std::cerr << "Could not get file names.";
    return EXIT_FAILURE;
  }

  vtkNew<vtkFileResourceStream> fileStream;
  fileStream->Open(fileName);

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetStream(fileStream);
  reader->ReadFromInputStreamOn();
  reader->Update();

  vtkNew<vtkXMLUnstructuredGridReader> reader2;
  reader2->SetFileName(fileName);
  reader2->Update();

  if (reader->GetOutput()->GetNumberOfPoints() == 0)
  {
    std::cerr << "Unexpected empty output reading a stream of unstructured grid" << std::endl;
    return false;
  }

  if (!vtkTestUtilities::CompareDataObjects(reader->GetOutput(), reader2->GetOutput()))
  {
    std::cerr
      << "Unstructured grids with polyhedrons are not the same when read from stream vs file"
      << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
