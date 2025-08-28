// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkExodusIIReader.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
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
bool TestDuplicatePoints(vtkMultiProcessController* controller)
{
  int myrank = controller->GetLocalProcessId();

  vtkNew<vtkRTAnalyticSource> wavelet;
  if (myrank == 0)
  {
    wavelet->SetWholeExtent(-10, 0, -10, 10, -10, 10);
  }
  else if (myrank == 1)
  {
    wavelet->SetWholeExtent(0, 10, -10, 10, -10, 10);
  }

  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetInputConnection(wavelet->GetOutputPort());
  redistribute->SetNumberOfPartitions(4);
  redistribute->Update();

  vtkDataSet* waveletDS = vtkDataSet::SafeDownCast(wavelet->GetOutputDataObject(0));
  vtkDataSet* redistributedDS = vtkDataSet::SafeDownCast(redistribute->GetOutputDataObject(0));

  return waveletDS->GetNumberOfPoints() == redistributedDS->GetNumberOfPoints();
}

bool TestMultiBlockEmptyOnAllRanksButZero(vtkMultiProcessController* controller)
{
  // See !8745
  int myrank = controller->GetLocalProcessId();
  vtkNew<vtkMultiBlockDataSet> mb;
  mb->SetNumberOfBlocks(1);
  if (myrank == 0)
  {
    vtkNew<vtkImageData> im;
    im->SetDimensions(10, 10, 10);
    mb->SetBlock(0, im);
  }

  vtkNew<vtkRedistributeDataSetFilter> filter;
  filter->SetInputData(mb);
  filter->SetController(controller);
  filter->Update();

  auto output = vtkMultiBlockDataSet::SafeDownCast(filter->GetOutputDataObject(0));
  if (output->GetNumberOfBlocks() != 1)
  {
    vtkLog(ERROR, "Wrong number of blocks in output");
    return false;
  }
  if (!output->GetBlock(0))
  {
    vtkLog(ERROR, "Output block should not be nullptr in rank " << myrank);
    return false;
  }
  return true;
}

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
    vtkLogF(ERROR,
      "incorrect global cell ids! expected %" VTK_ID_TYPE_PRId ", actual %" VTK_ID_TYPE_PRId,
      input->GetNumberOfCells() - 1, global_cellid_max);
    return false;
  }

  std::vector<vtkDataSet*> datasets = vtkCompositeDataSet::GetDataSets(output);
  for (vtkDataSet* ds : datasets)
  {
    vtkUnsignedCharArray* ghosts = ds->GetPointData()->GetGhostArray();
    for (vtkIdType i = 0; i < ghosts->GetNumberOfValues(); ++i)
    {
      if (ghosts->GetValue(i) != vtkDataSetAttributes::HIDDENPOINT)
      {
        vtkLog(ERROR, "Output ghost points has wrong value.");
        return false;
      }
    }

    // We need to not have a ghost array down the road
    ds->GetPointData()->RemoveArray(ghosts->GetName());
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

  if (!TestMultiBlockEmptyOnAllRanksButZero(controller))
  {
    return EXIT_FAILURE;
  }

  // See paraview/paraview#21161
  if (!TestDuplicatePoints(controller))
  {
    vtkLog(ERROR,
      "Wrong number of output points when applying the filter on a wavelet source."
      " The most likely reason for that is if the filter produced duplicated points.");
    return EXIT_FAILURE;
  }

  const int rank = controller->GetLocalProcessId();
  vtkLogger::SetThreadName("rank:" + std::to_string(rank));

  vtkNew<vtkUnstructuredGrid> data;
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

    data->ShallowCopy(vtkUnstructuredGrid::SafeDownCast(
      vtkMultiBlockDataSet::SafeDownCast(rdr->GetOutput()->GetBlock(0))->GetBlock(0)));

    std::vector<vtkDataSet*> datasets = vtkCompositeDataSet::GetDataSets(data);
    for (vtkDataSet* ds : datasets)
    {
      // Adding some duplicate ghost cells and ghost points.
      // The filter is supposed to only remove duplicates from the output.
      vtkNew<vtkUnsignedCharArray> ghosts;
      ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
      ghosts->SetNumberOfValues(ds->GetNumberOfPoints());
      ghosts->Fill(vtkDataSetAttributes::DUPLICATEPOINT | vtkDataSetAttributes::HIDDENPOINT);
      ds->GetPointData()->AddArray(ghosts);
    }
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
