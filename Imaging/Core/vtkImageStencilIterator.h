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
/**
 * @class   vtkImageStencilIterator
 * @brief   an image region iterator
 *
 * This is an image iterator that can be used to iterate over a
 * region of an image.
 *
 * @sa
 * vtkImageData vtkImageStencilData vtkImageProgressIterator
*/

#ifndef vtkImageStencilIterator_h
#define vtkImageStencilIterator_h

#include "vtkImagePointDataIterator.h"

template<class DType>
class VTKIMAGINGCORE_EXPORT vtkImageStencilIterator :
  public vtkImagePointDataIterator
{
public:
  //@{
  /**
   * Default constructor, its use must be followed by Initialize().
   */
  vtkImageStencilIterator()
  {
    this->Increment = 0;
    this->BasePointer = nullptr;
    this->Pointer = nullptr;
    this->SpanEndPointer = nullptr;
  }
  //@}

  //@{
  /**
   * Create an iterator for the given image, with several options.
   * If a stencil is provided, then the iterator's IsInStencil() method
   * reports whether each span is inside the stencil.  If an extent is
   * provided, it iterates over the extent and ignores the rest of the
   * image (the provided extent must be within the image extent).  If
   * a pointer to the algorithm is provided and threadId is set to zero,
   * then progress events will provided for the algorithm.
   */
  vtkImageStencilIterator(vtkImageData *image,
                          vtkImageStencilData *stencil=nullptr,
                          const int extent[6] = nullptr,
                          vtkAlgorithm *algorithm=nullptr,
                          int threadId=0)
    : vtkImagePointDataIterator(image, extent, stencil, algorithm, threadId)
  {
    this->BasePointer = static_cast<DType *>(
      vtkImagePointDataIterator::GetVoidPointer(image, 0, &this->Increment));
    this->UpdatePointer();
  }
  //@}

  //@{
  /**
   * Initialize an iterator.  See constructor for more details.
   */
  void Initialize(vtkImageData *image,
                  vtkImageStencilData *stencil=nullptr,
                  const int extent[6] = nullptr,
                  vtkAlgorithm *algorithm=nullptr,
                  int threadId=0)
  {
    this->vtkImagePointDataIterator::Initialize(
      image, extent, stencil, algorithm, threadId);
    this->BasePointer = static_cast<DType *>(
      vtkImagePointDataIterator::GetVoidPointer(image, 0, &this->Increment));
    this->UpdatePointer();
  }
  //@}

  //@{
  /**
   * Move the iterator to the beginning of the next span.
   * A span is a contiguous region of the image over which nothing but
   * the point Id and the X index changes.
   */
  void NextSpan()
  {
    this->vtkImagePointDataIterator::NextSpan();
    this->UpdatePointer();
  }
  //@}

  /**
   * Test if the iterator has completed iterating over the entire extent.
   */
  bool IsAtEnd()
  {
    return this->vtkImagePointDataIterator::IsAtEnd();
  }

  /**
   * Return a pointer to the beginning of the current span.
   */
  DType *BeginSpan()
  {
    return this->Pointer;
  }

  /**
   * Return a pointer to the end of the current span.
   */
  DType *EndSpan()
  {
    return this->SpanEndPointer;
  }

protected:

  //@{
  /**
   * Update the pointer (called automatically when a new span begins).
   */
  void UpdatePointer()
  {
    this->Pointer = this->BasePointer + this->Id*this->Increment;
    this->SpanEndPointer = this->BasePointer + this->SpanEnd*this->Increment;
  }
  //@}

  // The pointer must be incremented by this amount for each pixel.
  int Increment;

  // Pointers
  DType *BasePointer;       // pointer to the first voxel
  DType *Pointer;           // current iterator position within data
  DType *SpanEndPointer;    // end of current span
};

#ifndef vtkImageStencilIterator_cxx
#ifdef _MSC_VER
#pragma warning (push)
// The following is needed when the vtkImageStencilIterator template
// class is declared dllexport and is used within vtkImagingCore
#pragma warning (disable: 4910) // extern and dllexport incompatible
#endif
vtkExternTemplateMacro(
  extern template class VTKIMAGINGCORE_EXPORT vtkImageStencilIterator
)
#ifdef _MSC_VER
#pragma warning (pop)
#endif
#endif

#endif
// VTK-HeaderTest-Exclude: vtkImageStencilIterator.h
