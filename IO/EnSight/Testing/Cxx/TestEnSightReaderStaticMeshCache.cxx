// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkEnSightGoldCombinedReader.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkTestUtilities.h"
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"

//------------------------------------------------------------------------------
int TestEnSightReaderStaticMeshCache(int argc, char* argv[])
{
  vtkLog(INFO, "Test default cache");
  bool success = true;

  vtkNew<vtkEnSightGoldCombinedReader> enSightReader;
  enSightReader->SetCaseFileName(
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/EnSight/viga.case"));

  enSightReader->UpdateTimeStep(0);
  auto outDO = enSightReader->GetOutputDataObject(0);
  auto outPDSC = vtkPartitionedDataSetCollection::SafeDownCast(outDO);
  auto outUG = vtkUnstructuredGrid::SafeDownCast(outPDSC->GetPartition(0, 0));

  vtkMTimeType beforeMeshMTime = outUG->GetMeshMTime();

  enSightReader->UpdateTimeStep(1);
  outDO = enSightReader->GetOutputDataObject(0);
  outPDSC = vtkPartitionedDataSetCollection::SafeDownCast(outDO);
  outUG = vtkUnstructuredGrid::SafeDownCast(outPDSC->GetPartition(0, 0));

  vtkMTimeType afterMeshMTime = outUG->GetMeshMTime();

  if (afterMeshMTime != beforeMeshMTime)
  {
    vtkLog(ERROR, "MeshMTime differs between time steps. Mesh was not properly cached.");
    success = false;
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
