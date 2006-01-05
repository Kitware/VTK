/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkArrayIterator - Abstract superclass to iterate over elements in 
// an vtkAbstractArray. 
// .SECTION Description
//

#ifndef __vtkArrayIterator_h
#define __vtkArrayIterator_h

#include "vtkObject.h"
class vtkAbstractArray;
class VTK_COMMON_EXPORT vtkArrayIterator : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkArrayIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the array this iterator will iterate over.
  // After Initialize() has been called, the iterator is valid
  // so long as the Array has not been modified 
  // (except using the iterator itself).
  // If the array is modified, the iterator must be re-intialized.
  virtual void Initialize(vtkAbstractArray* array) = 0;
protected:
  vtkArrayIterator();
  ~vtkArrayIterator();

private:
  vtkArrayIterator(const vtkArrayIterator&); // Not implemented.
  void operator=(const vtkArrayIterator&); // Not implemented.
};


#endif

