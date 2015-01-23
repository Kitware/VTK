/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkCellIterator - Efficient cell iterator for vtkDataSet topologies.
//
// .SECTION Description
// vtkCellIterator provides a method for traversing cells in a data set. Call
// the vtkDataSet::NewCellIterator() method to use this class.
//
// The cell is represented as a set of three pieces of information: The cell
// type, the ids of the points constituting the cell, and the points themselves.
// This iterator fetches these as needed. If only the cell type is used,
// the type is not looked up until GetCellType is called, and the point
// information is left uninitialized. This allows efficient screening of cells,
// since expensive point lookups may be skipped depending on the cell type/etc.
//
// An example usage of this class:
// ~~~
// void myWorkerFunction(vtkDataSet *ds)
// {
//   vtkCellIterator *it = ds->NewCellIterator();
//   for (it->InitTraversal(); it->IsDoneWithTraversal(); it->GoToNextCell())
//     {
//     if (it->GetCellType() != VTK_TETRA)
//       {
//       continue; /* Skip non-tetrahedral cells */
//       }
//
//     vtkIdList *pointIds = it->GetPointIds();
//     /* Do screening on the point ids, maybe figure out scalar range and skip
//        cells that do not lie in a certain range? */
//
//     vtkPoints *points = it->GetPoints();
//     /* Do work using the cell points, or ... */
//
//     vtkGenericCell *cell = ...;
//     it->GetCell(cell);
//     /* ... do work with a vtkCell. */
//     }
//   it->Delete();
// }
// ~~~
//
// The example above pulls in bits of information as needed to filter out cells
// that aren't relevent. The least expensive lookups are performed first
// (cell type, then point ids, then points/full cell) to prevent wasted cycles
// fetching unnecessary data. Also note that at the end of the loop, the
// iterator must be deleted as these iterators are vtkObject subclasses.

#ifndef vtkCellIterator_h
#define vtkCellIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkNew.h" // For vtkNew
#include "vtkIdList.h" // For inline methods

class vtkGenericCell;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkCellIterator : public vtkObject
{
public:
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkAbstractTypeMacro(vtkCellIterator, vtkObject)

  // Description:
  // Reset to the first cell.
  void InitTraversal();

  // Description:
  // Increment to next cell. Always safe to call.
  void GoToNextCell();

  // Description:
  // Returns false while the iterator is valid. Always safe to call.
  virtual bool IsDoneWithTraversal() = 0;

  // Description:
  // Get the current cell type (e.g. VTK_LINE, VTK_VERTEX, VTK_TETRA, etc).
  // This should only be called when IsDoneWithTraversal() returns false.
  int GetCellType();

  // Description:
  // Get the id of the current cell.
  virtual vtkIdType GetCellId() = 0;

  // Description:
  // Get the ids of the points in the current cell.
  // This should only be called when IsDoneWithTraversal() returns false.
  vtkIdList *GetPointIds();

  // Description:
  // Get the points in the current cell.
  // This is usually a very expensive call, and should be avoided when possible.
  // This should only be called when IsDoneWithTraversal() returns false.
  vtkPoints *GetPoints();

  // Description:
  // Get the faces for a polyhedral cell.
  vtkIdList *GetFaces();

  // Description:
  // Write the current full cell information into the argument.
  // This is usually a very expensive call, and should be avoided when possible.
  // This should only be called when IsDoneWithTraversal() returns false.
  void GetCell(vtkGenericCell *cell);

  // Description:
  // Return the number of points in the current cell.
  // This should only be called when IsDoneWithTraversal() returns false.
  vtkIdType GetNumberOfPoints();

  // Description:
  // Return the number of faces in the current polyhedral cell.
  // This should only be called when IsDoneWithTraversal() returns false.
  vtkIdType GetNumberOfFaces();

protected:
  vtkCellIterator();
  ~vtkCellIterator();

  // Description:
  // Update internal state to point to the first cell.
  virtual void ResetToFirstCell() = 0;

