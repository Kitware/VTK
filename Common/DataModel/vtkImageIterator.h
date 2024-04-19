// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageIterator
 * @brief   a simple image iterator
 *
 * This is a simple image iterator that can be used to iterate over an
 * image. This should be used internally by Filter writers.
 *
 * @sa
 * vtkImageData vtkImageProgressIterator
 */

#ifndef vtkImageIterator_h
#define vtkImageIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSystemIncludes.h"
VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

template <class DType>
class VTKCOMMONDATAMODEL_EXPORT vtkImageIterator
{
public:
  typedef DType* SpanIterator;

  /**
   * Default empty constructor, useful only when creating an array of iterators
   * You need to call Initialize afterward
   */
  vtkImageIterator();

  /**
   * Create an image iterator for a given image data and a given extent
   */
  vtkImageIterator(vtkImageData* id, int* ext);

  /**
   * Initialize the image iterator for a given image data, and given extent
   */
  void Initialize(vtkImageData* id, int* ext);

  /**
   * Move the iterator to the next span
   */
  void NextSpan();

  /**
   * Return an iterator (pointer) for the span
   */
  SpanIterator BeginSpan() { return this->Pointer; }

  /**
   * Return an iterator (pointer) for the end of the span
   */
  SpanIterator EndSpan() { return this->SpanEndPointer; }

  /**
   * Test if the end of the extent has been reached
   */
  vtkTypeBool IsAtEnd() { return (this->Pointer >= this->EndPointer); }

protected:
  DType* Pointer;
  DType* SpanEndPointer;
  DType* SliceEndPointer;
  DType* EndPointer;
  vtkIdType Increments[3];
  vtkIdType ContinuousIncrements[3];
};

#ifndef vtkImageIterator_cxx
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkImageIterator is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
vtkExternTemplateMacro(extern template class VTKCOMMONDATAMODEL_EXPORT vtkImageIterator);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkImageIterator.h
