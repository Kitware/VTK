/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterator.h
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
// .NAME vtkImageIterator - a simple image iterator
// .SECTION Description
// This is a simple image iterator that can be used to iterate over an 
// image. This should be used internally by Filter writers.

// .SECTION See also
// vtkImage vtkImageProgressIterator

#ifndef __vtkImageIterator_h
#define __vtkImageIterator_h

#include "vtkWin32Header.h"
class vtkImageData;

template<typename DType> class VTK_COMMON_EXPORT vtkImageIterator 
{
public:        
  typedef DType *SpanIterator;
  
  // Description:
  // Create an image iterator fora given image data and a given extent
  vtkImageIterator(vtkImageData *id, int *ext);

  // Description:
  // Move the iterator to the next span
  void NextSpan();

  // Description:
  // Return an iterator (pointer) for the span
  SpanIterator BeginSpan()
    {
      return this->Pointer;
    }

  // Description:
  // Return an iterator (pointer) for the end of the span
  SpanIterator EndSpan()
    {
      return this->SpanEndPointer;
    }
    
  // Description:
  // tets if the end of the extent has been reached
  bool IsAtEnd()
    {
    return (this->Pointer >= this->EndPointer);
    }

protected:
  DType *Pointer;
  DType *SpanEndPointer;
  DType *SliceEndPointer;
  DType *EndPointer;
  int    Increments[3];
  int    ContinuousIncrements[3];
};

#ifdef CMAKE_NO_EXPLICIT_TEMPLATE_INSTANTIATION
// include the code
#include "vtkImageIterator.cxx"
#endif

#endif
