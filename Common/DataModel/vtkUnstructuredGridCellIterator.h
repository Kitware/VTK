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

#include "vtkCellIterator.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer

class vtkCellArray;
class vtkCellArrayIterator;
class vtkIdTypeArray;
class vtkUnsignedCharArray;
class vtkUnstructuredGrid;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGridCellIterator : public vtkCellIterator
{
public:
  static vtkUnstructuredGridCellIterator* New();
  vtkTypeMacro(vtkUnstructuredGridCellIterator, vtkCellIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool IsDoneWithTraversal() override;
  vtkIdType GetCellId() override;

protected:
  vtkUnstructuredGridCellIterator();
  ~vtkUnstructuredGridCellIterator() override;

  void ResetToFirstCell() override;
  void IncrementToNextCell() override;
  void FetchCellType() override;
  void FetchPointIds() override;
  void FetchPoints() override;
  void FetchFaces() override;

  friend class vtkUnstructuredGrid;
  void SetUnstructuredGrid(vtkUnstructuredGrid* ug);

  vtkSmartPointer<vtkCellArrayIterator> Cells;
  vtkSmartPointer<vtkUnsignedCharArray> Types;
  vtkSmartPointer<vtkIdTypeArray> FaceConn;
  vtkSmartPointer<vtkIdTypeArray> FaceLocs;
  vtkSmartPointer<vtkPoints> Coords;

private:
  vtkUnstructuredGridCellIterator(const vtkUnstructuredGridCellIterator&) = delete;
  void operator=(const vtkUnstructuredGridCellIterator&) = delete;
};

#endif // vtkUnstructuredGridCellIterator_h
