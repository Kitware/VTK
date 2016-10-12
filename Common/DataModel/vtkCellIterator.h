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

/**
 * @class   vtkCellIterator
 * @brief   Efficient cell iterator for vtkDataSet topologies.
 *
 *
 * vtkCellIterator provides a method for traversing cells in a data set. Call
 * the vtkDataSet::NewCellIterator() method to use this class.
 *
 * The cell is represented as a set of three pieces of information: The cell
 * type, the ids of the points constituting the cell, and the points themselves.
 * This iterator fetches these as needed. If only the cell type is used,
 * the type is not looked up until GetCellType is called, and the point
 * information is left uninitialized. This allows efficient screening of cells,
 * since expensive point lookups may be skipped depending on the cell type/etc.
 *
 * An example usage of this class:
 * ~~~
 * void myWorkerFunction(vtkDataSet *ds)
 * {
 *   vtkCellIterator *it = ds->NewCellIterator();
 *   for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
 *     {
 *     if (it->GetCellType() != VTK_TETRA)
 *       {
 *       continue; // Skip non-tetrahedral cells
 *       }
 *
 *     vtkIdList *pointIds = it->GetPointIds();
 *     // Do screening on the point ids, maybe figure out scalar range and skip
 *        cells that do not lie in a certain range?
 *
 *     vtkPoints *points = it->GetPoints();
 *     // Do work using the cell points, or ...
 *
 *     vtkGenericCell *cell = ...;
 *     it->GetCell(cell);
 *     // ... do work with a vtkCell.
 *     }
 *   it->Delete();
 * }
 * ~~~
 *
 * The example above pulls in bits of information as needed to filter out cells
 * that aren't relevent. The least expensive lookups are performed first
 * (cell type, then point ids, then points/full cell) to prevent wasted cycles
 * fetching unnecessary data. Also note that at the end of the loop, the
 * iterator must be deleted as these iterators are vtkObject subclasses.
*/

#ifndef vtkCellIterator_h
#define vtkCellIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCellType.h" // For VTK_EMPTY_CELL
#include "vtkObject.h"
#include "vtkNew.h" // For vtkNew
#include "vtkIdList.h" // For inline methods

class vtkGenericCell;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkCellIterator : public vtkObject
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  vtkAbstractTypeMacro(vtkCellIterator, vtkObject)

  /**
   * Reset to the first cell.
   */
  void InitTraversal();

  /**
   * Increment to next cell. Always safe to call.
   */
  void GoToNextCell();

  /**
   * Returns false while the iterator is valid. Always safe to call.
   */
  virtual bool IsDoneWithTraversal() = 0;

  /**
   * Get the current cell type (e.g. VTK_LINE, VTK_VERTEX, VTK_TETRA, etc).
   * This should only be called when IsDoneWithTraversal() returns false.
   */
  int GetCellType();

  /**
   * Get the current cell dimension (0, 1, 2, or 3). This should only be called
   * when IsDoneWithTraversal() returns false.
   */
  int GetCellDimension();

  /**
   * Get the id of the current cell.
   */
  virtual vtkIdType GetCellId() = 0;

  /**
   * Get the ids of the points in the current cell.
   * This should only be called when IsDoneWithTraversal() returns false.
   */
  vtkIdList *GetPointIds();

  /**
   * Get the points in the current cell.
   * This is usually a very expensive call, and should be avoided when possible.
   * This should only be called when IsDoneWithTraversal() returns false.
   */
  vtkPoints *GetPoints();

  /**
   * Get the faces for a polyhedral cell. This is only valid when CellType
   * is VTK_POLYHEDRON.
   */
  vtkIdList *GetFaces();

  /**
   * Write the current full cell information into the argument.
   * This is usually a very expensive call, and should be avoided when possible.
   * This should only be called when IsDoneWithTraversal() returns false.
   */
  void GetCell(vtkGenericCell *cell);

  /**
   * Return the number of points in the current cell.
   * This should only be called when IsDoneWithTraversal() returns false.
   */
  vtkIdType GetNumberOfPoints();

  /**
   * Return the number of faces in the current cell.
   * This should only be called when IsDoneWithTraversal() returns false.
   */
  vtkIdType GetNumberOfFaces();

