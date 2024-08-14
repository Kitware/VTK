// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "MeshCacheMockAlgorithms.h"
#include "vtkForceStaticMesh.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"

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
// Program main
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

  return EXIT_SUCCESS;
}
