/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProgressIterator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageProgressIterator - a simple image iterator with progress
// .SECTION Description
// This is a simple image iterator that can be used to iterate over an 
// image. Typically used to iterate over the output image

// .SECTION See also
// vtkImage vtkImageIterator

#ifndef __vtkImageProgressIterator_h
#define __vtkImageProgressIterator_h

#include "vtkImageIterator.h"
class vtkProcessObject;

template<typename DType> class VTK_COMMON_EXPORT vtkImageProgressIterator : 
  public vtkImageIterator<DType> 
{
public:        

  // Description:
  // Create a progress iterator for the provided image data
  // and extent to iterate over. The passes progress object will
  // receive any UpdateProgress calls if the thread id is zero
  vtkImageProgressIterator(vtkImageData *imgd, int *ext, 
                           vtkProcessObject *po, int id);

  // Description:
  // Move the iterator to the next span, may call UpdateProgress on the
  // filter (vtkProcessObject)
  void NextSpan();
  
protected:
  vtkProcessObject *ProcessObject;
  unsigned long     Count;
  unsigned long     Count2;
  unsigned long     Target;
  int               ID;
};

#ifdef CMAKE_NO_EXPLICIT_TEMPLATE_INSTANTIATION
// include the code
#include "vtkImageProgressIterator.txx"
#endif

#endif
