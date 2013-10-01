/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetCellIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataSetCellIterator.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkIdList.h"

vtkStandardNewMacro(vtkDataSetCellIterator)

//------------------------------------------------------------------------------
void vtkDataSetCellIterator::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataSet: " << this->DataSet.GetPointer() << endl;

}

//------------------------------------------------------------------------------
void vtkDataSetCellIterator::SetDataSet(vtkDataSet *ds)
{
  this->DataSet = ds;
  this->CellId = 0;
}

//------------------------------------------------------------------------------
bool vtkDataSetCellIterator::IsDoneWithTraversal()
{
  return this->DataSet.GetPointer() == NULL
      || this->CellId >= this->DataSet->GetNumberOfCells();
}

//------------------------------------------------------------------------------
vtkIdType vtkDataSetCellIterator::GetCellId()
{
  return this->CellId;
}

//------------------------------------------------------------------------------
void vtkDataSetCellIterator::IncrementToNextCell()
{
  ++this->CellId;
}

//------------------------------------------------------------------------------
vtkDataSetCellIterator::vtkDataSetCellIterator()
  : vtkCellIterator(),
    DataSet(NULL),
    CellId(0)
{
}

//------------------------------------------------------------------------------
vtkDataSetCellIterator::~vtkDataSetCellIterator()
{
}

//------------------------------------------------------------------------------
void vtkDataSetCellIterator::ResetToFirstCell()
{
  this->CellId = 0;
}

//------------------------------------------------------------------------------
void vtkDataSetCellIterator::FetchCellType()
{
  this->CellType = this->DataSet->GetCellType(this->CellId);
}

//------------------------------------------------------------------------------
void vtkDataSetCellIterator::FetchPointIds()
{
  this->DataSet->GetCellPoints(this->CellId, this->PointIds);
}

//------------------------------------------------------------------------------
void vtkDataSetCellIterator::FetchPoints()
{
  // This will fetch the point ids if needed:
  vtkIdList *pointIds = this->GetPointIds();

  vtkIdType numPoints = pointIds->GetNumberOfIds();
  vtkIdType *id = pointIds->GetPointer(0);

  this->Points->SetNumberOfPoints(numPoints);

  double point[3];
  for (int i = 0; i < numPoints; ++i)
    {
    this->DataSet->GetPoint(*id++, point);
    this->Points->SetPoint(i, point);
    }
}
