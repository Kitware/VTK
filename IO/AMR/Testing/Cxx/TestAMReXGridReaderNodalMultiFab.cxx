/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMReXParticlesReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRBox.h"
#include "vtkAMReXGridReader.h"
#include "vtkAMReXParticlesReader.h"
#include "vtkDataArraySelection.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"

#define ensure(x, msg)                                                                             \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      cerr << "FAILED: " << msg << endl;                                                           \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int Validate(vtkOverlappingAMR* mb)
{
  ensure(mb != nullptr, "expecting Overlapping AMR Dataset.");
  ensure(mb->GetNumberOfLevels() == 3, "expecting num-levels == 3");

  auto mp = mb->GetDataSet(0, 0);
  // we should have a valid level with a nodal array
  ensure(mp != nullptr, "expecting level is maintained in a vtkUniformGrid.");
  ensure(mp->GetPointData()->GetArray("nu") != nullptr, "missing nodal array nu");

  return EXIT_SUCCESS;
}

int TestAMReXGridReaderNodalMultiFab(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMReX/NodalMultiFab/plt00000");
  vtkNew<vtkAMReXGridReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  reader->SetMaxLevel(2);
  reader->SetPointArrayStatus("nu", 1);
  reader->UpdateInformation();
  reader->Update();
  ensure(reader->GetNumberOfPointArrays() == 1, "nodal array not found");
  ensure(reader->GetNumberOfCellArrays() == 4, "missing cell array(s)");

  return Validate(reader->GetOutput());
}
