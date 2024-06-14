// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

namespace
{
int TestDefaultImplementation()
{
  int result = EXIT_SUCCESS;

  vtkNew<vtkFloatArray> floatArray;
  floatArray->SetNumberOfValues(1);
  floatArray->SetValue(0, 0);

  // Image data does not implement GetMeshMTime (for now);
  // Modifications to data arrays should update MeshMTime.
  vtkNew<vtkImageData> imageData;
  imageData->GetPointData()->AddArray(floatArray);

  // Modify point data arrays
  const vtkMTimeType initialMeshMTime = imageData->GetMeshMTime();
  imageData->GetPointData()->AddArray(floatArray);
  const vtkMTimeType newMeshMTime = imageData->GetMeshMTime();

  if (initialMeshMTime == newMeshMTime)
  {
    vtkLog(ERROR, "Expected modified MeshMTime.");
    result &= EXIT_FAILURE;
  }

  return result;
}

int TestCustomImplementation()
{
  int result = EXIT_SUCCESS;

  vtkNew<vtkFloatArray> floatArray;
  floatArray->SetNumberOfValues(1);
  floatArray->SetValue(0, 0);

  float point[3] = { 0, 0, 0 };
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(1);
  points->SetPoint(0, point);

  // vtkPolyData implements GetMeshMTime:
  // Modifying field data shouldn't modify MeshMTime;
  // Modifying points themselves should.
  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);
  polydata->GetPointData()->AddArray(floatArray);

  // Modify point data array
  vtkMTimeType initialMeshMTime = polydata->GetMeshMTime();
  floatArray->SetValue(0, 1);
  vtkMTimeType newMeshMTime = polydata->GetMeshMTime();

  if (initialMeshMTime != newMeshMTime)
  {
    vtkLog(ERROR,
      "Modifying point data should not modify MeshMTime. Expected: " << initialMeshMTime
                                                                     << ", Got: " << newMeshMTime);
    result &= EXIT_FAILURE;
  }

  // Modify points
  initialMeshMTime = polydata->GetMeshMTime();
  float newPoint[3] = { 1, 1, 1 };
  points->SetNumberOfPoints(2);
  points->SetPoint(1, newPoint);
  newMeshMTime = polydata->GetMeshMTime();

  if (initialMeshMTime == newMeshMTime)
  {
    vtkLog(ERROR, "Modifying point data should modify MeshMTime.");
    result &= EXIT_FAILURE;
  }

  return result;
}
}

//------------------------------------------------------------------------------
int TestMeshMTime(int, char*[])
{
  int result = EXIT_SUCCESS;

  result &= TestDefaultImplementation();
  result &= TestCustomImplementation();

  return result;
}
