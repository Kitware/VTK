// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLGenericDataObjectReader.h"

#include "vtkDataSet.h"
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

  vtkNew<vtkFileResourceStream> fileStream;
  fileStream->Open(filename.c_str());

  vtkNew<vtkXMLGenericDataObjectReader> reader;
  reader->SetStream(fileStream);
  reader->ReadFromInputStreamOn();
  reader->Update();

  vtkNew<vtkXMLGenericDataObjectReader> reader2;
  reader2->SetFileName(filename.c_str());
  reader2->Update();

  if (vtkDataSet::SafeDownCast(reader->GetOutput())->GetNumberOfPoints() == 0)
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

int TestXMLGenericDataObjectReaderStream(int argc, char* argv[])
{
  bool ret = true;
  ret &= ::TestAndCompare(argc, argv, "cow.vtp");        // polydata appended
  ret &= ::TestAndCompare(argc, argv, "cube.vtu");       // unstructured grid
  ret &= ::TestAndCompare(argc, argv, "vase_1comp.vti"); // image data
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
