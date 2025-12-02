// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMRGaussianPulseSource.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkTesting.h"

#include <iostream>

#define TEST_SUCCESS 0
#define TEST_FAILED 1

#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      std::cerr << "ERROR: Condition FAILED!! : " << #x << std::endl;                              \
      return TEST_FAILED;                                                                          \
    }                                                                                              \
  } while (false)

bool Validate(vtkOverlappingAMR* input, vtkOverlappingAMR* result)
{
  vtk_assert(input->GetNumberOfLevels() == result->GetNumberOfLevels());
  vtk_assert(input->GetOrigin()[0] == result->GetOrigin()[0]);
  vtk_assert(input->GetOrigin()[1] == result->GetOrigin()[1]);
  vtk_assert(input->GetOrigin()[2] == result->GetOrigin()[2]);

  for (unsigned int level = 0; level < input->GetNumberOfLevels(); level++)
  {
    vtk_assert(input->GetNumberOfBlocks(level) == result->GetNumberOfBlocks(level));
  }

  std::cout << "Check input validity" << std::endl;
  bool ret = input->CheckValidity();
  std::cout << "Check output validity" << std::endl;
  ret &= result->CheckValidity();
  return ret;
}

int TestLegacyCompositeDataReaderWriter(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  vtkNew<vtkAMRGaussianPulseSource> source;

  std::string filename = testing->GetTempDirectory();
  filename += "/amr_data.vtk";
  vtkNew<vtkGenericDataObjectWriter> writer;
  writer->SetFileName(filename.c_str());
  writer->SetFileTypeToASCII();
  writer->SetInputConnection(source->GetOutputPort());
  writer->Write();

  vtkNew<vtkGenericDataObjectReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  // now valid the input and output datasets.
  vtkOverlappingAMR* input = vtkOverlappingAMR::SafeDownCast(source->GetOutputDataObject(0));
  vtkOverlappingAMR* result = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  if (!Validate(input, result))
  {
    return TEST_FAILED;
  }

  std::cout << "Test Binary IO" << std::endl;

  writer->SetFileTypeToBinary();
  writer->Write();

  reader->SetFileName(nullptr);
  reader->SetFileName(filename.c_str());
  reader->Update();
  return Validate(input, vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)))
    ? TEST_SUCCESS
    : TEST_FAILED;
}
