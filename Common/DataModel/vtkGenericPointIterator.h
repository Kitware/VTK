/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericPointIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericPointIterator - iterator used to traverse points
// .SECTION Description
// This class (and subclasses) are used to iterate over points. Use it
// only in conjunction with vtkGenericDataSet (i.e., the adaptor framework).
//
// Typical use is:
// <pre>
// vtkGenericDataSet *dataset;
// vtkGenericPointIterator *it = dataset->NewPointIterator();
// for (it->Begin(); !it->IsAtEnd(); it->Next());
//   {
//   x=it->GetPosition();
//   }
// </pre>


#ifndef vtkGenericPointIterator_h
#define vtkGenericPointIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONDATAMODEL_EXPORT vtkGenericPointIterator : public vtkObject
{
public:
  // Description:
  // Standard VTK construction and type macros.
  vtkTypeMacro(vtkGenericPointIterator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move iterator to first position if any (loop initialization).
  virtual void Begin() = 0;

  // Description:
  // Is the iterator at the end of traversal?
  virtual int IsAtEnd() = 0;

  // Description:
  // Move the iterator to the next position in the list.
  // \pre not_off: !IsAtEnd()
  virtual void Next() = 0;

  // Description:
  // Get the coordinates of the point at the current iterator position.
  // \pre not_off: !IsAtEnd()
  // \post result_exists: result!=0
  virtual double *GetPosition() = 0;

  // Description:
  // Get the coordinates of the point at the current iterator position.
  // \pre not_off: !IsAtEnd()
  // \pre x_exists: x!=0
  virtual void GetPosition(double x[3]) = 0;

  // Description:
  // Return the unique identifier for the point, could be non-contiguous.
  // \pre not_off: !IsAtEnd()
  virtual vtkIdType GetId() = 0;

protected:
  // Description:
  // Destructor.
  vtkGenericPointIterator();
  virtual ~vtkGenericPointIterator();

private:
  vtkGenericPointIterator(const vtkGenericPointIterator&);  // Not implemented.
  void operator=(const vtkGenericPointIterator&);  // Not implemented.
};

#endif
