// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkEnSightGoldCombinedReader.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkType.h"

#include <vector>

namespace
{
//------------------------------------------------------------------------------
void GetPartitionnedMeshMTimes(
  vtkPartitionedDataSetCollection* pdsc, std::vector<vtkMTimeType>& times)
{
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(pdsc->NewIterator());
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    vtkMTimeType time = ds->GetMeshMTime();
    times.push_back(time);
  }
}
}

//------------------------------------------------------------------------------
int TestEnSightReaderStaticMeshCache(int argc, char* argv[])
{
  vtkLog(INFO, "Test EnSight Combined Reader static mesh cache");
  bool success = true;

  vtkNew<vtkEnSightGoldCombinedReader> enSightReader;
  enSightReader->SetCaseFileName(
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/EnSight/elements.case"));

  enSightReader->UpdateTimeStep(0);
  auto outDO = enSightReader->GetOutputDataObject(0);
  auto outPDSC = vtkPartitionedDataSetCollection::SafeDownCast(outDO);

  std::vector<vtkMTimeType> beforeMeshMTimes;
  ::GetPartitionnedMeshMTimes(outPDSC, beforeMeshMTimes);

  enSightReader->UpdateTimeStep(1);
  outDO = enSightReader->GetOutputDataObject(0);
  outPDSC = vtkPartitionedDataSetCollection::SafeDownCast(outDO);

  std::vector<vtkMTimeType> afterMeshMTimes;
  ::GetPartitionnedMeshMTimes(outPDSC, afterMeshMTimes);

  for (auto beforeIt = beforeMeshMTimes.begin(), afterIt = afterMeshMTimes.begin();
       beforeIt != beforeMeshMTimes.end() && afterIt != afterMeshMTimes.end();
       ++beforeIt, ++afterIt)
  {
    if (*beforeIt != *afterIt)
    {
      vtkLog(ERROR, "MeshMTime differs between time steps. Mesh was not properly cached.");
      success = false;
    }
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