  // Description:
  // Update internal state to point to the next cell.
  virtual void IncrementToNextCell() = 0;

  // Description:
  // Lookup the cell type in the data set and store it in this->CellType.
  virtual void FetchCellType() = 0;

  // Description:
  // Lookup the cell point ids in the data set and store them in this->PointIds.
  virtual void FetchPointIds() = 0;

  // Description:
  // Lookup the cell points in the data set and store them in this->Points.
  virtual void FetchPoints() = 0;

  // Description:
  // Lookup the cell faces in the data set and store them in this->Points.
  // Few data sets support faces, so this method has a no-op default
  // implementation. See vtkUnstructuredGrid::GetFaceStream for
  // a description of the layout that Faces should have.
  virtual void FetchFaces() { }

  int CellType;
  vtkPoints *Points;
  vtkIdList *PointIds;
  vtkIdList *Faces;

private:
  vtkCellIterator(const vtkCellIterator &); // Not implemented.
  void operator=(const vtkCellIterator &);   // Not implemented.

  enum
    {
    UninitializedFlag = 0x0,
    CellTypeFlag = 0x1,
    PointIdsFlag = 0x2,
    PointsFlag = 0x4,
    FacesFlag = 0x8
    };

  void ResetCache()
  {
    this->CacheFlags = UninitializedFlag;
    this->ResetContainers();
  }

  void SetCache(unsigned char flags)
  {
    this->CacheFlags |= flags;
  }

  bool CheckCache(unsigned char flags)
  {
    return (this->CacheFlags & flags) == flags;
  }

  void ResetContainers();

  vtkNew<vtkPoints> PointsContainer;
  vtkNew<vtkIdList> PointIdsContainer;
  vtkNew<vtkIdList> FacesContainer;
  unsigned char CacheFlags;
};

//------------------------------------------------------------------------------
inline void vtkCellIterator::InitTraversal()
{
  this->ResetToFirstCell();
  this->ResetCache();
}

//------------------------------------------------------------------------------
inline void vtkCellIterator::GoToNextCell()
{
  this->IncrementToNextCell();
  this->ResetCache();
}

//------------------------------------------------------------------------------
inline int vtkCellIterator::GetCellType()
{
  if (!this->CheckCache(CellTypeFlag))
    {
    this->FetchCellType();
    this->SetCache(CellTypeFlag);
    }
  return this->CellType;
}

//------------------------------------------------------------------------------
inline vtkIdList* vtkCellIterator::GetPointIds()
{
  if (!this->CheckCache(PointIdsFlag))
    {
    this->FetchPointIds();
    this->SetCache(PointIdsFlag);
    }
  return this->PointIds;
}

//------------------------------------------------------------------------------
inline vtkPoints* vtkCellIterator::GetPoints()
{
  if (!this->CheckCache(PointsFlag))
    {
    this->FetchPoints();
    this->SetCache(PointsFlag);
    }
  return this->Points;
}

//------------------------------------------------------------------------------
inline vtkIdList *vtkCellIterator::GetFaces()
{
  if (!this->CheckCache(FacesFlag))
    {
    this->FetchFaces();
    this->SetCache(FacesFlag);
    }
  return this->Faces;
}

//------------------------------------------------------------------------------
inline vtkIdType vtkCellIterator::GetNumberOfPoints()
{
  if (!this->CheckCache(PointIdsFlag))
    {
    this->FetchPointIds();
    this->SetCache(PointIdsFlag);
    }
  return this->PointIds->GetNumberOfIds();
}

//------------------------------------------------------------------------------
inline vtkIdType vtkCellIterator::GetNumberOfFaces()
{
  if (!this->CheckCache(FacesFlag))
    {
    this->FetchFaces();
    this->SetCache(FacesFlag);
    }
  return this->Faces->GetNumberOfIds() != 0 ? this->Faces->GetId(0) : 0;
}

#endif //vtkCellIterator_h
