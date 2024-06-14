// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCubeSource.h"
#include "vtkDataArray.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMergeArrays.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

namespace
{
/**
 * Append data arrays from a cube to another cube which has no data arrays.
 * This cube should have a 'Normals' and 'Tcoords' as point data arrays.
 */
bool TestAppendArraysSimple()
{
  vtkNew<vtkCubeSource> cubeSource;
  cubeSource->Update();
  vtkPolyData* cube = vtkPolyData::SafeDownCast(cubeSource->GetOutput());

  vtkNew<vtkPolyData> cubeWithoutArrays;
  cubeWithoutArrays->SetPoints(cube->GetPoints());
  cubeWithoutArrays->SetPolys(cube->GetPolys());

  vtkNew<vtkMergeArrays> mergeArrays;
  mergeArrays->AddInputData(cubeWithoutArrays);
  mergeArrays->AddInputData(cube);
  mergeArrays->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(mergeArrays->GetOutput());

  int numberOfPointData = output->GetPointData()->GetNumberOfArrays();
  if (numberOfPointData != 2)
  {
    vtkLog(ERROR, "Expected 2 point data arrays but got " << numberOfPointData);
    return false;
  }

  if (!output->GetPointData()->HasArray("Normals"))
  {
    vtkLog(ERROR, "'Normals' array is missing.");
    return false;
  }

  if (!output->GetPointData()->HasArray("TCoords"))
  {
    vtkLog(ERROR, "'TCoords' array is missing.");
    return false;
  }

  return true;
}

/**
 * Append data arrays from several cubes into a cube.
 * The cube should have its original 'Normals' and 'Tcoords' and those
 * from other inputs prefixed by the input number.
 */
bool TestAppendArraysWithMultipleInputs()
{
  vtkNew<vtkCubeSource> cubeSource;
  cubeSource->Update();
  vtkPolyData* cube = vtkPolyData::SafeDownCast(cubeSource->GetOutput());

  vtkNew<vtkMergeArrays> mergeArrays;
  mergeArrays->AddInputData(cube);
  mergeArrays->AddInputData(cube);
  mergeArrays->AddInputData(cube);

  mergeArrays->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(mergeArrays->GetOutput());

  int numberOfPointData = output->GetPointData()->GetNumberOfArrays();
  if (numberOfPointData != 6)
  {
    vtkLog(ERROR, "Expected 6 point data arrays but got " << numberOfPointData);
    return false;
  }

  if (!output->GetPointData()->HasArray("Normals"))
  {
    vtkLog(ERROR, "'Normals' array is missing.");
    return false;
  }

  if (!output->GetPointData()->HasArray("TCoords"))
  {
    vtkLog(ERROR, "'TCoords' array is missing.");
    return false;
  }

  if (!output->GetPointData()->HasArray("Normals_input_1"))
  {
    vtkLog(ERROR, "'Normals_input_1' array is missing.");
    return false;
  }

  if (!output->GetPointData()->HasArray("TCoords_input_2"))
  {
    vtkLog(ERROR, "'TCoords_input_2' array is missing.");
    return false;
  }

  return true;
}

/**
 * Append cube with temporal data into another one.
 */
bool TestMergeArraysWithTemporalDatas()
{
  vtkNew<vtkCubeSource> cubeSource;
  cubeSource->Update();
  vtkPolyData* cube = vtkPolyData::SafeDownCast(cubeSource->GetOutput());

  vtkNew<vtkPolyData> cubeWithoutArrays;
  cubeWithoutArrays->SetPoints(cube->GetPoints());
  cubeWithoutArrays->SetPolys(cube->GetPolys());

  vtkNew<vtkGenerateTimeSteps> generateSteps;
  generateSteps->SetInputData(cube);
  generateSteps->GenerateTimeStepValues(0, 10, 2);

  // Test that the output will have temporal information when we have a non temporal + temporal
  // datas
  vtkNew<vtkMergeArrays> mergeArrays;
  mergeArrays->AddInputData(cubeWithoutArrays);
  mergeArrays->AddInputConnection(generateSteps->GetOutputPort());
  mergeArrays->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(mergeArrays->GetOutput());
  int numberOfPointData = output->GetPointData()->GetNumberOfArrays();
  if (numberOfPointData != 2)
  {
    vtkLog(ERROR, "Expected 2 point data arrays but got " << numberOfPointData);
    return false;
  }

  vtkInformation* outInfo = mergeArrays->GetOutputInformation(0);
  int numTimes = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (numTimes != 5)
  {
    vtkLog(ERROR, "Expected 5 time steps but got " << numTimes);
    return false;
  }

  // Test that the output will have all temporal information when we have a several temporal datas
  vtkNew<vtkGenerateTimeSteps> generateSteps2;
  generateSteps2->SetInputData(cube);
  generateSteps2->GenerateTimeStepValues(0, 16, 2);

  mergeArrays->RemoveAllInputs();
  mergeArrays->AddInputData(cubeWithoutArrays);
  mergeArrays->AddInputConnection(generateSteps->GetOutputPort());
  mergeArrays->AddInputConnection(generateSteps2->GetOutputPort());
  mergeArrays->Update();

  output = vtkPolyData::SafeDownCast(mergeArrays->GetOutput());
  numberOfPointData = output->GetPointData()->GetNumberOfArrays();
  if (numberOfPointData != 4)
  {
    vtkLog(ERROR, "Expected 4 point data arrays but got " << numberOfPointData);
    return false;
  }

  outInfo = mergeArrays->GetOutputInformation(0);
  numTimes = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (numTimes != 8)
  {
    vtkLog(ERROR, "Expected 8 time steps but got " << numTimes);
    return false;
  }

  double* times = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  for (int i = 0; i < numTimes; i++)
  {
    if (times[i] != i * 2)
    {
      vtkLog(ERROR,
        "Expected to have time value " << i * 2 << " at index " << i << " but got " << times[i]);
      return false;
    }
  }

  return true;
}

}

//------------------------------------------------------------------------------
int TestMergeArrays(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool success = true;

  success &= ::TestAppendArraysSimple();
  success &= ::TestAppendArraysWithMultipleInputs();
  success &= ::TestMergeArraysWithTemporalDatas();

  if (!success)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
