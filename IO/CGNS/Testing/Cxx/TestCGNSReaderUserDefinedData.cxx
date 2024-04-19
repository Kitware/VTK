// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSReader.h"

#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"

int TestCGNSReaderUserDefinedData(int argc, char* argv[])
{
  // Dataset contains two simple structured zones with UserDefinedData_t nodes
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/UserDefinedData.cgns");
  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;
  reader->Update();

  vtkMultiBlockDataSet* dataset = reader->GetOutput();

  if (!dataset)
  {
    std::cerr << "Empty reader output!" << std::endl;
    return EXIT_FAILURE;
  }

  vtkMultiBlockDataSet* base = vtkMultiBlockDataSet::SafeDownCast(dataset->GetBlock(0));

  if (!base)
  {
    std::cerr << "Could not find base block." << std::endl;
    return EXIT_FAILURE;
  }

  // Check field data array produced by UserDefinedData_t nodes
  vtkStructuredGrid* zone = vtkStructuredGrid::SafeDownCast(base->GetBlock(0));

  if (!zone)
  {
    std::cerr << "Could not find first zone block under base block." << std::endl;
    return EXIT_FAILURE;
  }

  vtkIntArray* array = vtkIntArray::SafeDownCast(zone->GetFieldData()->GetArray("Cube_Index"));

  if (!array)
  {
    std::cerr << "Missing 'Cube_Index' array from field data." << std::endl;
    return EXIT_FAILURE;
  }

  if (array->GetValue(0) != 1)
  {
    std::cerr << "Expected value equal to 1, but got " << array->GetValue(0) << "." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
