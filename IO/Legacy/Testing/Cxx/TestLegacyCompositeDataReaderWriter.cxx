/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLegacyCompositeDataReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRGaussianPulseSource.h"
#include "vtkOverlappingAMR.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkNew.h"
#include "vtkTesting.h"

#define TEST_SUCCESS 0
#define TEST_FAILED 1

#define vtk_assert(x)\
  if (! (x) ) { cerr << "ERROR: Condition FAILED!! : " << #x << endl;  return TEST_FAILED;}

int Validate(vtkOverlappingAMR* input, vtkOverlappingAMR* result)
{
  vtk_assert(input->GetNumberOfLevels() == result->GetNumberOfLevels());
  vtk_assert(input->GetOrigin()[0] == result->GetOrigin()[0]);
  vtk_assert(input->GetOrigin()[1] == result->GetOrigin()[1]);
  vtk_assert(input->GetOrigin()[2] == result->GetOrigin()[2]);

  for (unsigned int level=0; level < input->GetNumberOfLevels(); level++)
    {
    vtk_assert(input->GetNumberOfDataSets(level) ==
      result->GetNumberOfDataSets(level));

    }

  cout << "Audit Input" << endl;
  input->Audit();
  cout << "Audit Output" << endl;
  result->Audit();
  return TEST_SUCCESS;
}


int TestLegacyCompositeDataReaderWriter(int argc, char *argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, (const char**)(argv));

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
  vtkOverlappingAMR* input =
    vtkOverlappingAMR::SafeDownCast(source->GetOutputDataObject(0));
  vtkOverlappingAMR* result =
    vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  if (Validate(input, result) == TEST_FAILED)
    {
    return TEST_FAILED;
    }

  cout << "Test Binary IO" << endl;

  writer->SetFileTypeToBinary();
  writer->Write();

  reader->SetFileName(0);
  reader->SetFileName(filename.c_str());
  reader->Update();
  return Validate(input,
    vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)));
}
