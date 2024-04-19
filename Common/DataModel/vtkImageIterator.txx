// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkImageIterator_txx
#define vtkImageIterator_txx

#include "vtkImageData.h"
#include "vtkImageIterator.h"

//----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class DType>
vtkImageIterator<DType>::vtkImageIterator()
{
  this->Pointer = nullptr;
  this->EndPointer = nullptr;
  this->SpanEndPointer = nullptr;
  this->SliceEndPointer = nullptr;
}

//----------------------------------------------------------------------------
template <class DType>
void vtkImageIterator<DType>::Initialize(vtkImageData* id, int* ext)
{
  this->Pointer = static_cast<DType*>(id->GetScalarPointerForExtent(ext));
  id->GetIncrements(this->Increments[0], this->Increments[1], this->Increments[2]);
  id->GetContinuousIncrements(ext, this->ContinuousIncrements[0], this->ContinuousIncrements[1],
    this->ContinuousIncrements[2]);
  this->EndPointer =
    static_cast<DType*>(id->GetScalarPointer(ext[1], ext[3], ext[5])) + this->Increments[0];

  // if the extent is empty then the end pointer should equal the beg pointer
  if (ext[1] < ext[0] || ext[3] < ext[2] || ext[5] < ext[4])
  {
    this->EndPointer = this->Pointer;
  }

  this->SpanEndPointer = this->Pointer + this->Increments[0] * (ext[1] - ext[0] + 1);
  this->SliceEndPointer = this->Pointer + this->Increments[1] * (ext[3] - ext[2] + 1);
}

//----------------------------------------------------------------------------
template <class DType>
vtkImageIterator<DType>::vtkImageIterator(vtkImageData* id, int* ext)
{
  this->Initialize(id, ext);
}

//----------------------------------------------------------------------------
template <class DType>
void vtkImageIterator<DType>::NextSpan()
{
  this->Pointer += this->Increments[1];
  this->SpanEndPointer += this->Increments[1];
  if (this->Pointer >= this->SliceEndPointer)
  {
    this->Pointer += this->ContinuousIncrements[2];
    this->SpanEndPointer += this->ContinuousIncrements[2];
    this->SliceEndPointer += this->Increments[2];
  }
}

VTK_ABI_NAMESPACE_END
#endif
