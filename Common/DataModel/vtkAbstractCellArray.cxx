// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractCellArray.h"

#include "vtkIdList.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkAbstractCellArray::vtkAbstractCellArray() = default;

//------------------------------------------------------------------------------
vtkAbstractCellArray::~vtkAbstractCellArray() = default;

//------------------------------------------------------------------------------
void vtkAbstractCellArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkAbstractCellArray::GetCellAtId(vtkIdType cellId, vtkIdType& cellSize,
  vtkIdType const*& cellPoints) VTK_SIZEHINT(cellPoints, cellSize)
{
  this->GetCellAtId(cellId, cellSize, cellPoints, this->TempCell);
}

VTK_ABI_NAMESPACE_END
