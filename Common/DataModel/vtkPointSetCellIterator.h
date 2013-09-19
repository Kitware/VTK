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
// .NAME vtkPointSetCellIterator - Implementation of vtkCellIterator using
// vtkPointSet API.

#ifndef __vtkPointSetCellIterator_h
#define __vtkPointSetCellIterator_h

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
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  bool IsDoneWithTraversal();
  vtkIdType GetCellId();

protected:
  vtkPointSetCellIterator();
  ~vtkPointSetCellIterator();

  void ResetToFirstCell();
  void IncrementToNextCell();
  void FetchCellType();
  void FetchPointIds();
  void FetchPoints();

  friend class vtkPointSet;
  void SetPointSet(vtkPointSet *ds);

  vtkSmartPointer<vtkPointSet> PointSet;
  vtkSmartPointer<vtkPoints> PointSetPoints;
  vtkIdType CellId;

private:
  vtkPointSetCellIterator(const vtkPointSetCellIterator &); // Not implemented.
  void operator=(const vtkPointSetCellIterator &);   // Not implemented.
};

#endif //__vtkPointSetCellIterator_h
