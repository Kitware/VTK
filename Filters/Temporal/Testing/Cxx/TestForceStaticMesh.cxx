// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkForceStaticMesh.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include <cstdlib>

namespace
{
//------------------------------------------------------------------------------
void GetPartitionsMeshMTimes(
  vtkPartitionedDataSetCollection* pdsc, std::vector<vtkMTimeType>& times)
{
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(pdsc->NewIterator());
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    vtkMTimeType time = ds ? ds->GetMeshMTime() : -1;
    times.push_back(time);
  }
}

//------------------------------------------------------------------------------
bool TestCompositeTemporal()
{
  // create a composite temporal pipeline
  vtkNew<vtkSphereSource> sphere1;
  vtkNew<vtkSphereSource> sphere2;
  vtkNew<vtkGroupDataSetsFilter> group;
  group->AddInputConnection(sphere1->GetOutputPort());
  group->AddInputConnection(sphere2->GetOutputPort());
  group->SetOutputTypeToPartitionedDataSetCollection();
  vtkNew<vtkGenerateTimeSteps> generateTimes;
  generateTimes->SetInputConnection(group->GetOutputPort());
  generateTimes->ClearTimeStepValues();
  std::vector<double> times = { 0, 1, 2 };
  generateTimes->SetTimeStepValues(static_cast<int>(times.size()), times.data());

  vtkNew<vtkForceStaticMesh> forceStatic;
  forceStatic->SetInputConnection(generateTimes->GetOutputPort());
  forceStatic->UpdateTimeStep(times[0]);

  auto outputPDC =
    vtkPartitionedDataSetCollection::SafeDownCast(generateTimes->GetOutputDataObject(0));
  std::vector<vtkMTimeType> firstVaryingMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, firstVaryingMeshMTimes);

  outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(forceStatic->GetOutputDataObject(0));
  std::vector<vtkMTimeType> firstMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, firstMeshMTimes);

  vtkNew<vtkPartitionedDataSetCollection> firstOutput;
  firstOutput->DeepCopy(outputPDC);

  // Update the sources: the mesh has the same number of point/cell, but different coordinates.
  // Force static mesh will reuse previous mesh.
  sphere1->SetCenter(1, 0, 0);
  sphere2->SetCenter(0, 1, 0);
  forceStatic->UpdateTimeStep(times[1]);

  outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(generateTimes->GetOutputDataObject(0));
  std::vector<vtkMTimeType> secondVaryingMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, secondVaryingMeshMTimes);

  outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(forceStatic->GetOutputDataObject(0));
  std::vector<vtkMTimeType> secondMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, secondMeshMTimes);

  for (size_t timeIdx = 0; timeIdx != secondMeshMTimes.size(); ++timeIdx)
  {
    // Also compare the input's meshMTime to make sure we're testing something
    if (firstVaryingMeshMTimes[timeIdx] == secondVaryingMeshMTimes[timeIdx])
    {
      vtkLog(ERROR,
        "ForceStaticMesh's input's MeshMTime has not changed, this test does not test anything ! "
        "Was static mesh support added to the input filter ?");
    }

    if (firstMeshMTimes[timeIdx] != secondMeshMTimes[timeIdx])
    {
      vtkLog(ERROR, "GetMeshMTime has changed, mesh not static !");
      return false;
    }
  }

  return vtkTestUtilities::CompareDataObjects(outputPDC, firstOutput);
}

//------------------------------------------------------------------------------
bool TestOutputDataTime()
{
  vtkNew<vtkSphereSource> source;
  vtkNew<vtkGenerateTimeSteps> generateTimes;
  generateTimes->SetInputConnection(source->GetOutputPort());
  generateTimes->ClearTimeStepValues();
  generateTimes->GenerateTimeStepValues(0, 10, 1);

  vtkNew<vtkForceStaticMesh> forceStatic;
  forceStatic->SetInputConnection(generateTimes->GetOutputPort());
  forceStatic->UpdateTimeStep(1.);

  vtkPolyData* outputPolyData = forceStatic->GetPolyDataOutput();
  vtkNew<vtkPolyData> cacheFirstOutput;
  cacheFirstOutput->DeepCopy(outputPolyData);
  auto firstDataTime = outputPolyData->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());

  forceStatic->UpdateTimeStep(2.);
  auto secondDataTime = outputPolyData->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  vtkLogIf(ERROR, firstDataTime == secondDataTime,
    "Data time should be updated when new timestep was requested");

  // going back to timestep 1. should retrigger the pipeline, and update info accordingly
  forceStatic->UpdateTimeStep(1.);
  auto finalTime = outputPolyData->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  vtkLogIf(ERROR, finalTime != firstDataTime,
    "Data time should be updated when going back to first timestep");

  return vtkTestUtilities::CompareDataObjects(outputPolyData, cacheFirstOutput);
}
}

//------------------------------------------------------------------------------
int TestForceStaticMesh(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  if (!::TestCompositeTemporal())
  {
    return EXIT_FAILURE;
  }

  if (!::TestOutputDataTime())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
