/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEllipsoidSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder,ill Lorensen.

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
#include "vtkImageData.h"
#include "vtkImageCache.h"
#include "vtkImageEllipsoidSource.h"

//----------------------------------------------------------------------------
vtkImageEllipsoidSource::vtkImageEllipsoidSource()
{
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = 0;
  this->Center[0] = 128.0;
  this->Center[1] = 128.0;
  this->Center[2] = 0.0;
  this->Radius[0] = 70.0;
  this->Radius[1] = 70.0;
  this->Radius[2] = 70.0;
  this->InValue = 255.0;
  this->OutValue = 0.0;
  
  this->OutputScalarType = VTK_UNSIGNED_CHAR;
}

//----------------------------------------------------------------------------
vtkImageEllipsoidSource::~vtkImageEllipsoidSource()
{
}

//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Center: (" << this->Center[0] << ", "
     << this->Center[1] << ", " << this->Center[2] << ")\n";
  
  os << indent << "Radius: (" << this->Radius[0] << ", "
     << this->Radius[1] << ", " << this->Radius[2] << ")\n";
  
  os << indent << "InValue: " << this->InValue << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  
  vtkImageSource::PrintSelf(os,indent);
}
//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::SetWholeExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->WholeExtent[idx] != extent[idx])
      {
      this->WholeExtent[idx] = extent[idx];
      this->Modified();
      }
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::SetWholeExtent(int minX, int maxX, 
					    int minY, int maxY,
					    int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetWholeExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::GetWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->WholeExtent[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::UpdateImageInformation()
{
  this->CheckCache();
  this->Output->SetSpacing(1.0, 1.0, 1.0);
  this->Output->SetWholeExtent(this->WholeExtent);
  this->Output->SetNumberOfScalarComponents(1);
  this->Output->SetScalarType(this->OutputScalarType);
}





template <class T>
static void vtkImageEllipsoidSourceExecute(vtkImageEllipsoidSource *self,
					  vtkImageData *data, int ext[6],
					  T *ptr)
{
  int min0, max0;
  int idx0, idx1, idx2;
  int inc0, inc1, inc2;
  float s0, s1, s2, temp;
  T outVal, inVal;
  float *center, *radius;
  unsigned long count = 0;
  unsigned long target;

  outVal = (T)(self->GetOutValue());
  inVal = (T)(self->GetInValue());
  center = self->GetCenter();
  radius = self->GetRadius();

  min0 = ext[0];
  max0 = ext[1];
  data->GetContinuousIncrements(ext, inc0, inc1, inc2);

  target = (unsigned long)((ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)/50.0);
  target++;

  for (idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
    {
    temp = ((float)idx2 - center[2]) / radius[2];
    s2 = temp * temp;
    for (idx1 = ext[2]; !self->AbortExecute && idx1 <= ext[3]; ++idx1)
      {
      if (!(count%target))
	{
	self->UpdateProgress(count/(50.0*target));
	}
      count++;
      temp = ((float)idx1 - center[1]) / radius[1];
      s1 = temp * temp;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	temp = ((float)idx0 - center[0]) / radius[0];
	s0 = temp * temp;
	if (s0 + s1 + s2 > 1.0)
	  {
	  *ptr = outVal;
	  }
	else
	  {
	  *ptr = inVal;
	  }
	++ptr;
	// inc0 is 0
	}
      ptr += inc1;
      }
    ptr += inc2;
    }
}

//----------------------------------------------------------------------------
void vtkImageEllipsoidSource::Execute(vtkImageData *data)
{
  int *extent;
  void *ptr;
  
  extent = this->Output->GetUpdateExtent();
  ptr = data->GetScalarPointerForExtent(extent);
  
  switch (data->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageEllipsoidSourceExecute(this, data, extent, (float *)ptr);
      break;
    case VTK_INT:
      vtkImageEllipsoidSourceExecute(this, data, extent, (int *)ptr);
      break;
    case VTK_SHORT:
      vtkImageEllipsoidSourceExecute(this, data, extent, (short *)ptr);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageEllipsoidSourceExecute(this, data, extent, (unsigned short *)ptr);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageEllipsoidSourceExecute(this, data, extent, (unsigned char *)ptr);
      break;
    default:
      vtkErrorMacro("Execute: Unknown output ScalarType");
    }
}







