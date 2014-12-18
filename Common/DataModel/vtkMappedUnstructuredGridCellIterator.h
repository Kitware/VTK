/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMappedUnstructuredGridCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMappedUnstructuredGridCellIterator - Default cell iterator for
// vtkMappedUnstructuredGrid.
//
// .SECTION Description
// This class is used by default for vtkMappedUnstructedGrid instances. It
// uses random access for data lookups. Custom vtkCellIterator implementations
// should be used instead when random-access is inefficient.

#ifndef vtkMappedUnstructuredGridCellIterator_h
#define vtkMappedUnstructuredGridCellIterator_h

#include "vtkCellIterator.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkTypeTemplate.h" // For vtkTypeTemplate

template <class Implementation, class CellIterator>
class vtkMappedUnstructuredGrid;

template <class Implementation>
class vtkMappedUnstructuredGridCellIterator :
    public vtkTypeTemplate<vtkMappedUnstructuredGridCellIterator<Implementation>,
      vtkCellIterator>
{
public:
  typedef Implementation ImplementationType;
  typedef vtkMappedUnstructuredGridCellIterator<ImplementationType> ThisType;
  static vtkMappedUnstructuredGridCellIterator<ImplementationType> *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  void SetMappedUnstructuredGrid(
      vtkMappedUnstructuredGrid<ImplementationType, ThisType> *grid);

  bool IsDoneWithTraversal();
  vtkIdType GetCellId();

protected:
  vtkMappedUnstructuredGridCellIterator();
  ~vtkMappedUnstructuredGridCellIterator();

  void ResetToFirstCell();
  void IncrementToNextCell();
  void FetchCellType();
  void FetchPointIds();
  void FetchPoints();

private:
  vtkMappedUnstructuredGridCellIterator(const vtkMappedUnstructuredGridCellIterator &); // Not implemented.
  void operator=(const vtkMappedUnstructuredGridCellIterator &);   // Not implemented.

  vtkSmartPointer<ImplementationType> Impl;
  vtkSmartPointer<vtkPoints> GridPoints;
  vtkIdType CellId;
  vtkIdType NumberOfCells;
};

#include "vtkMappedUnstructuredGridCellIterator.txx"

#endif //vtkMappedUnstructuredGridCellIterator_h

// VTK-HeaderTest-Exclude: vtkMappedUnstructuredGridCellIterator.h
