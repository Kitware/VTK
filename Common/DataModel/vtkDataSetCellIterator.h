/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetCellIterator - Implementation of vtkCellIterator using
// vtkDataSet API.

#ifndef vtkDataSetCellIterator_h
#define vtkDataSetCellIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCellIterator.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkDataSetCellIterator: public vtkCellIterator
{
public:
  static vtkDataSetCellIterator *New();
  vtkTypeMacro(vtkDataSetCellIterator, vtkCellIterator)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  bool IsDoneWithTraversal();
  vtkIdType GetCellId();

protected:
  vtkDataSetCellIterator();
  ~vtkDataSetCellIterator();

  void ResetToFirstCell();
  void IncrementToNextCell();
  void FetchCellType();
  void FetchPointIds();
  void FetchPoints();

  friend class vtkDataSet;
  void SetDataSet(vtkDataSet *ds);

  vtkSmartPointer<vtkDataSet> DataSet;
  vtkIdType CellId;

private:
  vtkDataSetCellIterator(const vtkDataSetCellIterator &) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataSetCellIterator &) VTK_DELETE_FUNCTION;
};

#endif //vtkDataSetCellIterator_h
