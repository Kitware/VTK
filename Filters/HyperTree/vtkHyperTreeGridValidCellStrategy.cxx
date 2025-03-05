// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridValidCellStrategy.h"

#include "vtkBitArray.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridValidCellStrategy);

//------------------------------------------------------------------------------
vtkHyperTreeGridValidCellStrategy::vtkHyperTreeGridValidCellStrategy() = default;

//------------------------------------------------------------------------------
vtkHyperTreeGridValidCellStrategy::~vtkHyperTreeGridValidCellStrategy() = default;

//------------------------------------------------------------------------------
void vtkHyperTreeGridValidCellStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "InputGhost size: " << (this->InputGhost ? this->InputGhost->GetNumberOfTuples() : 0)
     << "\n";
  os << indent << "ValidCellsArray size: "
     << (this->ValidCellsArray ? this->ValidCellsArray->GetNumberOfTuples() : 0) << "\n";
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridValidCellStrategy::Initialize(vtkHyperTreeGrid* inputHTG)
{
  this->ValidCellsArray->SetName(this->ArrayName.c_str());
  this->ValidCellsArray->SetNumberOfComponents(1);
  this->ValidCellsArray->SetNumberOfTuples(inputHTG->GetNumberOfCells());
  this->ValidCellsArray->Fill(0);

  this->InputGhost = inputHTG->GetGhostCells();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridValidCellStrategy::Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  if (cursor->IsLeaf())
  {
    const vtkIdType currentId = cursor->GetGlobalNodeIndex();
    if (cursor->IsMasked())
    {
      return;
    }
    if (this->InputGhost != nullptr && this->InputGhost->GetTuple1(currentId) != 0)
    {
      return;
    }
    this->ValidCellsArray->SetTuple1(currentId, 1);
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHyperTreeGridValidCellStrategy::GetAndFinalizeArray()
{
  return this->ValidCellsArray;
}

VTK_ABI_NAMESPACE_END
