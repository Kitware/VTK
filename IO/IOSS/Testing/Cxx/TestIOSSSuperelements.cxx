/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOSSSuperelements.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkIOSSReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkTestUtilities.h>

static std::string GetFileName(int argc, char* argv[], const char* fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC);
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

/**
 * This test open various unsupported files and ensures that the reader raises
 * errors as expected without crashing.
 */
int TestIOSSSuperelements(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  reader->SetFileName(
    GetFileName(argc, argv, "Data/Exodus/SAND2017-5827O-FSM_Residual-bad-eigen.e").c_str());
  reader->Update();

  int status = EXIT_SUCCESS;

  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  // block named "eb2" has the super element
  if (pdc->GetPartitionedDataSet(10)->GetNumberOfPoints() != 16)
  {
    vtkLogF(ERROR, "ERROR: Incorrect superelement point count; expected=16, got=%d",
      (int)pdc->GetPartitionedDataSet(2)->GetNumberOfPoints());
    status = EXIT_FAILURE;
  }

  reader->SetFileName(
    GetFileName(argc, argv, "Data/Exodus/SAND2017-5827O-FSM_Residual_good-eigen.e").c_str());
  reader->Update();

  pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  // block named "electronicboards" has the super element
  if (pdc->GetPartitionedDataSet(4)->GetNumberOfPoints() != 8)
  {
    vtkLogF(ERROR, "Incorrect superelement point count; expected=8, got=%d",
      (int)pdc->GetPartitionedDataSet(1)->GetNumberOfPoints());
    status = EXIT_FAILURE;
  }

  return status;
}
