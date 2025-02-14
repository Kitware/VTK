// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridValidCellStrategy.h"

#include "vtkBitArray.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridValidCellStrategy);

//------------------------------------------------------------------------------
void vtkHyperTreeGridValidCellStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputMask size: " << (this->InputMask ? this->InputMask->GetNumberOfTuples() : 0)
     << "\n";
  os << indent
     << "InputGhost size: " << (this->InputGhost ? this->InputGhost->GetNumberOfTuples() : 0)
     << "\n";
  os << indent << "ValidCellsArray size: "
     << (this->ValidCellsArray ? this->ValidCellsArray->GetNumberOfTuples() : 0) << "\n";
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridValidCellStrategy::SetLeafValidity(const vtkIdType& index)
{
  bool validity = true;
  if (this->InputMask != nullptr && this->InputMask->GetTuple1(index) != 0)
  {
    validity = false;
  }
  if (this->InputGhost != nullptr && this->InputGhost->GetTuple1(index) != 0)
  {
    validity = false;
  }
  this->ValidCellsArray->SetTuple1(index, validity ? 1 : 0);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridValidCellStrategy::Initialize(vtkHyperTreeGrid* inputHTG)
{
  this->ValidCellsArray->SetName(this->ArrayName.c_str());
  this->ValidCellsArray->SetNumberOfComponents(1);
  this->ValidCellsArray->SetNumberOfTuples(inputHTG->GetNumberOfCells());
  this->ValidCellsArray->Fill(0);

  this->InputMask = inputHTG->HasMask() ? inputHTG->GetMask() : nullptr;
  this->InputGhost = inputHTG->GetGhostCells();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridValidCellStrategy::Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  if (cursor->IsLeaf())
  {
    vtkIdType currentId = cursor->GetGlobalNodeIndex();
    this->SetLeafValidity(currentId);
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHyperTreeGridValidCellStrategy::GetAndFinalizeArray()
{
  return this->ValidCellsArray;
}

VTK_ABI_NAMESPACE_END
