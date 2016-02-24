/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageStencilIterator - an image region iterator
// .SECTION Description
// This is an image iterator that can be used to iterate over a
// region of an image.

// .SECTION See also
// vtkImageData vtkImageStencilData vtkImageProgressIterator

#ifndef vtkImageStencilIterator_h
#define vtkImageStencilIterator_h

#include "vtkImagePointDataIterator.h"

template<class DType>
class VTKIMAGINGCORE_EXPORT vtkImageStencilIterator :
  public vtkImagePointDataIterator
{
public:
  // Description:
  // Default constructor, its use must be followed by Initialize().
  vtkImageStencilIterator()
    {
    this->Increment = 0;
    this->BasePointer = 0;
    this->Pointer = 0;
    this->SpanEndPointer = 0;
    }

  // Description:
  // Create an iterator for the given image, with several options.
  // If a stencil is provided, then the iterator's IsInStencil() method
  // reports whether each span is inside the stencil.  If an extent is
  // provided, it iterates over the extent and ignores the rest of the
  // image (the provided extent must be within the image extent).  If
  // a pointer to the algorithm is provided and threadId is set to zero,
  // then progress events will provided for the algorithm.
  vtkImageStencilIterator(vtkImageData *image,
                          vtkImageStencilData *stencil=0,
                          const int extent[6] = 0,
                          vtkAlgorithm *algorithm=0,
                          int threadId=0)
    : vtkImagePointDataIterator(image, extent, stencil, algorithm, threadId)
    {
    this->BasePointer = static_cast<DType *>(
      vtkImagePointDataIterator::GetVoidPointer(image, 0, &this->Increment));
    this->UpdatePointer();
    }

  // Description:
  // Initialize an iterator.  See constructor for more details.
  void Initialize(vtkImageData *image,
                  vtkImageStencilData *stencil=0,
                  const int extent[6] = 0,
                  vtkAlgorithm *algorithm=0,
                  int threadId=0)
    {
    this->vtkImagePointDataIterator::Initialize(
      image, extent, stencil, algorithm, threadId);
    this->BasePointer = static_cast<DType *>(
      vtkImagePointDataIterator::GetVoidPointer(image, 0, &this->Increment));
    this->UpdatePointer();
    }

  // Description:
  // Move the iterator to the beginning of the next span.
  // A span is a contiguous region of the image over which nothing but
  // the point Id and the X index changes.
  void NextSpan()
    {
    this->vtkImagePointDataIterator::NextSpan();
    this->UpdatePointer();
    }

  // Description:
  // Test if the iterator has completed iterating over the entire extent.
  bool IsAtEnd()
    {
    return this->vtkImagePointDataIterator::IsAtEnd();
    }

  // Description:
  // Return a pointer to the beginning of the current span.
  DType *BeginSpan()
    {
    return this->Pointer;
    }

  // Description:
  // Return a pointer to the end of the current span.
  DType *EndSpan()
    {
    return this->SpanEndPointer;
    }

protected:

  // Description:
  // Update the pointer (called automatically when a new span begins).
  void UpdatePointer()
    {
    this->Pointer = this->BasePointer + this->Id*this->Increment;
    this->SpanEndPointer = this->BasePointer + this->SpanEnd*this->Increment;
    }

  // The pointer must be incremented by this amount for each pixel.
  int Increment;

  // Pointers
  DType *BasePointer;       // pointer to the first voxel
  DType *Pointer;           // current iterator position within data
  DType *SpanEndPointer;    // end of current span
};

#endif
// VTK-HeaderTest-Exclude: vtkImageStencilIterator.h
