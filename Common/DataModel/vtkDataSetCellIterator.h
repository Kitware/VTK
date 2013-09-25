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

#ifndef __vtkDataSetCellIterator_h
#define __vtkDataSetCellIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCellIterator.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkDataSetCellIterator: public vtkCellIterator
{
public:
  static vtkDataSetCellIterator *New();
  vtkTypeMacro(vtkDataSetCellIterator, vtkCellIterator)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

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
  vtkDataSetCellIterator(const vtkDataSetCellIterator &); // Not implemented.
  void operator=(const vtkDataSetCellIterator &);   // Not implemented.
};

#endif //__vtkDataSetCellIterator_h
