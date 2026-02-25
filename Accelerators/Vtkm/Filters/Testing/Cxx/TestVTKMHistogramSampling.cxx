// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmCleanGrid.h"
#include "vtkmHistogramSampling.h"

#include <viskores/testing/Testing.h>

namespace
{

//------------------------------------------------------------------------------
int PerformTest(vtkDataSet* grid)
{
  const char fieldName[] = "RTData";

  vtkNew<vtkmHistogramSampling> pointSampling;
  pointSampling->SetInputData(grid);
  pointSampling->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointSampling->SetSampleFraction(0.1);
  pointSampling->SetNumberOfBins(10);
  pointSampling->Update();

  // We set the sample fraction to 0.1, so we expect a tenth of the points. However, the selection
  // is stochastic, so the actual selected amount can be different. Within 10% of the expected
  // amount should be good enough.
  const vtkIdType expectedNumPoints = grid->GetNumberOfPoints() / 10;
  const vtkIdType tolerance = expectedNumPoints / 10;
  const vtkIdType minExpectedPoints = expectedNumPoints - tolerance;
  const vtkIdType maxExpectedPoints = expectedNumPoints + tolerance;

  vtkDataArray* sampledPointArray = pointSampling->GetOutput()->GetPointData()->GetArray(fieldName);
  vtkIdType actualSize = sampledPointArray->GetNumberOfValues();
  if ((actualSize < minExpectedPoints) || (actualSize > maxExpectedPoints))
  {
    std::cout << "Expected between " << minExpectedPoints << " and " << maxExpectedPoints
              << ", but received " << actualSize << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}
} // end local namespace

//------------------------------------------------------------------------------
int TestVTKMHistogramSampling(int /* argc */, char* /* argv */[])
{
  vtkDataSet* grid = nullptr;

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetCenter(0, 0, 0);
  wavelet->Update();

  std::cout << "Test structured grid." << std::endl;
  grid = vtkDataSet::SafeDownCast(wavelet->GetOutput());

  if (PerformTest(grid) != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }

  // convert the structured grid to an unstructured grid
  vtkNew<vtkmCleanGrid> ug;
  ug->SetInputConnection(wavelet->GetOutputPort());
  ug->Update();

  std::cout << "Test unstructured grid." << std::endl;
  grid = vtkDataSet::SafeDownCast(ug->GetOutput());
  if (PerformTest(grid) != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