protected:
  vtkCellIterator();
  ~vtkCellIterator() VTK_OVERRIDE;

  /**
   * Update internal state to point to the first cell.
   */
  virtual void ResetToFirstCell() = 0;

  /**
   * Update internal state to point to the next cell.
   */
  virtual void IncrementToNextCell() = 0;

  /**
   * Lookup the cell type in the data set and store it in this->CellType.
   */
  virtual void FetchCellType() = 0;

  /**
   * Lookup the cell point ids in the data set and store them in this->PointIds.
   */
  virtual void FetchPointIds() = 0;

  /**
   * Lookup the cell points in the data set and store them in this->Points.
   */
  virtual void FetchPoints() = 0;

  /**
   * Lookup the cell faces in the data set and store them in this->Points.
   * Few data sets support faces, so this method has a no-op default
   * implementation. See vtkUnstructuredGrid::GetFaceStream for
   * a description of the layout that Faces should have.
   */
  virtual void FetchFaces() { }

  int CellType;
  vtkPoints *Points;
  vtkIdList *PointIds;
  vtkIdList *Faces;

private:
  vtkCellIterator(const vtkCellIterator &) VTK_DELETE_FUNCTION;
  void operator=(const vtkCellIterator &) VTK_DELETE_FUNCTION;

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
    this->CellType = VTK_EMPTY_CELL;
  }

  void SetCache(unsigned char flags)
  {
    this->CacheFlags |= flags;
  }

  bool CheckCache(unsigned char flags)
  {
    return (this->CacheFlags & flags) == flags;
  }

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
  switch (this->GetCellType())
  {
    case VTK_EMPTY_CELL:
    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
    case VTK_LINE:
    case VTK_POLY_LINE:
    case VTK_TRIANGLE:
    case VTK_TRIANGLE_STRIP:
    case VTK_POLYGON:
    case VTK_PIXEL:
    case VTK_QUAD:
    case VTK_QUADRATIC_EDGE:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUADRATIC_QUAD:
    case VTK_QUADRATIC_POLYGON:
    case VTK_BIQUADRATIC_QUAD:
    case VTK_QUADRATIC_LINEAR_QUAD:
    case VTK_BIQUADRATIC_TRIANGLE:
    case VTK_CUBIC_LINE:
    case VTK_CONVEX_POINT_SET:
    case VTK_PARAMETRIC_CURVE:
    case VTK_PARAMETRIC_SURFACE:
    case VTK_PARAMETRIC_TRI_SURFACE:
    case VTK_PARAMETRIC_QUAD_SURFACE:
    case VTK_HIGHER_ORDER_EDGE:
    case VTK_HIGHER_ORDER_TRIANGLE:
    case VTK_HIGHER_ORDER_QUAD:
    case VTK_HIGHER_ORDER_POLYGON:
      return 0;

    case VTK_TETRA:
    case VTK_QUADRATIC_TETRA:
    case VTK_PARAMETRIC_TETRA_REGION:
    case VTK_HIGHER_ORDER_TETRAHEDRON:
      return 4;

    case VTK_PYRAMID:
    case VTK_QUADRATIC_PYRAMID:
    case VTK_HIGHER_ORDER_PYRAMID:
    case VTK_WEDGE:
    case VTK_QUADRATIC_WEDGE:
    case VTK_QUADRATIC_LINEAR_WEDGE:
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
    case VTK_HIGHER_ORDER_WEDGE:
      return 5;

    case VTK_VOXEL:
    case VTK_HEXAHEDRON:
    case VTK_QUADRATIC_HEXAHEDRON:
    case VTK_TRIQUADRATIC_HEXAHEDRON:
    case VTK_HIGHER_ORDER_HEXAHEDRON:
    case VTK_PARAMETRIC_HEX_REGION:
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      return 6;

    case VTK_PENTAGONAL_PRISM:
      return 7;

    case VTK_HEXAGONAL_PRISM:
      return 8;

    case VTK_POLYHEDRON: // Need to look these up
      if (!this->CheckCache(FacesFlag))
      {
        this->FetchFaces();
        this->SetCache(FacesFlag);
      }
      return this->Faces->GetNumberOfIds() != 0 ? this->Faces->GetId(0) : 0;

    default:
      vtkGenericWarningMacro("Unknown cell type: " << this->CellType);
      break;
  }

  return 0;
}

#endif //vtkCellIterator_h
