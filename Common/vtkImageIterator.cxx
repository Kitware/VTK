/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterator.cxx
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

#ifndef CMAKE_NO_EXPLICIT_TEMPLATE_INSTATIATION
#include "vtkImageIterator.h"
#include "vtkImageData.h"
#endif

template <typename DType>
vtkImageIterator<DType>::vtkImageIterator(vtkImageData *id, int *ext)
{
  this->Pointer = (DType *)id->GetScalarPointerForExtent(ext);
  id->GetIncrements(this->Increments[0], this->Increments[1], 
                    this->Increments[2]);
  id->GetContinuousIncrements(ext,this->ContinuousIncrements[0], 
                              this->ContinuousIncrements[1], 
                              this->ContinuousIncrements[2]);
  this->EndPointer = 
    (DType *)id->GetScalarPointer(ext[1],ext[3],ext[5]) +this->Increments[0];

  this->SpanEndPointer = 
    this->Pointer + this->Increments[0]*(ext[1] - ext[0] + 1);
  this->SliceEndPointer = 
    this->Pointer + this->Increments[1]*(ext[3] - ext[2] + 1);
}
  
  
template <typename DType>
void vtkImageIterator<DType>::NextSpan()
{
  this->Pointer = this->Pointer + this->Increments[1];
  this->SpanEndPointer += this->Increments[1];
  if (this->Pointer >= this->SliceEndPointer)
    {
    this->Pointer = this->Pointer + this->ContinuousIncrements[2];
    this->SliceEndPointer += this->Increments[2];
    }
}


#ifndef CMAKE_NO_EXPLICIT_TEMPLATE_INSTANTIATION

template class VTK_COMMON_EXPORT vtkImageIterator<char>;
template class VTK_COMMON_EXPORT vtkImageIterator<int>;
template class VTK_COMMON_EXPORT vtkImageIterator<long>;
template class VTK_COMMON_EXPORT vtkImageIterator<short>;
template class VTK_COMMON_EXPORT vtkImageIterator<float>;
template class VTK_COMMON_EXPORT vtkImageIterator<double>;
template class VTK_COMMON_EXPORT vtkImageIterator<unsigned long>;
template class VTK_COMMON_EXPORT vtkImageIterator<unsigned short>;
template class VTK_COMMON_EXPORT vtkImageIterator<unsigned char>;
template class VTK_COMMON_EXPORT vtkImageIterator<unsigned int>;

#endif
