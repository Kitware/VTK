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
// .NAME vtkUnstructuredGridCellIterator - Implementation of vtkCellIterator
// specialized for vtkUnstructuredGrid.

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
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  bool IsDoneWithTraversal();
  vtkIdType GetCellId();

protected:
  vtkUnstructuredGridCellIterator();
  ~vtkUnstructuredGridCellIterator();

  void ResetToFirstCell();
  void IncrementToNextCell();
  void FetchCellType();
  void FetchPointIds();
  void FetchPoints();
  void FetchFaces();

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
  vtkUnstructuredGridCellIterator(const vtkUnstructuredGridCellIterator &); // Not implemented.
  void operator=(const vtkUnstructuredGridCellIterator &);   // Not implemented.
};

#endif //vtkUnstructuredGridCellIterator_h
