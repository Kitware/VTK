/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageIterator - a simple image iterator
// .SECTION Description
// This is a simple image iterator that can be used to iterate over an
// image. This should be used internally by Filter writers.

// .SECTION See also
// vtkImageData vtkImageProgressIterator

#ifndef vtkImageIterator_h
#define vtkImageIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSystemIncludes.h"
class vtkImageData;

template<class DType>
class VTKCOMMONDATAMODEL_EXPORT vtkImageIterator
{
public:
  typedef DType *SpanIterator;

  // Description:
  // Default empty constructor, useful only when creating an array of iterators
  // You need to call Initialize afterward
  vtkImageIterator();

  // Description:
  // Create an image iterator for a given image data and a given extent
  vtkImageIterator(vtkImageData *id, int *ext);

  // Description:
  // Initialize the image iterator for a given image data, and given extent
  void Initialize(vtkImageData *id, int *ext);

  // Description:
  // Move the iterator to the next span
  void NextSpan();

  // Description:
  // Return an iterator (pointer) for the span
  SpanIterator BeginSpan()
    {
    return this->Pointer;
    }

  // Description:
  // Return an iterator (pointer) for the end of the span
  SpanIterator EndSpan()
    {
    return this->SpanEndPointer;
    }

  // Description:
  // Test if the end of the extent has been reached
  int IsAtEnd()
    {
    return (this->Pointer >= this->EndPointer);
    }

protected:
  DType *Pointer;
  DType *SpanEndPointer;
  DType *SliceEndPointer;
  DType *EndPointer;
  vtkIdType    Increments[3];
  vtkIdType    ContinuousIncrements[3];
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkImageIterator.txx"
#endif

#endif
// VTK-HeaderTest-Exclude: vtkImageIterator.h
