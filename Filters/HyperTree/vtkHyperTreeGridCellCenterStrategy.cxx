// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridCellCenterStrategy.h"

#include "vtkBitArray.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridCellCenterStrategy);

//------------------------------------------------------------------------------
vtkHyperTreeGridCellCenterStrategy::vtkHyperTreeGridCellCenterStrategy() = default;

//------------------------------------------------------------------------------
vtkHyperTreeGridCellCenterStrategy::~vtkHyperTreeGridCellCenterStrategy() = default;

//------------------------------------------------------------------------------
void vtkHyperTreeGridCellCenterStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputMask size: " << (this->InputMask ? this->InputMask->GetNumberOfTuples() : 0)
     << "\n";
  os << indent
     << "InputGhost size: " << (this->InputGhost ? this->InputGhost->GetNumberOfTuples() : 0)
     << "\n";
  os << indent << "CellCentersArray size: "
     << (this->CellCentersArray ? this->CellCentersArray->GetNumberOfTuples() : 0) << "\n";
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridCellCenterStrategy::Initialize(vtkHyperTreeGrid* inputHTG)
{
  this->CellCentersArray->SetName(this->ArrayName.c_str());
  this->CellCentersArray->SetNumberOfComponents(3);
  this->CellCentersArray->SetNumberOfTuples(inputHTG->GetNumberOfCells());

  this->InputMask = inputHTG->HasMask() ? inputHTG->GetMask() : nullptr;
  this->InputGhost = inputHTG->GetGhostCells();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridCellCenterStrategy::Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  if (cursor->IsLeaf())
  {
    vtkIdType currentId = cursor->GetGlobalNodeIndex();
    bool validity = true;
    if (this->InputMask != nullptr && this->InputMask->GetTuple1(currentId) != 0)
    {
      validity = false;
    }
    if (this->InputGhost != nullptr && this->InputGhost->GetTuple1(currentId) != 0)
    {
      validity = false;
    }

    if (validity)
    {
      double pt[3];
      cursor->GetPoint(pt);

      this->CellCentersArray->SetTuple3(currentId, pt[0], pt[1], pt[2]);
    }
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHyperTreeGridCellCenterStrategy::GetAndFinalizeArray()
{
  return this->CellCentersArray;
}

VTK_ABI_NAMESPACE_END
