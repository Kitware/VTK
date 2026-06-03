// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "MeshCacheMockAlgorithms.h"
#include "vtkForceStaticMesh.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkTestUtilities.h"

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
bool TestOutputDataTime()
{
  vtkNew<vtkStaticDataSource> source;
  source->SetStartData(1);

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
  vtkNew<vtkStaticCompositeSource> source;
  source->SetStartData(1);

  vtkNew<vtkForceStaticMesh> forceStatic;
  forceStatic->SetInputConnection(source->GetOutputPort());
  forceStatic->Update();

  auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(source->GetOutputDataObject(0));
  std::vector<vtkMTimeType> beforeVaryingMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, beforeVaryingMeshMTimes);

  outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(forceStatic->GetOutputDataObject(0));
  std::vector<vtkMTimeType> beforeMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, beforeMeshMTimes);

  // Update the source: the mesh has the same point/cell count but different values.
  // it should be cached by the forceStaticMesh filter anyway
  source->SetStartData(5);
  forceStatic->Update();

  outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(source->GetOutputDataObject(0));
  std::vector<vtkMTimeType> afterVaryingMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, afterVaryingMeshMTimes);

  outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(forceStatic->GetOutputDataObject(0));
  std::vector<vtkMTimeType> afterMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, afterMeshMTimes);

  for (size_t timeIdx = 0; timeIdx != afterMeshMTimes.size(); ++timeIdx)
  {
    // Also compare the input's meshMTime to make sure we're testing something
    if (beforeVaryingMeshMTimes[timeIdx] == afterVaryingMeshMTimes[timeIdx])
    {
      vtkLog(WARNING,
        "ForceStaticMesh's input's MeshMTime has not changed, this test does not test anything ! "
        "Was static mesh support added to the input filter ?");
    }

    if (beforeMeshMTimes[timeIdx] != afterMeshMTimes[timeIdx])
    {
      vtkLog(ERROR, "GetMeshMTime has changed, mesh not static !");
      return EXIT_FAILURE;
    }
  }

  if (!::TestOutputDataTime())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
