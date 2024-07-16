// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkHyperTreeGridGhostCellsGeneratorInternals.h"

#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSetGet.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridGhostCellsGenerator);

//------------------------------------------------------------------------------
vtkHyperTreeGridGhostCellsGenerator::vtkHyperTreeGridGhostCellsGenerator()
{
  this->AppropriateOutput = true;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGenerator::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGenerator::ProcessTrees(
  vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int numberOfProcesses = controller->GetNumberOfProcesses();

  if (!input || input->GetNumberOfNonEmptyTrees() == 0)
  {
    return 1;
  }

  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  if (input->HasAnyGhostCells())
  {
    vtkWarningMacro("Ghost cells already computed, we reuse them.");
    output->ShallowCopy(input);
    return 1;
  }

  vtkDebugMacro(<< "Start processing trees: copy input structure");
  output->Initialize();
  if (numberOfProcesses == 1)
  {
    // No ghost cells to generate if we have a single process, pass through
    output->ShallowCopy(input);
    return 1;
  }
  else
  {
    output->CopyEmptyStructure(input);
    output->GetCellData()->CopyStructure(input->GetCellData());
  }

  vtkHyperTreeGridGhostCellsGeneratorInternals subroutines{ this, controller, input, output };
  subroutines.InitializeCellData();
  this->UpdateProgress(0.1);

  // Create a vector containing the processId of each consecutive tree in the HTG.
  vtkDebugMacro("Broadcast tree locations");
  subroutines.BroadcastTreeLocations();
  this->UpdateProgress(0.2);

  vtkDebugMacro("Determine neighbors");
  subroutines.DetermineNeighbors();
  this->UpdateProgress(0.3);

  vtkDebugMacro("Exchange sizes with neighbors");
  if (subroutines.ExchangeSizes() == 0)
  {
    vtkErrorMacro("Failure during size exchange, aborting.");
    return 0;
  }
  controller->Barrier();
  this->UpdateProgress(0.4);

  vtkDebugMacro("Exchange tree decomposition and masks with neighbors");
  if (subroutines.ExchangeTreeDecomposition() == 0)
  {
    vtkErrorMacro("Failure during mask exchange, aborting.");
    return 0;
  }
  controller->Barrier();
  this->UpdateProgress(0.6);

  vtkDebugMacro("Exchange cell data with neighbors");
  if (subroutines.ExchangeCellData() == 0)
  {
    vtkErrorMacro("Failure during cell data exchange, aborting.");

    return 0;
  }
  controller->Barrier();
  this->UpdateProgress(0.8);

  vtkDebugMacro("Create ghost array and set output mask");
  subroutines.FinalizeCellData();

  this->UpdateProgress(1.);
  return 1;
}

VTK_ABI_NAMESPACE_END
