/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEllipsoidSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkImageData.h"

#include "vtkImageEllipsoidSource.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageEllipsoidSource* vtkImageEllipsoidSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageEllipsoidSource");
  if(ret)
    {
    return (vtkImageEllipsoidSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageEllipsoidSource;
}




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
void vtkImageEllipsoidSource::PrintSelf(vtkOstream& os, vtkIndent indent)
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
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->WholeExtent[idx] != extent[idx])
      {
      this->WholeExtent[idx] = extent[idx];
      this->Modified();
      }
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
void vtkImageEllipsoidSource::ExecuteInformation()
{
  vtkImageData *data = this->GetOutput();
  unsigned long mem;
  
  data->SetSpacing(1.0, 1.0, 1.0);
  data->SetWholeExtent(this->WholeExtent);
  data->SetNumberOfScalarComponents(1);
  data->SetScalarType(this->OutputScalarType);

  // What if we are trying to process a VERY large 2D image?
  mem = data->GetScalarSize();
  mem = mem * (this->WholeExtent[1] - this->WholeExtent[0] + 1);
  mem = mem * (this->WholeExtent[3] - this->WholeExtent[2] + 1);
  mem = mem / 1000;
  mem = mem * (this->WholeExtent[5] - this->WholeExtent[4] + 1);
  if (mem < 1)
    {
    mem = 1;
    }
  
  //  data->SetEstimatedWholeMemorySize(mem);
}





template <class T>
static void vtkImageEllipsoidSourceExecute(vtkImageEllipsoidSource *self,
					  vtkImageData *data, int ext[6],
					  T *ptr)
{
  int min0, max0;
  int idx0, idx1, idx2;
  int inc0, inc1, inc2;
  double s0, s1, s2, temp;
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
    // handle divide by zero
    if (radius[2] != 0.0)
      {
      temp = ((double)idx2 - center[2]) / radius[2];
      }
    else
      {
      if ((double)idx2 - center[2] == 0.0)
	{
	temp = 0.0;
	}
      else
	{
	temp = VTK_LARGE_FLOAT;
	}  
      }
    
    
    s2 = temp * temp;
    for (idx1 = ext[2]; !self->AbortExecute && idx1 <= ext[3]; ++idx1)
      {
      if (!(count%target))
	{
	self->UpdateProgress(count/(50.0*target));
	}
      count++;
      
      // handle divide by zero
      if (radius[1] != 0.0)
	{
	temp = ((double)idx1 - center[1]) / radius[1];
	}
      else
	{
	if ((double)idx1 - center[1] == 0.0)
	  {
	  temp = 0.0;
	  }
	else
	  {
	  temp = VTK_LARGE_FLOAT;
	  }  
	}
      
      s1 = temp * temp;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	// handle divide by zero
	if (radius[0] != 0.0)
	  {
	  temp = ((double)idx0 - center[0]) / radius[0];
	  }
	else
	  {
	  if ((double)idx0 - center[0] == 0.0)
	    {
	    temp = 0.0;
	    }
	  else
	    {
	    temp = VTK_LARGE_FLOAT;
	    }  
	  }

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
  
  extent = this->GetOutput()->GetUpdateExtent();
  ptr = data->GetScalarPointerForExtent(extent);
  
  switch (data->GetScalarType())
    {
    case VTK_DOUBLE:
      vtkImageEllipsoidSourceExecute(this, data, extent, (double *)ptr);
      break;
    case VTK_FLOAT:
      vtkImageEllipsoidSourceExecute(this, data, extent, (float *)ptr);
      break;
    case VTK_LONG:
      vtkImageEllipsoidSourceExecute(this, data, extent, (long *)ptr);
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageEllipsoidSourceExecute(this, data, extent, (unsigned long *)ptr);
      break;
    case VTK_INT:
      vtkImageEllipsoidSourceExecute(this, data, extent, (int *)ptr);
      break;
    case VTK_UNSIGNED_INT:
      vtkImageEllipsoidSourceExecute(this, data, extent, (unsigned int *)ptr);
      break;
    case VTK_SHORT:
      vtkImageEllipsoidSourceExecute(this, data, extent, (short *)ptr);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageEllipsoidSourceExecute(this, data, extent, (unsigned short *)ptr);
      break;
    case VTK_CHAR:
      vtkImageEllipsoidSourceExecute(this, data, extent, (char *)ptr);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageEllipsoidSourceExecute(this, data, extent, (unsigned char *)ptr);
      break;
    default:
      vtkErrorMacro("Execute: Unknown output ScalarType");
    }
}







