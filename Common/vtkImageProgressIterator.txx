/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProgressIterator.txx
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
// Include blockers needed since vtkImageProgressIterator.h includes
// this file when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkImageProgressIterator_txx
#define __vtkImageProgressIterator_txx

#include "vtkImageProgressIterator.h"
#include "vtkImageData.h"
#include "vtkProcessObject.h"

template <class DType>
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

template <class DType>
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

#endif
