// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataSetReader.h"

#include "vtkFileResourceStream.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>

namespace
{
bool TestAndCompare(int argc, char* argv[], const std::string& file)
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string filename = testing->GetDataRoot();
  filename += "/Data/";
  filename += file;

  if (!vtkDataSetReader::CanReadFile(filename.c_str()))
  {
    std::cerr << "Unexpected CanReadFile result with filename" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkFileResourceStream> fileStream;
  fileStream->Open(filename.c_str());

  if (!vtkDataSetReader::CanReadFile(fileStream))
  {
    std::cerr << "Unexpected CanReadFile result with stream" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkDataSetReader> reader;
  reader->SetStream(fileStream);
  reader->ReadFromInputStreamOn();
  reader->Update();

  vtkNew<vtkDataSetReader> reader2;
  reader2->SetFileName(filename.c_str());
  reader2->Update();

  if (reader->GetOutput()->GetNumberOfPoints() == 0)
  {
    std::cerr << "Unexpected empty output reading a stream of " << file << std::endl;
    return false;
  }

  if (!vtkTestUtilities::CompareDataObjects(reader->GetOutput(), reader2->GetOutput()))
  {
    std::cerr << file << " are not the same when read from stream vs file" << std::endl;
    return false;
  }
  return true;
}
}

int TestLegacyDataSetReaderStream(int argc, char* argv[])
{
  bool ret = true;
  ret &= ::TestAndCompare(argc, argv, "fran_cut.vtk");      // polydata
  ret &= ::TestAndCompare(argc, argv, "blow.vtk");          // unstructured grid
  ret &= ::TestAndCompare(argc, argv, "ironProt.vtk");      // image data
  ret &= ::TestAndCompare(argc, argv, "RectGrid2.vtk");     // rectilinear grid
  ret &= ::TestAndCompare(argc, argv, "office.binary.vtk"); // structured grid
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
