/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProgressIterator.cxx
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
#include "vtkImageProgressIterator.h"
#include "vtkImageData.h"
#include "vtkProcessObject.h"
#endif

template <typename DType>
vtkImageProgressIterator<DType>::vtkImageProgressIterator(vtkImageData *imgd, 
                                                       int *ext, 
                                                       vtkProcessObject *po, 
                                                       int id) : 
  vtkImageIterator<DType>(imgd,ext)
{
  this->Target = 
    (unsigned long)((ext[5] - ext[4]+1)*(ext[3] - ext[2]+1)/50.0);
  this->Target++;
  this->Count = 0;
  this->Count2 = 0;
  this->ProcessObject = po;
  this->ID = id;
}

template <typename DType>
void vtkImageProgressIterator<DType>::NextSpan()
{
  this->Pointer = this->Pointer + this->Increments[1];
  this->SpanEndPointer += this->Increments[1];
  if (this->Pointer >= this->SliceEndPointer)
    {
    this->Pointer = this->Pointer + this->ContinuousIncrements[2];
    this->SliceEndPointer += this->Increments[2];
    }
  if (this->ID)
    {
    if (this->Count2 == this->Target)
      {
      this->Count += this->Count2;
      this->ProcessObject->UpdateProgress(this->Count/(50.0*this->Target));
      this->Count2 = 0;
      }
    this->Count2++;
    }
}


#ifndef CMAKE_NO_EXPLICIT_TEMPLATE_INSTANTIATION

template class VTK_COMMON_EXPORT vtkImageProgressIterator<char>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<int>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<long>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<short>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<float>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<double>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<unsigned long>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<unsigned short>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<unsigned char>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<unsigned int>;

#endif



