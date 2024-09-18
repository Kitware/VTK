// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Test vtkRedistributeDataSetFilter can operate on vtkIOSSReader output and
 * produce correct cell ids.
 * Tests: paraview/paraview#20438
 */

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSet.h"
#include "vtkIOSSReader.h"
#include "vtkLogger.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkTestUtilities.h"
#include "vtkVector.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

namespace
{
std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

}

int TestRedistributeDataSetFilterOnIOSS(int argc, char* argv[])
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

  vtkNew<vtkIOSSReader> reader;
  auto fname = GetFileName(argc, argv, std::string("Data/Exodus/can.e.4/can.e.4.0"));
  reader->SetFileName(fname.c_str());
  reader->UpdateInformation();
  reader->ReadIdsOff(); // turn off global ids
  for (int etype = vtkIOSSReader::BLOCK_START; etype < vtkIOSSReader::BLOCK_END; ++etype)
  {
    // enable all blocks.
    reader->GetEntitySelection(etype)->EnableAllArrays();
  }

  vtkNew<vtkRedistributeDataSetFilter> rdsf;
  rdsf->SetInputConnection(reader->GetOutputPort());
  rdsf->SetNumberOfPartitions(16);
  rdsf->GenerateGlobalCellIdsOn();
  rdsf->LoadBalanceAcrossAllBlocksOff(); // let's test this other mode.
  rdsf->UpdatePiece(controller->GetLocalProcessId(), controller->GetNumberOfProcesses(), 0);

  auto data = vtkDataObjectTree::SafeDownCast(rdsf->GetOutputDataObject(0));

  vtkVector2d range{ VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  using Opts = vtk::DataObjectTreeOptions;
  for (auto dobj :
    vtk::Range(data, Opts::SkipEmptyNodes | Opts::TraverseSubTree | Opts::VisitOnlyLeaves))
  {
    if (auto ds = vtkDataSet::SafeDownCast(dobj))
    {
      const vtkVector2d crange{ ds->GetCellData()->GetArray("vtkGlobalCellIds")->GetRange(0) };
      range[0] = std::min(range[0], crange[0]);
      range[1] = std::max(range[1], crange[1]);
    }
  }

  vtkLogF(INFO, "local range: %g, %g", range[0], range[1]);
  if (controller->GetNumberOfProcesses() > 1)
  {
    vtkVector2d globalRange;
    controller->AllReduce(&range[0], &globalRange[0], 1, vtkCommunicator::MIN_OP);
    controller->AllReduce(&range[1], &globalRange[1], 1, vtkCommunicator::MAX_OP);
    range = globalRange;
    vtkLogF(INFO, "global range: %g, %g", range[0], range[1]);
  }

  controller->Barrier();
  controller->Finalize();
  vtkMultiProcessController::SetGlobalController(nullptr);
  return (range[0] == 0 && range[1] == 7151) ? EXIT_SUCCESS : EXIT_FAILURE;
}
