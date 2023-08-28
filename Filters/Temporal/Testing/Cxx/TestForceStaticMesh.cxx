// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkDataSetMapper.h"
#include "vtkForceStaticMesh.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

//------------------------------------------------------------------------------
// Program main
int TestForceStaticMesh(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create the pipeline to produce the initial grid
  vtkNew<vtkConeSource> cone;
  cone->SetResolution(15);
  vtkNew<vtkGenerateTimeSteps> addTime;
  addTime->SetInputConnection(cone->GetOutputPort());
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

  vtkPolyData* outputPD = vtkPolyData::SafeDownCast(forceStatic->GetOutputDataObject(0));
  const auto initMeshTime = outputPD->GetMeshMTime();
  forceStatic->UpdateTimeStep(2); // update scalars, not mesh
  outputPD = vtkPolyData::SafeDownCast(forceStatic->GetOutputDataObject(0));
  if (outputPD->GetMeshMTime() != initMeshTime)
  {
    std::cerr << "ERROR: GetMeshMTime has changed, mesh not static !" << std::endl;
  }

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(forceStatic->GetOutputPort());
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
