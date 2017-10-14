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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool IsDoneWithTraversal() override;
  vtkIdType GetCellId() override;

protected:
  vtkPointSetCellIterator();
  ~vtkPointSetCellIterator() override;

  void ResetToFirstCell() override;
  void IncrementToNextCell() override;
  void FetchCellType() override;
  void FetchPointIds() override;
  void FetchPoints() override;

  friend class vtkPointSet;
  void SetPointSet(vtkPointSet *ds);

  vtkSmartPointer<vtkPointSet> PointSet;
  vtkSmartPointer<vtkPoints> PointSetPoints;
  vtkIdType CellId;

private:
  vtkPointSetCellIterator(const vtkPointSetCellIterator &) = delete;
  void operator=(const vtkPointSetCellIterator &) = delete;
};

#endif //vtkPointSetCellIterator_h
