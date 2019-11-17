/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRedistributeDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

===========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkExodusIIReader.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPartitionedDataSet.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/mpi.hpp)
// clang-format on

namespace
{
bool ValidateDataset(
  vtkUnstructuredGrid* input, vtkPartitionedDataSet* output, vtkMultiProcessController* controller)
{
  const int rank = controller->GetLocalProcessId();
  vtkIdType local_cellid_max = 0;
  for (unsigned int part = 0; part < output->GetNumberOfPartitions(); ++part)
  {
    if (auto ds = vtkDataSet::SafeDownCast(output->GetPartition(part)))
    {
      if (auto gcids = vtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetGlobalIds()))
      {
        local_cellid_max =
          std::max(static_cast<vtkIdType>(gcids->GetRange(0)[1]), local_cellid_max);
      }
    }
  }

  vtkIdType global_cellid_max;
  controller->AllReduce(&local_cellid_max, &global_cellid_max, 1, vtkCommunicator::MAX_OP);
  if (rank == 0 && global_cellid_max != input->GetNumberOfCells() - 1)
  {
    vtkLogF(ERROR, "incorrect global cell ids! expected %lld, actual %lld",
      input->GetNumberOfCells() - 1, global_cellid_max);
    return false;
  }

  return true;
}

}

int TestRedistributeDataSetFilter(int argc, char* argv[])
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

  vtkSmartPointer<vtkUnstructuredGrid> data;
  if (rank == 0)
  {
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/disk_out_ref.ex2");
    if (!fname)
    {
      vtkLogF(ERROR, "Could not obtain filename for test data.");
      return EXIT_FAILURE;
    }

    vtkNew<vtkExodusIIReader> rdr;
    if (!rdr->CanReadFile(fname))
    {
      vtkLogF(ERROR, "Cannot read `%s`", fname);
      return 1;
    }

    rdr->SetFileName(fname);
    delete[] fname;
    rdr->Update();

    data = vtkUnstructuredGrid::SafeDownCast(
      vtkMultiBlockDataSet::SafeDownCast(rdr->GetOutput()->GetBlock(0))->GetBlock(0));
  }
  else
  {
    data = vtkSmartPointer<vtkUnstructuredGrid>::New();
  }

  vtkNew<vtkRedistributeDataSetFilter> rdsf;
  rdsf->SetInputDataObject(data);
  rdsf->SetNumberOfPartitions(16);
  rdsf->GenerateGlobalCellIdsOn();
  rdsf->PreservePartitionsInOutputOn();
  rdsf->Update();

  if (!ValidateDataset(
        data, vtkPartitionedDataSet::SafeDownCast(rdsf->GetOutputDataObject(0)), controller))
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkDataSetSurfaceFilter> dsf;
  dsf->SetInputConnection(rdsf->GetOutputPort());

  vtkNew<vtkRandomAttributeGenerator> rag;
  rag->SetDataTypeToDouble();
  rag->SetNumberOfComponents(1);
  rag->SetComponentRange(0, 1.0);
  rag->GenerateCellScalarsOn();
  rag->AttributesConstantPerBlockOn();
  rag->SetInputConnection(dsf->GetOutputPort());

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

    if (auto camera = renderer->GetActiveCamera())
    {
      camera->SetFocalPoint(-0.531007, -1.16954, -1.12284);
      camera->SetPosition(8.62765, 28.0586, -33.585);
      camera->SetViewUp(-0.373065, 0.739388, 0.560472);
    }

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
