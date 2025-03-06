// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Description:
// This tests reading of an EnSight Gold SOS file using MPI.

#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDataArraySelection.h"
#include "vtkEnSightSOSGoldReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCompositeRenderManager.h"
#include "vtkGeometryFilter.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    int success = 1;                                                                               \
    if (!(x))                                                                                      \
    {                                                                                              \
      cerr << "ERROR: Condition FAILED!! : " << #x << endl;                                        \
      success = 0;                                                                                 \
    }                                                                                              \
    int allSuccess;                                                                                \
    controller->AllReduce(&success, &allSuccess, 1, vtkCommunicator::LOGICAL_AND_OP);              \
    if (!success || !allSuccess)                                                                   \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
  } while (false)

struct TestArgs
{
  int* retval;
  int argc;
  char** argv;
};

void TestEnSightSOSReaderMandelbrotMPI(vtkMultiProcessController* controller, void* _args)
{
  TestArgs* args = reinterpret_cast<TestArgs*>(_args);
  int argc = args->argc;
  char** argv = args->argv;
  *(args->retval) = 1;
  auto rank = controller->GetLocalProcessId();

  vtkNew<vtkEnSightSOSGoldReader> reader;

  char* filePath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/EnSight/mandelbrot.sos");

  if (!reader->CanReadFile(filePath))
  {
    std::cerr << "Cannot read file " << reader->GetCaseFileName() << std::endl;
    return;
  }

  reader->SetCaseFileName(filePath);
  delete[] filePath;

  reader->SetController(controller);

  reader->UpdateInformation();
  auto selection = reader->GetPartSelection();
  selection->DisableAllArrays();
  selection->EnableArray("Part 1");

  reader->Update();

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfPartitionedDataSets() == 1);
  for (unsigned int i = 0; i < output->GetNumberOfPartitionedDataSets(); i++)
  {
    auto pds = output->GetPartitionedDataSet(i);
    vtk_assert(pds->GetNumberOfPartitions() == 1);
    vtk_assert(pds->GetPartition(0)->GetPointData()->GetNumberOfArrays() == 1);
    vtk_assert(pds->GetPartition(0)->GetCellData()->GetNumberOfArrays() == 0);
  }

  selection->EnableAllArrays();
  reader->Update();

  output = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfPartitionedDataSets() == 2);
  for (unsigned int i = 0; i < output->GetNumberOfPartitionedDataSets(); i++)
  {
    auto pds = output->GetPartitionedDataSet(i);
    vtk_assert(pds->GetNumberOfPartitions() == 1);
    vtk_assert(pds->GetPartition(0)->GetPointData()->GetNumberOfArrays() == 1);
    vtk_assert(pds->GetPartition(0)->GetCellData()->GetNumberOfArrays() == 0);
  }

  vtkNew<vtkGeometryFilter> geomFilter;
  geomFilter->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(geomFilter->GetOutputPort());
  mapper->ColorByArrayComponent("Iterations", 0);
  mapper->SetScalarRange(1.9, 100);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkCompositeRenderManager> prm;

  vtkSmartPointer<vtkRenderer> renderer;
  renderer.TakeReference(prm->MakeRenderer());
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  // renderer->ResetCamera();

  vtkSmartPointer<vtkRenderWindow> rendWin;
  rendWin.TakeReference(prm->MakeRenderWindow());
  rendWin->SetSize(300, 300);
  rendWin->AddRenderer(renderer);
  // rendWin->SetPosition(0, 360 * rank);

  prm->SetRenderWindow(rendWin);
  prm->SetController(controller);
  prm->InitializePieces();
  prm->InitializeOffScreen(); // Mesa GL only

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(rendWin);

  if (rank == 0)
  {
    prm->ResetAllCameras();
    rendWin->Render();

    // Do the test comparison
    int retval = vtkRegressionTestImage(rendWin);
    if (retval == vtkRegressionTester::DO_INTERACTOR)
    {
      prm->StartInteractor();
      retval = vtkRegressionTester::PASSED;
    }
    *(args->retval) = (retval == vtkRegressionTester::PASSED) ? 0 : 1;

    prm->StopServices();
  }
  else // not root node
  {
    prm->StartServices();
  }

  controller->Broadcast(args->retval, 1, 0);
}

int TestEnSightSOSReaderMandelbrotMPI(int argc, char* argv[])
{
  int retval = 1;
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  vtkMultiProcessController::SetGlobalController(controller);

  TestArgs args;
  args.retval = &retval;
  args.argc = argc;
  args.argv = argv;

  controller->SetSingleMethod(TestEnSightSOSReaderMandelbrotMPI, &args);
  controller->SingleMethodExecute();

  controller->Finalize();

  return retval;
}
