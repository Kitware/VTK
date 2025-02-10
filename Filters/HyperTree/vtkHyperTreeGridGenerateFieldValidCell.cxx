// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridGenerateFieldValidCell.h"

#include "vtkBitArray.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridGenerateFieldValidCell)

  //------------------------------------------------------------------------------
  void vtkHyperTreeGridGenerateFieldValidCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputMask size: " << (this->InputMask ? this->InputMask->GetNumberOfTuples() : 0)
     << "\n";
  os << indent
     << "InputGhost size: " << (this->InputGhost ? this->InputGhost->GetNumberOfTuples() : 0)
     << "\n";
  os << indent << "PackedValidCellArray size: " << this->PackedValidCellArray.size() << "\n";
  os << indent << "ValidCellsImplicitArray size: "
     << (this->ValidCellsImplicitArray ? this->ValidCellsImplicitArray->GetNumberOfTuples() : 0)
     << "\n";
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFieldValidCell::SetLeafValidity(const vtkIdType& index)
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
  this->PackedValidCellArray[index] = validity;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFieldValidCell::Initialize(vtkHyperTreeGrid* inputHTG)
{
  this->PackedValidCellArray.clear();
  this->PackedValidCellArray.resize(inputHTG->GetNumberOfCells(), false);

  this->InputMask = inputHTG->HasMask() ? inputHTG->GetMask() : nullptr;
  this->InputGhost = inputHTG->GetGhostCells();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFieldValidCell::Compute(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  if (cursor->IsLeaf())
  {
    vtkIdType currentId = cursor->GetGlobalNodeIndex();
    this->SetLeafValidity(currentId);
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHyperTreeGridGenerateFieldValidCell::GetAndFinalizeArray()
{
  this->ValidCellsImplicitArray->ConstructBackend(this->PackedValidCellArray);
  this->ValidCellsImplicitArray->SetName(this->ArrayName.c_str());
  this->ValidCellsImplicitArray->SetNumberOfComponents(1);
  this->ValidCellsImplicitArray->SetNumberOfTuples(this->PackedValidCellArray.size());
  for (vtkIdType iCell = 0; iCell < static_cast<vtkIdType>(this->PackedValidCellArray.size());
       ++iCell)
  {
    this->ValidCellsImplicitArray->SetTuple1(iCell, this->PackedValidCellArray[iCell]);
  }
  this->PackedValidCellArray.clear();
  return this->ValidCellsImplicitArray;
}

VTK_ABI_NAMESPACE_END
