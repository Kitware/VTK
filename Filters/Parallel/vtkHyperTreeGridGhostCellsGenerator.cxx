// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkHyperTreeGridGhostCellsGeneratorInternals.h"

#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkSetGet.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridGhostCellsGenerator);
vtkCxxSetObjectMacro(vtkHyperTreeGridGhostCellsGenerator, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkHyperTreeGridGhostCellsGenerator::vtkHyperTreeGridGhostCellsGenerator()
{
  this->AppropriateOutput = true;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkHyperTreeGridGhostCellsGenerator::~vtkHyperTreeGridGhostCellsGenerator()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkHyperTreeGridGhostCellsGenerator::GetController()
{
  return this->Controller.Get();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGenerator::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGenerator::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->UpdateProgress(0.);

  vtkInformation* info = outputVector->GetInformationObject(0);
  int currentPiece = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  // Make sure input is either a HTG or a PartitionedDataSet that contains a HTG piece.
  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::GetData(inputVector[0], 0);
  vtkPartitionedDataSet* inputPDS = vtkPartitionedDataSet::GetData(inputVector[0], 0);

  if (!inputPDS && !inputHTG)
  {
    vtkErrorMacro("Input data is neither HTG or PartitionedDataSet. Cannot proceed with ghost cell "
                  "generation.");
    return 0;
  }

  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::GetData(outputVector, 0);
  vtkPartitionedDataSet* outputPDS = vtkPartitionedDataSet::GetData(outputVector, 0);
  if (outputPDS)
  {
    outputPDS->CopyStructure(inputPDS);
  }

  // When the filter receives a PartitionedDataSet, the data for the current rank can be in either
  // partition, depending on the data generation method. We survey the partitions to find the one
  // that contains the actual data. There should be exactly one non-null HTG partition in each
  // piece. If we find multiple, the HTG structure is not capable of merging multiple grids, so we
  // simply use the last one.
  if (inputPDS && outputPDS)
  {
    for (unsigned int partId = 0; partId < inputPDS->GetNumberOfPartitions(); partId++)
    {
      auto partHTG = vtkHyperTreeGrid::SafeDownCast(inputPDS->GetPartitionAsDataObject(partId));
      if (partHTG)
      {
        if (inputHTG)
        {
          vtkWarningMacro("Found more than one non-null HTG in the partitioned dataset for piece "
            << currentPiece << ". Generating ghost data only for partition " << partId);
        }
        inputHTG = partHTG;
        vtkNew<vtkHyperTreeGrid> newOutputHTG;
        outputPDS->SetPartition(partId, newOutputHTG);
        outputHTG = newOutputHTG; // Not dangling, outputPDS maintains a reference.
      }
    }
  }

  if (!outputHTG && !outputPDS)
  {
    vtkErrorMacro("No output available. Cannot proceed with hyper tree grid algorithm.");
    return 0;
  }

  if (!inputHTG)
  {
    vtkWarningMacro("Incorrect HTG for piece " << currentPiece);
  }

  // Make sure every HTG piece has a correct extent and can be processed.
  // This way, we make sure the `ProcessTrees` function will either be executed by all ranks
  // or by none, and avoids getting stuck on barriers.
  int correctExtent = inputHTG && inputHTG->GetExtent()[0] <= inputHTG->GetExtent()[1] &&
    inputHTG->GetExtent()[2] <= inputHTG->GetExtent()[3] &&
    inputHTG->GetExtent()[4] <= inputHTG->GetExtent()[5];

  if (!correctExtent)
  {
    vtkWarningMacro("Piece " << currentPiece << " does not have a valid extend. Cannot process.");
  }

  int allCorrect = 1; // Reduction operation cannot be done on bools
  this->Controller->AllReduce(&correctExtent, &allCorrect, 1, vtkCommunicator::LOGICAL_AND_OP);

  if (!allCorrect)
  {
    vtkWarningMacro("Every individual distributed process does not have a valid HTG extent. No "
                    "ghost cells will be generated.");
    if (outputHTG)
    {
      outputHTG->ShallowCopy(inputHTG);
    }
    return 1;
  }
  else if (!this->ProcessTrees(inputHTG, outputHTG))
  {
    return 0;
  }

  // Update progress and return
  this->UpdateProgress(1.);
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGenerator::ProcessTrees(
  vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  int numberOfProcesses = this->Controller->GetNumberOfProcesses();

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

  vtkHyperTreeGridGhostCellsGeneratorInternals subroutines{ this, this->Controller, input, output };
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
  this->Controller->Barrier();
  this->UpdateProgress(0.4);

  vtkDebugMacro("Exchange tree decomposition and masks with neighbors");
  if (subroutines.ExchangeTreeDecomposition() == 0)
  {
    vtkErrorMacro("Failure during mask exchange, aborting.");
    return 0;
  }
  this->Controller->Barrier();
  this->UpdateProgress(0.6);

  vtkDebugMacro("Exchange cell data with neighbors");
  if (subroutines.ExchangeCellData() == 0)
  {
    vtkErrorMacro("Failure during cell data exchange, aborting.");

    return 0;
  }
  this->Controller->Barrier();
  this->UpdateProgress(0.8);

  vtkDebugMacro("Create ghost array and set output mask");
  subroutines.FinalizeCellData();

  this->UpdateProgress(1.);
  return 1;
}

VTK_ABI_NAMESPACE_END
