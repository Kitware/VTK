/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointSetCellIterator
 * @brief   Implementation of vtkCellIterator using
 * vtkPointSet API.
*/

#ifndef vtkPointSetCellIterator_h
#define vtkPointSetCellIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCellIterator.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

class vtkPoints;
class vtkPointSet;

class VTKCOMMONDATAMODEL_EXPORT vtkPointSetCellIterator: public vtkCellIterator
{
public:
  static vtkPointSetCellIterator *New();
  vtkTypeMacro(vtkPointSetCellIterator, vtkCellIterator)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  bool IsDoneWithTraversal() VTK_OVERRIDE;
  vtkIdType GetCellId() VTK_OVERRIDE;

protected:
  vtkPointSetCellIterator();
  ~vtkPointSetCellIterator() VTK_OVERRIDE;

  void ResetToFirstCell() VTK_OVERRIDE;
  void IncrementToNextCell() VTK_OVERRIDE;
  void FetchCellType() VTK_OVERRIDE;
  void FetchPointIds() VTK_OVERRIDE;
  void FetchPoints() VTK_OVERRIDE;

  friend class vtkPointSet;
  void SetPointSet(vtkPointSet *ds);

  vtkSmartPointer<vtkPointSet> PointSet;
  vtkSmartPointer<vtkPoints> PointSetPoints;
  vtkIdType CellId;

private:
  vtkPointSetCellIterator(const vtkPointSetCellIterator &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointSetCellIterator &) VTK_DELETE_FUNCTION;
};

#endif //vtkPointSetCellIterator_h
