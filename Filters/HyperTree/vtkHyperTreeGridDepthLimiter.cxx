// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridDepthLimiter.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridDepthLimiter);

//------------------------------------------------------------------------------
vtkHyperTreeGridDepthLimiter::vtkHyperTreeGridDepthLimiter()
{
  // Require root-level depth by default
  this->Depth = 0;

  // Default mask is empty
  this->OutMask = nullptr;

  // Output indices begin at 0
  this->CurrentId = 0;

  // By default, just create a new mask
  this->JustCreateNewMask = true;

  // The AppropriateOutput attribute is only used when setting JustCreateNewMask.
  // The AppropriateOutput attribute is inherited from the parent class
  // vtkHyperTreeGridAlgorithm. If its value is true, on output an HTG of the
  // same type as the one on input will be constructed.
  // Note that there are two HTG representations: vtkHyperTreeGrid (it manages
  // pad of different sizes on the same level) and vtkUniformHyperTreeGrid (it
  // manages quads/cubes of same size on the same level).
  this->AppropriateOutput = true;
}

//------------------------------------------------------------------------------
vtkHyperTreeGridDepthLimiter::~vtkHyperTreeGridDepthLimiter()
{
  if (this->OutMask)
  {
    this->OutMask->Delete();
    this->OutMask = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridDepthLimiter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Depth: " << this->Depth << endl;
  os << indent << "OutMask: " << this->OutMask << endl;
  os << indent << "CurrentId: " << this->CurrentId << endl;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridDepthLimiter::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridDepthLimiter::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  if (this->JustCreateNewMask)
  {
    output->ShallowCopy(input);
    output->SetDepthLimiter(this->Depth);
    return 1;
  }

  // Retrieve material mask
  this->InMask = input->HasMask() ? input->GetMask() : nullptr;

  // Set grid parameters
  output->SetDimensions(input->GetDimensions());
  output->SetTransposedRootIndexing(input->GetTransposedRootIndexing());
  output->SetBranchFactor(input->GetBranchFactor());
  output->CopyCoordinates(input);
  output->SetHasInterface(input->GetHasInterface());
  output->SetInterfaceNormalsName(input->GetInterfaceNormalsName());
  output->SetInterfaceInterceptsName(input->GetInterfaceInterceptsName());

  // Initialize output point data
  this->InData = input->GetCellData();
  this->OutData = output->GetCellData();
  this->OutData->CopyAllocate(this->InData);

  // Create material mask bit array if one is present on input
  if (!this->OutMask && input->HasMask())
  {
    this->OutMask = vtkBitArray::New();
  }

  // Output indices begin at 0
  this->CurrentId = 0;

  // Iterate over all input and output hyper trees
  vtkIdType inIndex;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> inCursor;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
  while (it.GetNextTree(inIndex))
  {
    if (this->CheckAbort())
    {
      break;
    }
    // Initialize new grid cursor at root of current input tree
    input->InitializeNonOrientedCursor(inCursor, inIndex);

    // Initialize new cursor at root of current output tree
    output->InitializeNonOrientedCursor(outCursor, inIndex, true);

    // Limit depth recursively
    this->RecursivelyProcessTree(inCursor, outCursor);
  }

  // Squeeze and set output material mask if necessary
  if (this->OutMask)
  {
    this->OutMask->Squeeze();
    output->SetMask(this->OutMask);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridDepthLimiter::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedCursor* inCursor, vtkHyperTreeGridNonOrientedCursor* outCursor)
{
  // Retrieve global index of input cursor
  vtkIdType inId = inCursor->GetGlobalNodeIndex();

  // Increase index count on output: postfix is intended
  vtkIdType outId = this->CurrentId++;

  // Retrieve output tree and set global index of output cursor
  vtkHyperTree* outTree = outCursor->GetTree();
  outTree->SetGlobalIndexFromLocal(outCursor->GetVertexId(), outId);

  // Update material mask if relevant
  if (this->InMask)
  {
    this->OutMask->InsertValue(outId, this->InMask->GetValue(inId));
  }

  // Copy output cell data from that of input cell
  this->OutData->CopyData(this->InData, inId, outId);

  // Descend further into input trees only if cursor is not at leaf and depth not reached
  if (!inCursor->IsLeaf() && inCursor->GetLevel() < this->Depth)
  {
    // Cursor is not at leaf, subdivide output tree one level further
    outCursor->SubdivideLeaf();

    // If input cursor is neither at leaf nor at maximum depth, recurse to all children
    int numChildren = inCursor->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      if (this->CheckAbort())
      {
        break;
      }
      inCursor->ToChild(child);
      // Descend into child in output grid as well
      outCursor->ToChild(child);
      // Recurse
      this->RecursivelyProcessTree(inCursor, outCursor);
      // Return to parent in output grid
      inCursor->ToParent();
      outCursor->ToParent();
    }
  }
}
VTK_ABI_NAMESPACE_END
