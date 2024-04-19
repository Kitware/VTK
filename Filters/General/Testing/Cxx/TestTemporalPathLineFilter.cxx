// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Hide VTK_DEPRECATED_IN_9_3_0() warnings.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkTemporalPathLineFilter.h"

#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimeSourceExample.h"

bool TestForwardTime()
{
  vtkNew<vtkTimeSourceExample> timeSource;
  timeSource->SetXAmplitude(10);
  timeSource->SetYAmplitude(0);
  timeSource->UpdateInformation();
  vtkInformation* sourceInformation = timeSource->GetOutputInformation(0);
  if (!sourceInformation)
  {
    vtkLog(ERROR, "Invalid source information.");
    return false;
  }
  double* timeSteps = sourceInformation->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int timeStepsNumber = sourceInformation->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  vtkNew<vtkTemporalPathLineFilter> temporalPathLineFilter;
  temporalPathLineFilter->SetInputConnection(0, timeSource->GetOutputPort());
  temporalPathLineFilter->SetMaxTrackLength(100);
  temporalPathLineFilter->SetMaxStepDistance(100, 100, 100);

  for (int timeStep = 0; timeStep < timeStepsNumber; ++timeStep)
  {
    temporalPathLineFilter->UpdateTimeStep(timeSteps[timeStep]);
  }

  auto* resultPolyData = temporalPathLineFilter->GetOutput();
  if (!resultPolyData)
  {
    vtkLog(ERROR, "Invalid result poly data.");
    return false;
  }

  if (resultPolyData->GetNumberOfPoints() != 10)
  {
    vtkLog(
      ERROR, "Wrong number of points in result poly data: " << resultPolyData->GetNumberOfPoints());
    return false;
  }

  auto* lines = resultPolyData->GetLines();
  if (!lines)
  {
    vtkLog(ERROR, "Invalid lines in result poly data.");
    return false;
  }

  if (lines->GetNumberOfCells() != 1)
  {
    vtkLog(
      ERROR, "Wrong number of cells in lines from result poly data: " << lines->GetNumberOfCells());
    return false;
  }

  vtkNew<vtkIdList> pointIDs;
  lines->GetCellAtId(0, pointIDs);
  if (pointIDs->GetNumberOfIds() != 10)
  {
    vtkLog(ERROR, "Wrong number of points in result poly data.");
    return false;
  }

  return true;
}

bool TestBackwardTime()
{
  vtkNew<vtkTimeSourceExample> timeSource;
  timeSource->SetXAmplitude(10);
  timeSource->SetYAmplitude(0);
  timeSource->UpdateInformation();
  vtkInformation* sourceInformation = timeSource->GetOutputInformation(0);
  if (!sourceInformation)
  {
    vtkLog(ERROR, "Invalid source information.");
    return false;
  }
  double* timeSteps = sourceInformation->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int timeStepsNumber = sourceInformation->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  vtkNew<vtkTemporalPathLineFilter> temporalPathLineFilter;
  temporalPathLineFilter->SetInputConnection(0, timeSource->GetOutputPort());
  temporalPathLineFilter->SetMaxTrackLength(100);
  temporalPathLineFilter->SetMaxStepDistance(100, 100, 100);
  temporalPathLineFilter->SetBackwardTime(true);

  for (int timeStep = timeStepsNumber - 1; timeStep >= 0; --timeStep)
  {
    temporalPathLineFilter->UpdateTimeStep(timeSteps[timeStep]);
  }

  auto* resultPolyData = temporalPathLineFilter->GetOutput();
  if (!resultPolyData)
  {
    vtkLog(ERROR, "Invalid result poly data.");
    return false;
  }

  if (resultPolyData->GetNumberOfPoints() != 10)
  {
    vtkLog(
      ERROR, "Wrong number of points in result poly data: " << resultPolyData->GetNumberOfPoints());
    return false;
  }

  auto* lines = resultPolyData->GetLines();
  if (!lines)
  {
    vtkLog(ERROR, "Invalid lines in result poly data.");
    return false;
  }

  if (lines->GetNumberOfCells() != 1)
  {
    vtkLog(
      ERROR, "Wrong number of cells in lines from result poly data: " << lines->GetNumberOfCells());
    return false;
  }

  vtkNew<vtkIdList> pointIDs;
  lines->GetCellAtId(0, pointIDs);
  if (pointIDs->GetNumberOfIds() != 10)
  {
    vtkLog(ERROR, "Wrong number of points in result poly data.");
    return false;
  }

  return true;
}

int TestTemporalPathLineFilter(int, char*[])
{
  if (!TestForwardTime() || !TestBackwardTime())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
