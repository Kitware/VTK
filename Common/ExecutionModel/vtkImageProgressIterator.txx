// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkImageProgressIterator_txx
#define vtkImageProgressIterator_txx

#include "vtkAlgorithm.h"
#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"

VTK_ABI_NAMESPACE_BEGIN
template <class DType>
vtkImageProgressIterator<DType>::vtkImageProgressIterator(
  vtkImageData* imgd, int* ext, vtkAlgorithm* po, int id)
  : vtkImageIterator<DType>(imgd, ext)
{
  this->Target = static_cast<unsigned long>((ext[5] - ext[4] + 1) * (ext[3] - ext[2] + 1) / 50.0);
  this->Target++;
  this->Count = 0;
  this->Count2 = 0;
  this->Algorithm = po;
  this->ID = id;
}

template <class DType>
void vtkImageProgressIterator<DType>::NextSpan()
{
  this->Pointer += this->Increments[1];
  this->SpanEndPointer += this->Increments[1];
  if (this->Pointer >= this->SliceEndPointer)
  {
    this->Pointer += this->ContinuousIncrements[2];
    this->SpanEndPointer += this->ContinuousIncrements[2];
    this->SliceEndPointer += this->Increments[2];
  }
  if (!this->ID)
  {
    if (this->Count2 == this->Target)
    {
      this->Count += this->Count2;
      this->Algorithm->UpdateProgress(this->Count / (50.0 * this->Target));
      this->Count2 = 0;
    }
    this->Count2++;
  }
}

template <class DType>
vtkTypeBool vtkImageProgressIterator<DType>::IsAtEnd()
{
  if (this->Algorithm->GetAbortExecute())
  {
    return 1;
  }
  else
  {
    return this->Superclass::IsAtEnd();
  }
}

VTK_ABI_NAMESPACE_END
#endif
