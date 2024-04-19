// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSReader.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"

int TestCGNSUnsteadyTemporalSolution(int argc, char* argv[])
{
  // Dataset is a cube with three timesteps
  // Third timestep is unaccessible due to a missing link
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/TemporalBox.cgns");
  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  // Read cell data
  reader->UpdateInformation();
  reader->EnableAllCellArrays();
  reader->SetUseUnsteadyPattern(true);
  reader->SetUnsteadySolutionStartTimestep(0);
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

  if (array->GetValue(0) != 2)
  {
    std::cerr << "Expected cell value for first timestep equal to 2, but got " << array->GetValue(0)
              << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Check second timestep
  reader->GetOutputInformation(0)->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 2);
  reader->Update();

  cube = reader->GetOutput();
  ds = vtkDataSet::SafeDownCast(vtkMultiBlockDataSet::SafeDownCast(cube->GetBlock(0))->GetBlock(0));
  array = vtkDoubleArray::SafeDownCast(ds->GetCellData()->GetArray("CellValue"));

  if (array->GetValue(0) != 4)
  {
    std::cerr << "Expected cell value for second timestep equal to 4, but got "
              << array->GetValue(0) << "." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
