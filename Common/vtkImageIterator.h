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
  
  vtkImageIterator(vtkImageData *id, int *ext)
    {
      m_Ptr = (DType *)id->GetScalarPointerForExtent(ext);
      id->GetIncrements(m_Inc[0], m_Inc[1], m_Inc[2]);
      id->GetContinuousIncrements(ext,m_CInc[0], m_CInc[1], m_CInc[2]);
      m_EndPtr = 
        (DType *)id->GetScalarPointer(ext[1], ext[3], ext[5]) + m_Inc[0];
      // handle y z volumes quickly as well
      if (id->GetExtent()[1] == id->GetExtent()[0])
        {
        m_SpanEndPtr = m_Ptr + m_Inc[1]*(ext[3] - ext[2] + 1);
        m_SliceEndPtr = m_EndPtr;
        m_CInc[0] = m_CInc[1];
        m_CInc[1] = m_CInc[2];
        m_Inc[0] = m_Inc[1];
        m_Inc[1] = m_Inc[2];
        }
      else // normal volumes
        {
        m_SpanEndPtr = m_Ptr + m_Inc[0]*(ext[1] - ext[0] + 1);
        m_SliceEndPtr = m_Ptr + m_Inc[1]*(ext[3] - ext[2] + 1);
        }
    }


  void NextSpan()
    {
      m_Ptr = m_Ptr + m_Inc[1];
      m_SpanEndPtr += m_Inc[1];
      if (m_Ptr >= m_SliceEndPtr)
        {
        m_Ptr = m_Ptr + m_CInc[2];
        m_SliceEndPtr += m_Inc[2];
        }
    }

  SpanIterator BeginSpan()
    {
      return m_Ptr;
    }

  SpanIterator EndSpan()
    {
      return m_SpanEndPtr;
    }
    
  bool IsAtEnd()
    {
    return (m_Ptr >= m_EndPtr);
    }

protected:
  DType *m_Ptr;
  DType *m_SpanEndPtr;
  DType *m_SliceEndPtr;
  DType *m_EndPtr;
  int m_Inc[3];
  int m_CInc[3];
};

#endif
