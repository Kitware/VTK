/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredGridCellIterator
 * @brief   Implementation of vtkCellIterator
 * specialized for vtkUnstructuredGrid.
*/

#ifndef vtkUnstructuredGridCellIterator_h
#define vtkUnstructuredGridCellIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCellIterator.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

class vtkCellArray;
class vtkUnsignedCharArray;
class vtkUnstructuredGrid;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGridCellIterator :
    public vtkCellIterator
{
public:
  static vtkUnstructuredGridCellIterator *New();
  vtkTypeMacro(vtkUnstructuredGridCellIterator, vtkCellIterator)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  bool IsDoneWithTraversal() VTK_OVERRIDE;
  vtkIdType GetCellId() VTK_OVERRIDE;

protected:
  vtkUnstructuredGridCellIterator();
  ~vtkUnstructuredGridCellIterator() VTK_OVERRIDE;

  void ResetToFirstCell() VTK_OVERRIDE;
  void IncrementToNextCell() VTK_OVERRIDE;
  void FetchCellType() VTK_OVERRIDE;
  void FetchPointIds() VTK_OVERRIDE;
  void FetchPoints() VTK_OVERRIDE;
  void FetchFaces() VTK_OVERRIDE;

  friend class vtkUnstructuredGrid;
  void SetUnstructuredGrid(vtkUnstructuredGrid *ug);

  unsigned char *CellTypeBegin;
  unsigned char *CellTypePtr;
  unsigned char *CellTypeEnd;

  vtkIdType *ConnectivityBegin;
  vtkIdType *ConnectivityPtr;
  vtkIdType *FacesBegin;
  vtkIdType *FacesLocsBegin;
  vtkIdType *FacesLocsPtr;

  // Cache misses make updating ConnectivityPtr in IncrementToNextCell too
  // expensive, so we wait to walk through the array until the point ids are
  // needed. This variable keeps track of how far we need to increment.
  vtkIdType SkippedCells;
  void CatchUpSkippedCells();

  vtkSmartPointer<vtkPoints> UnstructuredGridPoints;

private:
  vtkUnstructuredGridCellIterator(const vtkUnstructuredGridCellIterator &) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridCellIterator &) VTK_DELETE_FUNCTION;
};

#endif //vtkUnstructuredGridCellIterator_h
