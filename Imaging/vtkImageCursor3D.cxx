/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCursor3D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include "vtkImageCursor3D.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageCursor3D* vtkImageCursor3D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageCursor3D");
  if(ret)
    {
    return (vtkImageCursor3D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageCursor3D;
}





//----------------------------------------------------------------------------
void vtkImageCursor3D::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  os << indent << "Cursor Radius: " << this->CursorRadius << "\n";
  os << indent << "Cursor Value: " << this->CursorValue << "\n";
  os << indent << "Cursor Position: (" << this->CursorPosition[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->CursorPosition[idx];
    }
  os << ")\n";
  
  vtkImageInPlaceFilter::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkImageCursor3D::vtkImageCursor3D()
{
  this->CursorPosition[0] = 0;
  this->CursorPosition[1] = 0;
  this->CursorPosition[2] = 0;
  
  this->CursorRadius = 5;
  this->CursorValue = 255;
}



template <class T>
static void vtkImageCursor3DExecute(vtkImageCursor3D *self,
				    vtkImageData *outData, T *ptr)
{
  int min0, max0, min1, max1, min2, max2;
  int c0, c1, c2;
  int idx;
  float value = 0.0;
  int rad = self->GetCursorRadius();
  
  c0 = (int)(self->GetCursorPosition()[0]);
  c1 = (int)(self->GetCursorPosition()[1]);
  c2 = (int)(self->GetCursorPosition()[2]);
  value = self->GetCursorValue();
  
  outData->GetExtent(min0, max0, min1, max1, min2, max2);
  
  if (c1 >= min1 && c1 <= max1 && c2 >= min2 && c2 <= max2)
    {
    for (idx = c0 - rad; idx <= c0 + rad; ++idx)
      {
      if (idx >= min0 && idx <= max0)
	{
	ptr = (T *)(outData->GetScalarPointer(idx, c1, c2));
	*ptr = (T)(value);
	}
      }
    }
  
  
  if (c0 >= min0 && c0 <= max0 && c2 >= min2 && c2 <= max2)
    {
    for (idx = c1 - rad; idx <= c1 + rad; ++idx)
      {
      if (idx >= min1 && idx <= max1)
	{
	ptr = (T *)(outData->GetScalarPointer(c0, idx, c2));
	*ptr = (T)(value);
	}
      }
    }
  
  
  if (c0 >= min0 && c0 <= max0 && c1 >= min1 && c1 <= max1)
    {
    for (idx = c2 - rad; idx <= c2 + rad; ++idx)
      {
      if (idx >= min2 && idx <= max2)
	{
	ptr = (T *)(outData->GetScalarPointer(c0, c1, idx));
	*ptr = (T)(value);
	}
      }
    }
}

//----------------------------------------------------------------------------
// Split up into finished and border datas.  Fill the border datas.
void vtkImageCursor3D::ExecuteData(vtkDataObject *out)
{
  void *ptr = NULL;
  
  // let superclass allocate data
  this->vtkImageInPlaceFilter::ExecuteData(out);

  vtkImageData *outData = this->GetOutput();
  
  switch (outData->GetScalarType())
    {
    vtkTemplateMacro3(vtkImageCursor3DExecute, this, 
                      outData, (VTK_TT *)(ptr));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
  
}




