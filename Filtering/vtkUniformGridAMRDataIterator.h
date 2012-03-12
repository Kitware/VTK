/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGridAMRDataIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUniformGridAMRDataIterator - subclass of vtkCompositeDataIterator
// with API to get current level and dataset index.
// .SECTION Description

#ifndef __vtkUniformGridAMRDataIterator_h
#define __vtkUniformGridAMRDataIterator_h

#include "vtkCompositeDataIterator.h"

class VTK_FILTERING_EXPORT vtkUniformGridAMRDataIterator :
  public vtkCompositeDataIterator
{
public:
  static vtkUniformGridAMRDataIterator* New();
  vtkTypeMacro(vtkUniformGridAMRDataIterator, vtkCompositeDataIterator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the level for the current dataset.
  unsigned int GetCurrentLevel();

  // Description:
  // Returns the dataset index for the current data object. Valid only if the
  // current data is a leaf node i.e. no a composite dataset.
  unsigned int GetCurrentIndex();

//BTX
protected:
  vtkUniformGridAMRDataIterator();
  ~vtkUniformGridAMRDataIterator();

private:
  vtkUniformGridAMRDataIterator(const vtkUniformGridAMRDataIterator&); // Not implemented.
  void operator=(const vtkUniformGridAMRDataIterator&); // Not implemented.
//ETX
};

#endif


