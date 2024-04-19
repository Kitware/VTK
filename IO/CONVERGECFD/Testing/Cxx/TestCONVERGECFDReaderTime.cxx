// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test of vtkCONVERGECFDReader's handling of timesteps

#include "vtkCONVERGECFDReader.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDebugLeaks.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cstdlib>

int TestCONVERGECFDReaderTime(int argc, char* argv[])
{
  vtkNew<vtkCONVERGECFDReader> reader;

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/CONVERGETimeSeries/post000001_+0.00000e+00.h5");
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // Check the point and cell count in each
  std::array<double, 6> times = { 0.0, 0.0002, 0.0004, 0.0006, 0.0008, 0.0010 };
  std::array<vtkIdType, 6> numCells = { 20597, 36720, 26503, 21671, 21229, 21053 };
  std::array<vtkIdType, 6> numPoints = { 23513, 39636, 29419, 24587, 24145, 23969 };

  vtkExecutive* executive = reader->GetExecutive();
  vtkInformationVector* outputVector = executive->GetOutputInformation();
  for (size_t i = 0; i < numCells.size(); ++i)
  {
    double timeRequested = times[i];
    outputVector->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeRequested);
    reader->Update();

    vtkPartitionedDataSetCollection* pdc =
      vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
    if (!pdc)
    {
      vtkLog(ERROR, << "No output for time " << i);
      return EXIT_FAILURE;
    }

    if (pdc->GetNumberOfCells() != numCells[i])
    {
      vtkLog(ERROR, << "Number of cells for time " << i << " is " << pdc->GetNumberOfCells()
                    << ", but " << numCells[i] << " were expected.");
      return EXIT_FAILURE;
    }

    if (pdc->GetNumberOfPoints() != numPoints[i])
    {
      vtkLog(ERROR, << "Number of points for time " << i << " is " << pdc->GetNumberOfPoints()
                    << ", but " << numPoints[i] << " were expected.");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
