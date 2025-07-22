// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArrayAccessor.h"
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

  vtkDataArray* sampledPointArray = pointSampling->GetOutput()->GetPointData()->GetArray(fieldName);
  if (sampledPointArray->GetNumberOfValues() != 982)
    return EXIT_FAILURE;
  else
    return EXIT_SUCCESS;
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

  grid = vtkDataSet::SafeDownCast(wavelet->GetOutput());

  if (PerformTest(grid))
  {
    return EXIT_FAILURE;
  }

  // convert the structured grid to an unstructured grid
  vtkNew<vtkmCleanGrid> ug;
  ug->SetInputConnection(wavelet->GetOutputPort());
  ug->Update();

  grid = vtkDataSet::SafeDownCast(ug->GetOutput());
  if (PerformTest(grid))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
