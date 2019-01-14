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
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <cassert>

vtkStandardNewMacro(vtkUnstructuredGridCellIterator)

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // Cast the 'unsigned char*' members to void* to prevent the compiler from
  // interpreting them as strings.
  os << indent << "CellTypeBegin: "
     << static_cast<void*>(this->CellTypeBegin) << endl;
  os << indent << "CellTypePtr: "
     << static_cast<void*>(this->CellTypePtr) << endl;
  os << indent << "CellTypeEnd: "
     << static_cast<void*>(this->CellTypeEnd) << endl;
  os << indent << "ConnectivityBegin: " << this->ConnectivityBegin << endl;
  os << indent << "ConnectivityPtr: " << this->ConnectivityPtr << endl;
  os << indent << "FacesBegin: " << this->FacesBegin<< endl;
  os << indent << "FacesLocsBegin: " << this->FacesLocsBegin << endl;
  os << indent << "FacesLocsPtr: " << this->FacesLocsPtr << endl;
  os << indent << "SkippedCells: " << this->SkippedCells << endl;
  os << indent << "UnstructuredGridPoints: " <<
        this->UnstructuredGridPoints << endl;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::SetUnstructuredGrid(
    vtkUnstructuredGrid *ug)
{
  // If the unstructured grid has not been initialized yet, these may not exist:
  vtkUnsignedCharArray *cellTypeArray = ug ? ug->GetCellTypesArray() : nullptr;
  vtkCellArray *cellArray = ug ? ug->GetCells() : nullptr;
  vtkPoints *points = ug ? ug->GetPoints() : nullptr;

  if(points)
  {
    this->Points->SetDataType(points->GetDataType());
  }

  if (ug && cellTypeArray && cellArray && points)
  {
    // Cell types
    this->CellTypeBegin = this->CellTypeEnd = this->CellTypePtr
        = cellTypeArray ? cellTypeArray->GetPointer(0) : nullptr;
    this->CellTypeEnd += cellTypeArray ? cellTypeArray->GetNumberOfTuples() : 0;

    // CellArray
    this->ConnectivityBegin = this->ConnectivityPtr = cellArray->GetPointer();

    // Point
    this->UnstructuredGridPoints = points;

    // Faces
    vtkIdTypeArray *faces = ug->GetFaces();
    vtkIdTypeArray *facesLocs = ug->GetFaceLocations();
    if (faces && facesLocs)
    {
      this->FacesBegin = faces->GetPointer(0);
      this->FacesLocsBegin = this->FacesLocsPtr = facesLocs->GetPointer(0);
    }
    else
    {
      this->FacesBegin = nullptr;
      this->FacesLocsBegin = nullptr;
      this->FacesLocsPtr = nullptr;
    }
  }
  else
  {
    this->CellTypeBegin = nullptr;
    this->CellTypePtr = nullptr;
    this->CellTypeEnd = nullptr;
    this->FacesBegin = nullptr;
    this->FacesLocsBegin = nullptr;
    this->FacesLocsPtr = nullptr;
    this->ConnectivityBegin= nullptr;
    this->ConnectivityPtr = nullptr;
    this->UnstructuredGridPoints = nullptr;
  }

  this->SkippedCells = 0;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::CatchUpSkippedCells()
{
  // catch up on skipped cells -- cache misses make incrementing Connectivity
  // in IncrementToNextCell() too expensive, so we delay it until here. Special
  // cases are used for 0 or 1 skipped cells to reduce the number of jumps.
  switch (this->SkippedCells)
  {
    default:
      while (this->SkippedCells > 1)
      {
        this->ConnectivityPtr += *this->ConnectivityPtr + 1;
        this->SkippedCells--;
      }
      assert(this->SkippedCells == 1);
      VTK_FALLTHROUGH;
    case 1:
      this->ConnectivityPtr += *this->ConnectivityPtr + 1;
      --this->SkippedCells;
      VTK_FALLTHROUGH;
    case 0:
      // do nothing.
      break;
  }
}

//------------------------------------------------------------------------------
bool vtkUnstructuredGridCellIterator::IsDoneWithTraversal()
{
  return this->CellTypePtr >= this->CellTypeEnd;
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridCellIterator::GetCellId()
{
  return static_cast<vtkIdType>(this->CellTypePtr - this->CellTypeBegin);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::IncrementToNextCell()
{
  ++this->CellTypePtr;

  // Bookkeeping for ConnectivityPtr
  ++this->SkippedCells;

  // Note that we may be incrementing an invalid pointer here...check
  // if FacesLocsBegin is nullptr before dereferencing this!
  ++this->FacesLocsPtr;
}

//------------------------------------------------------------------------------
vtkUnstructuredGridCellIterator::vtkUnstructuredGridCellIterator()
  : vtkCellIterator(),
    CellTypeBegin(nullptr),
    CellTypePtr(nullptr),
    CellTypeEnd(nullptr),
    ConnectivityBegin(nullptr),
    ConnectivityPtr(nullptr),
    FacesBegin(nullptr),
    FacesLocsBegin(nullptr),
    FacesLocsPtr(nullptr),
    SkippedCells(0),
    UnstructuredGridPoints(nullptr)
{
}

//------------------------------------------------------------------------------
vtkUnstructuredGridCellIterator::~vtkUnstructuredGridCellIterator() = default;

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::ResetToFirstCell()
{
  this->CellTypePtr = this->CellTypeBegin;
  this->FacesLocsPtr = this->FacesLocsBegin;
  this->ConnectivityPtr = this->ConnectivityBegin;
  this->SkippedCells = 0;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchCellType()
{
  this->CellType = *this->CellTypePtr;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchPointIds()
{
  CatchUpSkippedCells();
  const vtkIdType *connPtr = this->ConnectivityPtr;
  vtkIdType numCellPoints = *(connPtr++);
  this->PointIds->SetNumberOfIds(numCellPoints);
  vtkIdType *cellPtr = this->PointIds->GetPointer(0);
  std::copy(connPtr, connPtr + numCellPoints, cellPtr);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridCellIterator::FetchPoints()
{
  this->UnstructuredGridPoints->GetPoints(this->GetPointIds(), this->Points);
}

//------------------------------------------------------------------------------
// Given a pointer into a set of faces, traverse the faces and return the total
// number of ids (including size hints) in the face set.
namespace {
inline vtkIdType FaceSetSize(vtkIdType *begin)
{
  vtkIdType *result = begin;
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
  // FacesLocsPtr may be non-null and invalid (this is done to prevent branching
  // in IncrementToNextCell()). Check FacesLocsBegin to determine validity of
  // the pointer.
  if (this->FacesLocsBegin && *this->FacesLocsPtr >= 0)
  {
    vtkIdType *faceSet = this->FacesBegin + *this->FacesLocsPtr;
    vtkIdType facesSize = FaceSetSize(faceSet);
    this->Faces->SetNumberOfIds(facesSize);
    vtkIdType *tmpPtr = this->Faces->GetPointer(0);
    std::copy(faceSet, faceSet + facesSize, tmpPtr);
  }
  else
  {
    this->Faces->SetNumberOfIds(0);
  }
}
