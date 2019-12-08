/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridCellIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkUnstructuredGridCellIterator.h"

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <cassert>

vtkStandardNewMacro(vtkUnstructuredGridCellIterator);

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cells:\n";
  this->Cells->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Types:\n";
  this->Types->PrintSelf(os, indent.GetNextIndent());

  os << indent << "FaceConn:\n";
  this->FaceConn->PrintSelf(os, indent.GetNextIndent());

  os << indent << "FaceLocs:\n";
  this->FaceLocs->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Coords:\n";
  this->Coords->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::SetUnstructuredGrid(vtkUnstructuredGrid* ug)
{
  // If the unstructured grid has not been initialized yet, these may not exist:
  vtkUnsignedCharArray* cellTypeArray = ug ? ug->GetCellTypesArray() : nullptr;
  vtkCellArray* cellArray = ug ? ug->GetCells() : nullptr;
  vtkPoints* points = ug ? ug->GetPoints() : nullptr;

  if (points)
  {
    this->Points->SetDataType(points->GetDataType());
  }

  if (ug && cellTypeArray && cellArray && points)
  {
    this->Cells = vtk::TakeSmartPointer(cellArray->NewIterator());
    this->Cells->GoToFirstCell();

    this->Types = cellTypeArray;
    this->FaceConn = ug->GetFaces();
    this->FaceLocs = ug->GetFaceLocations();
    this->Coords = points;
  }
}

//------------------------------------------------------------------------------
bool vtkUnstructuredGridCellIterator::IsDoneWithTraversal()
{
  return this->Cells ? this->Cells->IsDoneWithTraversal() : true;
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridCellIterator::GetCellId()
{
  return this->Cells->GetCurrentCellId();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::IncrementToNextCell()
{
  this->Cells->GoToNextCell();
}

//------------------------------------------------------------------------------
vtkUnstructuredGridCellIterator::vtkUnstructuredGridCellIterator() {}

//------------------------------------------------------------------------------
vtkUnstructuredGridCellIterator::~vtkUnstructuredGridCellIterator() = default;

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::ResetToFirstCell()
{
  if (this->Cells)
  {
    this->Cells->GoToFirstCell();
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchCellType()
{
  const vtkIdType cellId = this->Cells->GetCurrentCellId();
  this->CellType = this->Types->GetValue(cellId);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchPointIds()
{
  this->Cells->GetCurrentCell(this->PointIds);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchPoints()
{
  this->Coords->GetPoints(this->GetPointIds(), this->Points);
}

//------------------------------------------------------------------------------
// Given a pointer into a set of faces, traverse the faces and return the total
// number of ids (including size hints) in the face set.
namespace
{
inline vtkIdType FaceSetSize(const vtkIdType* begin)
{
  const vtkIdType* result = begin;
  vtkIdType numFaces = *(result++);
  while (numFaces-- > 0)
  {
    result += *result + 1;
  }
  return result - begin;
}
} // end anon namespace

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchFaces()
{
  if (this->FaceLocs)
  {
    const vtkIdType cellId = this->Cells->GetCurrentCellId();
    const vtkIdType faceLoc = this->FaceLocs->GetValue(cellId);
    const vtkIdType* faceSet = this->FaceConn->GetPointer(faceLoc);
    vtkIdType facesSize = FaceSetSize(faceSet);
    this->Faces->SetNumberOfIds(facesSize);
    vtkIdType* tmpPtr = this->Faces->GetPointer(0);
    std::copy_n(faceSet, facesSize, tmpPtr);
  }
  else
  {
    this->Faces->SetNumberOfIds(0);
  }
}
