// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkClipDataSet.h"
#include "vtkCommand.h"
#include "vtkContourGrid.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkPlane.h"
#include "vtkRTAnalyticSource.h"
#include "vtkShrinkFilter.h"
#include "vtkUnstructuredGrid.h"

class vtkHandler : public vtkObject
{
public:
  int AbortEventCounts = 0;
  int CleanupEventCounts = 0;

  static vtkHandler* New();
  vtkTypeMacro(vtkHandler, vtkObject);

  void AbortCallback() { this->AbortEventCounts++; }
  void CleanupCallback() { this->CleanupEventCounts++; }
};
vtkStandardNewMacro(vtkHandler);

int TestAbortExecute(int, char*[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkShrinkFilter> shrink;
  vtkNew<vtkContourGrid> contour;
  vtkNew<vtkClipDataSet> clip;
  vtkNew<vtkHandler> handler;

  wavelet->SetWholeExtent(0, 10, 0, 10, 0, 10);
  wavelet->AddObserver(vtkCommand::AbortCheckEvent, handler.Get(), &vtkHandler::AbortCallback);
  wavelet->AddObserver(
    vtkCommand::CleanupAbortCheckEvent, handler.Get(), &vtkHandler::CleanupCallback);

  shrink->SetInputConnection(wavelet->GetOutputPort());

  contour->SetInputConnection(shrink->GetOutputPort());
  contour->GenerateValues(1, 10, 10);

  vtkNew<vtkPlane> clipPlane;
  clipPlane->SetNormal(1, 0, 0);
  clipPlane->SetOrigin(0, 0, 0);

  clip->SetInputConnection(contour->GetOutputPort());
  clip->SetClipFunction(clipPlane);

  wavelet->SetAbortExecuteAndUpdateTime();
  clip->Update();

  if (handler->AbortEventCounts != 1)
  {
    vtkLog(ERROR, "Wavelet did not invoke expected abort check event");
  }
  if (handler->CleanupEventCounts != 1)
  {
    vtkLog(ERROR, "Wavelet did not invoke expected cleanup abort check event");
  }
  if (!wavelet->GetAbortExecute())
  {
    vtkLog(ERROR, "Wavelet AbortExecute flag is not set.");
    return 1;
  }

  if (shrink->GetAbortExecute() || contour->GetAbortExecute() || clip->GetAbortExecute())
  {
    vtkLog(ERROR, "Shrink, Contour, or Clip AbortExecute flag is set.");
    return 1;
  }

  if (!wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip ABORTED flag is not set.");
    return 1;
  }

  if (clip->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "Found output data.");
    return 1;
  }

  wavelet->SetAbortExecute(0);
  shrink->SetAbortExecuteAndUpdateTime();
  clip->Update();

  if (!shrink->GetAbortExecute())
  {
    vtkLog(ERROR, "Shrink AbortExecute flag is not set.");
    return 1;
  }

  if (wavelet->GetAbortExecute() || contour->GetAbortExecute() || clip->GetAbortExecute())
  {
    vtkLog(ERROR, "Wavelet, Contour, or Clip AbortExecute flag is set.");
    return 1;
  }

  if (wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Wavelet ABORTED flag is set.");
    return 1;
  }

  if (!shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    vtkLog(ERROR, "Wavelet, Shrink, Contour, or Clip ABORTED flag is not set.");
    return 1;
  }

  if (clip->GetOutput()->GetNumberOfPoints())
  {
    vtkLog(ERROR, "Found output data.");
    return 1;
  }

  shrink->SetAbortExecute(0);
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

  return 0;
}
