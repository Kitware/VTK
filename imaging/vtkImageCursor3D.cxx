/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCursor3D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageCursor3D.h"


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
void vtkImageCursor3D::Execute(vtkImageData *vtkNotUsed(inData), 
			       vtkImageData *outData)
{
  void *ptr = NULL;
  
  switch (outData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageCursor3DExecute(this, 
			      outData, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageCursor3DExecute(this, 
			      outData, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageCursor3DExecute(this, 
			      outData, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageCursor3DExecute(this, 
			      outData, (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageCursor3DExecute(this, 
			      outData, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
  
}




