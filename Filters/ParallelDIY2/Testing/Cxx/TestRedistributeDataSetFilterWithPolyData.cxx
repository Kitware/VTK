// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkExodusIIReader.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

int TestRedistributeDataSetFilterWithPolyData(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);
  const int rank = controller->GetLocalProcessId();
  vtkLogger::SetThreadName("rank:" + std::to_string(rank));

  vtkNew<vtkPolyData> pd;
  if (controller->GetLocalProcessId() == 0)
  {
    vtkNew<vtkXMLPolyDataReader> reader;
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
    reader->SetFileName(fname);
    delete[] fname;
    reader->Update();
    pd->ShallowCopy(reader->GetOutputDataObject(0));
  }

  vtkNew<vtkRedistributeDataSetFilter> rdsf;
  rdsf->SetInputDataObject(pd);
  rdsf->SetNumberOfPartitions(16);
  rdsf->PreservePartitionsInOutputOn();

  vtkNew<vtkDataSetSurfaceFilter> dsf;
  dsf->SetInputConnection(rdsf->GetOutputPort());

  vtkNew<vtkRandomAttributeGenerator> rag;
  rag->SetInputConnection(dsf->GetOutputPort());
  rag->SetDataTypeToDouble();
  rag->SetNumberOfComponents(1);
  rag->SetComponentRange(0, 1.0);
  rag->GenerateCellScalarsOn();
  rag->AttributesConstantPerBlockOn();

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(rag->GetOutputPort());

  vtkNew<vtkCompositeRenderManager> prm;
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::Take(prm->MakeRenderer());
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::Take(prm->MakeRenderWindow());
  renWin->AddRenderer(renderer);
  renWin->DoubleBufferOn();
  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  prm->SetRenderWindow(renWin);
  prm->SetController(controller);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  int retVal = 1;
  if (rank == 0)
  {
    prm->ResetAllCameras();
    renWin->Render();
    retVal = vtkRegressionTestImage(renWin);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      prm->StartInteractor();
    }
    controller->TriggerBreakRMIs();
  }
  else
  {
    prm->StartServices();
  }

  controller->Broadcast(&retVal, 1, 0);
  controller->Finalize();
  vtkMultiProcessController::SetGlobalController(nullptr);
  return !retVal;
}
