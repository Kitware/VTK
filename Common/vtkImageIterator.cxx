/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterator.cxx
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

  // handle y z volumes quickly as well
  if (id->GetExtent()[1] == id->GetExtent()[0])
    {
    this->SpanEndPointer = this->Pointer + 
      this->Increments[1]*(ext[3] - ext[2] + 1);
    this->SliceEndPointer = this->EndPointer;
    this->ContinuousIncrements[0] = this->ContinuousIncrements[1];
    this->ContinuousIncrements[1] = this->ContinuousIncrements[2];
    this->Increments[0] = this->Increments[1];
    this->Increments[1] = this->Increments[2];
    }
  else // normal volumes
    {
    this->SpanEndPointer = 
      this->Pointer + this->Increments[0]*(ext[1] - ext[0] + 1);
    this->SliceEndPointer = 
      this->Pointer + this->Increments[1]*(ext[3] - ext[2] + 1);
    }
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
