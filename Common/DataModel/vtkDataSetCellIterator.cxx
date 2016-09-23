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

#include "vtkHyperTreeGrid.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"

vtkStandardNewMacro(vtkDataSetCellIterator)

namespace
{
  template<typename T>
  void SetArrayType(T* grid, vtkPoints* points)
  {
    // check all directions to see if any of them are doubles and
    // if they are we set the points data type to double. If the
    // data types are all the same then we set it to the common
    // data type. Otherwise we give up and just keep the default
    // float data type.
    int xType = -1, yType = -1, zType = -1;
    if (vtkDataArray* x = grid->GetXCoordinates())
    {
      xType = x->GetDataType();
      if (xType == VTK_DOUBLE)
      {
        points->SetDataType(VTK_DOUBLE);
        return;
      }
    }
    if (vtkDataArray* y = grid->GetYCoordinates())
    {
      yType = y->GetDataType();
      if (yType == VTK_DOUBLE)
      {
        points->SetDataType(VTK_DOUBLE);
        return;
      }
    }
    if (vtkDataArray* z = grid->GetZCoordinates())
    {
      zType = z->GetDataType();
      if (zType == VTK_DOUBLE)
      {
        points->SetDataType(VTK_DOUBLE);
        return;
      }
    }
    if (xType != -1 || yType != -1 || zType != -1)
    {
      if(xType == yType && xType == zType)
      {
        points->SetDataType(xType);
        return;
      }
      if (xType == -1)
      {
        if (yType == -1)
        {
          points->SetDataType(zType);
          return;
        }
        else if (zType == -1 || yType == zType)
        {
          points->SetDataType(yType);
          return;
        }
      }
      if (yType == -1)
      {
        if (xType == -1)
        {
          points->SetDataType(zType);
          return;
        }
        else if (zType == -1 || xType == zType)
        {
          points->SetDataType(xType);
          return;
        }
      }
      if (zType == -1)
      {
        if (xType == -1)
        {
          points->SetDataType(yType);
          return;
        }
        else if (yType == -1 || xType == yType)
        {
          points->SetDataType(xType);
          return;
        }
      }
    }

    // Set it to the default since it may have gotten set to something else
    points->SetDataType(VTK_FLOAT);
  }
}

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
  cerr << "setting data set\n";
  if (vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(ds))
  {
    SetArrayType(rg, this->Points);
  }
  else if (vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(ds))
  {
    SetArrayType(htg, this->Points);
  }
  else if (ds->IsA("vtkImageData") || ds->IsA("vtkHyperOctree"))
  {
    // ImageData and HyperOctree Origin and Spacing are doubles so
    // the data type for this should also be double
    this->Points->SetDataType(VTK_DOUBLE);
  }
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
