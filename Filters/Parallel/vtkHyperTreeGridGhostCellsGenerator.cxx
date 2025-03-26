// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkHyperTreeGridGhostCellsGeneratorInternals.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridGhostCellsGenerator);

namespace
{

/**
 * Recursively copy the input tree (cell data and mask information)
 * pointed by the cursor to the output.
 * Fills memory gaps if present.
 */
void CopyInputTreeToOutput(vtkHyperTreeGridNonOrientedCursor* inCursor,
  vtkHyperTreeGridNonOrientedCursor* outCursor, vtkCellData* inCellData, vtkCellData* outCellData,
  vtkBitArray* inMask, vtkBitArray* outMask)
{
  vtkIdType outIdx = outCursor->GetGlobalNodeIndex();
  vtkIdType inIdx = inCursor->GetGlobalNodeIndex();
  if (inMask)
  {
    outMask->InsertTuple1(outIdx, inMask->GetValue(inIdx));
  }
  outCellData->InsertTuple(outIdx, inIdx, inCellData);
  if (!inCursor->IsMasked())
  {
    if (!inCursor->IsLeaf())
    {
      outCursor->SubdivideLeaf();
      for (int ichild = 0; ichild < inCursor->GetNumberOfChildren(); ++ichild)
      {
        outCursor->ToChild(ichild);
        inCursor->ToChild(ichild);
        ::CopyInputTreeToOutput(inCursor, outCursor, inCellData, outCellData, inMask, outMask);
        outCursor->ToParent();
        inCursor->ToParent();
      }
    }
  }
}

/**
 * ProcessTree subroutine copying the input tree to the output (cell and mask data information)
 * We do it "by hand" to fill gaps if they exist.
 *
 * Return the number of vertices in output trree
 */
vtkIdType CopyInputHyperTreeToOutput(
  vtkHyperTreeGrid* input, vtkHyperTreeGrid* output, vtkBitArray* outputMask)
{
  vtkBitArray* inputMask = input->HasMask() ? input->GetMask() : nullptr;

  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor, inCursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  vtkIdType inTreeIndex = 0, totalVertices = 0;
  input->InitializeTreeIterator(inputIterator);

  while (inputIterator.GetNextTree(inTreeIndex))
  {
    input->InitializeNonOrientedCursor(inCursor, inTreeIndex);
    output->InitializeNonOrientedCursor(outCursor, inTreeIndex, true);
    outCursor->SetGlobalIndexStart(totalVertices);
    ::CopyInputTreeToOutput(
      inCursor, outCursor, input->GetCellData(), output->GetCellData(), inputMask, outputMask);
    totalVertices += outCursor->GetTree()->GetNumberOfVertices();
  }

  return totalVertices;
}
} // Anonymous namespace

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

  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
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

  vtkSmartPointer<vtkBitArray> outputMask =
    input->HasMask() ? vtkSmartPointer<vtkBitArray>::Take(vtkBitArray::New()) : nullptr;
  vtkIdType totalVertices = ::CopyInputHyperTreeToOutput(input, output, outputMask);
  this->UpdateProgress(0.1);

  vtkHyperTreeGridGhostCellsGeneratorInternals subroutines{ this, controller, input, output,
    outputMask, totalVertices };

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
  subroutines.AppendGhostArray(totalVertices);
  output->SetMask(outputMask);

  this->UpdateProgress(1.);
  return 1;
}

VTK_ABI_NAMESPACE_END
