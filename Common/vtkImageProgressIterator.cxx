/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProgressIterator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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



