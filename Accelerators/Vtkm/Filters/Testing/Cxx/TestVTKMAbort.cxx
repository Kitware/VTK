// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkPlane.h"
#include "vtkRTAnalyticSource.h"
#include "vtkShrinkFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkmClip.h"
#include "vtkmContour.h"

#include <viskores/cont/Initialize.h>

int TestVTKMAbort(int, char*[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkShrinkFilter> shrink;
  vtkNew<vtkmContour> contour;
  vtkNew<vtkmClip> clip;

  wavelet->SetWholeExtent(0, 10, 0, 10, 0, 10);

  shrink->SetInputConnection(wavelet->GetOutputPort());

  contour->SetInputConnection(shrink->GetOutputPort());
  contour->GenerateValues(5, -6, 250);

  vtkNew<vtkPlane> clipPlane;
  clipPlane->SetNormal(1, 0, 0);
  clipPlane->SetOrigin(0, 0, 0);

  clip->SetInputConnection(contour->GetOutputPort());
  clip->SetClipFunction(clipPlane);

  //--------------------------------------------------------------------------
  std::cout << "Run 1 with abort on contour\n";

  contour->SetAbortExecuteAndUpdateTime();
  clip->Update();

  if (!contour->GetAbortExecute())
  {
    vtkLog(ERROR, "Contour AbortExecute flag is not set.");
    return 1;
  }

  if (shrink->GetAbortExecute() || wavelet->GetAbortExecute() || clip->GetAbortExecute())
  {
    vtkLog(ERROR, "Shrink, Wavelet, or Clip AbortExecute flag is set.");
    return 1;
  }

  if (!contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Contour, or Clip ABORTED flag is not set.");
    return 1;
  }

  if (clip->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "Found output data.");
    return 1;
  }

  //--------------------------------------------------------------------------
  std::cout << "Run 2 with no aborts\n";
  contour->SetAbortExecute(0);
  clip->Update();

  if (wavelet->GetAbortExecute() || shrink->GetAbortExecute() || contour->GetAbortExecute() ||
    clip->GetAbortExecute())
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip AbortExecute flag is set.");
    return 1;
  }

  if (wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip ABORTED flag is set.");
    return 1;
  }

  if (!clip->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "No output data.");
    return 1;
  }

  std::cout << "Tests successful\n";

  return 0;
}
