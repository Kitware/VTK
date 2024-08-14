// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkDataSetMapper.h"
#include "vtkForceStaticMesh.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkType.h"
#include <cstdlib>

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
  // Create the pipeline to produce the initial grid
  vtkNew<vtkConeSource> cone1;
  cone1->SetResolution(15);

  vtkNew<vtkConeSource> cone2;
  cone2->SetResolution(12);
  cone2->SetCenter(2, 2, 2);

  vtkNew<vtkGroupDataSetsFilter> group;
  group->SetInputConnection(0, cone1->GetOutputPort());
  group->AddInputConnection(0, cone2->GetOutputPort());
  group->SetOutputTypeToPartitionedDataSetCollection();

  vtkNew<vtkGenerateTimeSteps> addTime;
  addTime->SetInputConnection(group->GetOutputPort());
  addTime->AddTimeStepValue(0);
  addTime->AddTimeStepValue(1);
  addTime->AddTimeStepValue(2);
  addTime->AddTimeStepValue(3);

  vtkNew<vtkRandomAttributeGenerator> addScalars;
  addScalars->SetInputConnection(addTime->GetOutputPort());
  addScalars->GenerateAllDataOff();
  addScalars->GenerateCellScalarsOn();
  addScalars->SetDataTypeToDouble();
  addScalars->SetComponentRange(0, 30);

  vtkNew<vtkForceStaticMesh> forceStatic;
  forceStatic->SetInputConnection(addScalars->GetOutputPort());
  forceStatic->Update();

  auto outputPDC =
    vtkPartitionedDataSetCollection::SafeDownCast(forceStatic->GetOutputDataObject(0));

  std::vector<vtkMTimeType> beforeMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, beforeMeshMTimes);

  forceStatic->UpdateTimeStep(2); // update scalars, not mesh

  outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(forceStatic->GetOutputDataObject(0));
  std::vector<vtkMTimeType> afterMeshMTimes;
  ::GetPartitionsMeshMTimes(outputPDC, afterMeshMTimes);

  for (auto beforeIt = beforeMeshMTimes.begin(), afterIt = afterMeshMTimes.begin();
       beforeIt != beforeMeshMTimes.end() && afterIt != afterMeshMTimes.end();
       ++beforeIt, ++afterIt)
  {
    if (*beforeIt != *afterIt)
    {
      vtkLog(ERROR, "GetMeshMTime has changed, mesh not static !");
      return EXIT_FAILURE;
    }
  }

  auto outputPD = vtkPolyData::SafeDownCast(outputPDC->GetPartitionAsDataObject(0, 0));

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputData(outputPD);
  mapper->SetScalarRange(0, 30);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);

  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
