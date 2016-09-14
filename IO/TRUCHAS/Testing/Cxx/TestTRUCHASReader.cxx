/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTRUCHASReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Input test to validate ability to read GE TRUCHAS files
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTesting.h"
#include "vtkTRUCHASReader.h"
#include "vtkUnstructuredGrid.h"

int TestTRUCHASReader(int argc, char *argv[])
{
  // Load the initial dataset:
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::string filename = testing->GetDataRoot();
  filename = "/home/demarle/Downloads/moving_rod/moving_rod.h5";

  vtkNew<vtkTRUCHASReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();
  vtkUnstructuredGrid *grid = vtkUnstructuredGrid::SafeDownCast
    (reader->GetOutput()->GetBlock(0));

  if (!grid || grid->GetNumberOfPoints()==0)
    {
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
