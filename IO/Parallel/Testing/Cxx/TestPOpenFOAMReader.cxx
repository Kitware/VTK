// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkPOpenFOAMReader.h"

#include "vtkCellData.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositeRenderManager.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestPOpenFOAMReader(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  int rank = controller->GetLocalProcessId();
  vtkLogger::SetThreadName("rank=" + std::to_string(rank));
  vtkMultiProcessController::SetGlobalController(controller);

  // Read file name.
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/cavity/cavity.foam");
  std::cerr << filename << std::endl;

  // Read the file
  vtkNew<vtkPOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;
  reader->SetCaseType(vtkPOpenFOAMReader::RECONSTRUCTED_CASE);
  reader->Update();

  reader->SetTimeValue(.5);
  //  reader->CreateCellToPointOn();
  reader->ReadZonesOn();
  reader->Update();
  reader->Print(std::cout);
  reader->GetOutput()->Print(std::cout);

  vtkNew<vtkCompositeDataGeometryFilter> geom;
  geom->SetInputConnection(reader->GetOutputPort(0));

  // may have empty block on parallel.
  vtkUnstructuredGrid* block0 = vtkUnstructuredGrid::SafeDownCast(reader->GetOutput()->GetBlock(0));
  if (block0)
  {
    block0->Print(std::cout);

    // 1) Default array settings
    int numberOfCellArrays = reader->GetNumberOfCellArrays();
    std::cout << "----- Default array settings" << std::endl;
    for (int i = 0; i < numberOfCellArrays; ++i)
    {
      const char* name = reader->GetCellArrayName(i);
      std::cout << "  Cell Array: " << i << " is named " << name << " and is "
                << (reader->GetCellArrayStatus(name) ? "Enabled" : "Disabled") << std::endl;
    }

    int numberOfPointArrays = reader->GetNumberOfPointArrays();
    std::cout << "----- Default array settings" << std::endl;
    for (int i = 0; i < numberOfPointArrays; ++i)
    {
      const char* name = reader->GetPointArrayName(i);
      std::cout << "  Point Array: " << i << " is named " << name << " and is "
                << (reader->GetPointArrayStatus(name) ? "Enabled" : "Disabled") << std::endl;
    }

    int numberOfLagrangianArrays = reader->GetNumberOfLagrangianArrays();
    std::cout << "----- Default array settings" << std::endl;
    for (int i = 0; i < numberOfLagrangianArrays; ++i)
    {
      const char* name = reader->GetLagrangianArrayName(i);
      std::cout << "  Lagrangian Array: " << i << " is named " << name << " and is "
                << (reader->GetLagrangianArrayStatus(name) ? "Enabled" : "Disabled") << std::endl;
    }

    int numberOfPatchArrays = reader->GetNumberOfPatchArrays();
    std::cout << "----- Default array settings" << std::endl;
    for (int i = 0; i < numberOfPatchArrays; ++i)
    {
      const char* name = reader->GetPatchArrayName(i);
      std::cout << "  Patch Array: " << i << " is named " << name << " and is "
                << (reader->GetPatchArrayStatus(name) ? "Enabled" : "Disabled") << std::endl;
    }

    block0->GetCellData()->SetActiveScalars("p");
    std::cout << "Scalar range: " << block0->GetCellData()->GetScalars()->GetRange()[0] << ", "
              << block0->GetCellData()->GetScalars()->GetRange()[1] << std::endl;
  }

  // Visualize
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom->GetOutputPort(0));
  if (block0)
  {
    mapper->SetScalarRange(block0->GetScalarRange());
  }

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkCompositeRenderManager> crm;
  vtkSmartPointer<vtkRenderer> renderer;
  renderer.TakeReference(crm->MakeRenderer());

  vtkSmartPointer<vtkRenderWindow> renderWindow;
  renderWindow.TakeReference(crm->MakeRenderWindow());
  renderWindow->AddRenderer(renderer);
  crm->SetRenderWindow(renderWindow);
  crm->SetController(controller);
  crm->InitializePieces();

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(.2, .4, .6);

  int retVal = 0;
  if (rank == 0)
  {
    renderWindow->Render();
    retVal = vtkRegressionTestImage(renderWindow);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      crm->StartInteractor();
    }
    controller->TriggerBreakRMIs();
  }
  else
  {
    crm->StartServices();
  }
  controller->Barrier();
  controller->Broadcast(&retVal, 1, 0);

  controller->Finalize();

  return !retVal;
}
