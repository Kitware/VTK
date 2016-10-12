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

/**
 * @class   vtkMappedUnstructuredGridCellIterator
 * @brief   Default cell iterator for
 * vtkMappedUnstructuredGrid.
 *
 *
 * This class is used by default for vtkMappedUnstructedGrid instances. It
 * uses random access for data lookups. Custom vtkCellIterator implementations
 * should be used instead when random-access is inefficient.
*/

#ifndef vtkMappedUnstructuredGridCellIterator_h
#define vtkMappedUnstructuredGridCellIterator_h

#include "vtkCellIterator.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

template <class Implementation, class CellIterator>
class vtkMappedUnstructuredGrid;

template <class Implementation>
class vtkMappedUnstructuredGridCellIterator : public vtkCellIterator
{
public:
  vtkTemplateTypeMacro(vtkMappedUnstructuredGridCellIterator<Implementation>,
                       vtkCellIterator)
  typedef Implementation ImplementationType;
  typedef vtkMappedUnstructuredGridCellIterator<ImplementationType> ThisType;
  static vtkMappedUnstructuredGridCellIterator<ImplementationType> *New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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
  vtkMappedUnstructuredGridCellIterator(const vtkMappedUnstructuredGridCellIterator &) VTK_DELETE_FUNCTION;
  void operator=(const vtkMappedUnstructuredGridCellIterator &) VTK_DELETE_FUNCTION;

  vtkSmartPointer<ImplementationType> Impl;
  vtkSmartPointer<vtkPoints> GridPoints;
  vtkIdType CellId;
  vtkIdType NumberOfCells;
};

#include "vtkMappedUnstructuredGridCellIterator.txx"

#endif //vtkMappedUnstructuredGridCellIterator_h

// VTK-HeaderTest-Exclude: vtkMappedUnstructuredGridCellIterator.h
