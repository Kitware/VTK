/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCGNSReaderMissingBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCGNSReader.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"

int TestCGNSReaderMissingBase(int argc, char* argv[])
{
  // Dataset is a cube with three timesteps
  // Third timestep is unaccessible due to a missing link
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/MissingBase.cgns");
  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  // Read cell data
  reader->UpdateInformation();
  reader->EnableAllCellArrays();
  reader->Update();

  vtkMultiBlockDataSet* cube = reader->GetOutput();
  vtkDataSet* ds =
    vtkDataSet::SafeDownCast(vtkMultiBlockDataSet::SafeDownCast(cube->GetBlock(0))->GetBlock(0));

  if (!ds)
  {
    std::cerr << "Empty reader output!" << std::endl;
    return EXIT_FAILURE;
  }

  vtkDoubleArray* array = vtkDoubleArray::SafeDownCast(ds->GetCellData()->GetArray("CellValue"));

  if (!array)
  {
    std::cerr << "Cell array 'CellValue' missing!" << std::endl;
    return EXIT_FAILURE;
  }

  if (array->GetValue(0) != 0)
  {
    std::cerr << "Expected cell value equal to 0, but got " << array->GetValue(0) << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
