// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPointSetCellIterator.h"

#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPointSetCellIterator);

//------------------------------------------------------------------------------
void vtkPointSetCellIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PointSet: " << this->PointSet << endl;
}

//------------------------------------------------------------------------------
void vtkPointSetCellIterator::SetPointSet(vtkPointSet* ds)
{
  this->PointSet = ds;
  this->PointSetPoints = ds ? ds->GetPoints() : nullptr;
  this->CellId = 0;
  if (this->PointSetPoints)
  {
    this->Points->SetDataType(this->PointSetPoints->GetDataType());
  }
}

//------------------------------------------------------------------------------
bool vtkPointSetCellIterator::IsDoneWithTraversal()
{
  return this->PointSet == nullptr || this->CellId >= this->PointSet->GetNumberOfCells();
}

//------------------------------------------------------------------------------
vtkIdType vtkPointSetCellIterator::GetCellId()
{
  return this->CellId;
}

//------------------------------------------------------------------------------
void vtkPointSetCellIterator::IncrementToNextCell()
{
  ++this->CellId;
}

//------------------------------------------------------------------------------
vtkPointSetCellIterator::vtkPointSetCellIterator()
  : PointSet(nullptr)
  , PointSetPoints(nullptr)
  , CellId(0)
{
}

//------------------------------------------------------------------------------
vtkPointSetCellIterator::~vtkPointSetCellIterator() = default;

//------------------------------------------------------------------------------
void vtkPointSetCellIterator::ResetToFirstCell()
{
  this->CellId = 0;
}

//------------------------------------------------------------------------------
void vtkPointSetCellIterator::FetchCellType()
{
  this->CellType = this->PointSet->GetCellType(this->CellId);
}

//------------------------------------------------------------------------------
void vtkPointSetCellIterator::FetchPointIds()
{
  this->PointSet->GetCellPoints(this->CellId, this->PointIds);
}

//------------------------------------------------------------------------------
void vtkPointSetCellIterator::FetchPoints()
{
  vtkIdList* pointIds = this->GetPointIds();
  this->PointSetPoints->GetPoints(pointIds, this->Points);
}
VTK_ABI_NAMESPACE_END
