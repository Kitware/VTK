/*=========================================================================

  Program:   Visualization Toolkit
  Module:    %M%
  Language:  C++
  Date:      %D%
  Version:   %V%

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
#include <math.h>
#include "vtkWindowLevelLookupTable.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkWindowLevelLookupTable* vtkWindowLevelLookupTable::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWindowLevelLookupTable");
  if(ret)
    {
    return (vtkWindowLevelLookupTable*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWindowLevelLookupTable;
}

vtkWindowLevelLookupTable::vtkWindowLevelLookupTable(int sze, int ext)
  : vtkLookupTable(sze, ext)
{
  this->Level = (this->TableRange[0] + this->TableRange[1])/2;
  this->Window = (this->TableRange[1] - this->TableRange[0]);

  this->InverseVideo = 0;
  
  this->MinimumTableValue[0] = 0.0f;
  this->MinimumTableValue[1] = 0.0f;
  this->MinimumTableValue[2] = 0.0f;
  this->MinimumTableValue[3] = 1.0f;

  this->MaximumTableValue[0] = 1.0f;
  this->MaximumTableValue[1] = 1.0f;
  this->MaximumTableValue[2] = 1.0f;
  this->MaximumTableValue[3] = 1.0f;

  this->MinimumColor[0] = 0;
  this->MinimumColor[1] = 0;
  this->MinimumColor[2] = 0;
  this->MinimumColor[3] = 255;

  this->MaximumColor[0] = 255;
  this->MaximumColor[1] = 255;
  this->MaximumColor[2] = 255;
  this->MaximumColor[3] = 255;
}

// Table is built as a linear ramp between MinimumTableValue and
// MaximumTableValue.
void vtkWindowLevelLookupTable::Build()
{
  if (this->Table->GetNumberOfTuples() < 1 ||
      (this->GetMTime() > this->BuildTime && 
       this->InsertTime < this->BuildTime))
    {
    int i, j;
    unsigned char *rgba;
    float start[4], incr[4];

    for (j = 0; j < 4; j++)
      {
      start[j] = this->MinimumTableValue[j]*255;
      incr[j] = ((this->MaximumTableValue[j]-this->MinimumTableValue[j]) / 
		 (this->NumberOfColors - 1) * 255);
      }

    if (this->InverseVideo)
      {
      for (i = 0; i < this->NumberOfColors; i++)
	{
        rgba = this->Table->WritePointer(4*i,4);
	for (j = 0; j < 4; j++)
	  {
	  rgba[j] = (unsigned char) \
	    (start[j] + (this->NumberOfColors - i - 1)*incr[j] + 0.5);
	  }
	}
      }
    else
      {
      for (i = 0; i < this->NumberOfColors; i++)
	{
        rgba = this->Table->WritePointer(4*i,4);
	for (j = 0; j < 4; j++)
	  {
	  rgba[j] = (unsigned char)(start[j] + i*incr[j] + 0.5);
	  }
	}
      }
    }
  this->BuildTime.Modified();
}

// Reverse the color table (don't rebuild in case someone has
// been adjusting the table values by hand)
// This is a little ugly ... it might be best to remove
// SetInverseVideo altogether and just use a negative Window.
void vtkWindowLevelLookupTable::SetInverseVideo(int iv)
{
  if (this->InverseVideo == iv)
    {
    return;
    }

  this->InverseVideo = iv;

  if (this->Table->GetNumberOfTuples() < 1)
    {
    return;
    }

  unsigned char *rgba, *rgba2;
  unsigned char tmp[4];
  int i;
  int n = this->NumberOfColors-1;

  for (i = 0; i < this->NumberOfColors/2; i++)
    {
    rgba = this->Table->WritePointer(4*i,4);
    rgba2 = this->Table->WritePointer(4*(n-i),4);
    tmp[0]=rgba[0]; tmp[1]=rgba[1]; tmp[2]=rgba[2]; tmp[3]=rgba[3];
    rgba[0]=rgba2[0]; rgba[1]=rgba2[1]; rgba[2]=rgba2[2]; rgba[3]=rgba2[3]; 
    rgba2[0]=tmp[0]; rgba2[1]=tmp[1]; rgba2[2]=tmp[2]; rgba2[3]=tmp[3]; 
    }
  this->Modified();
}

void vtkWindowLevelLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLookupTable::PrintSelf(os,indent);

  os << indent << "Window: " << this->Window << "\n";
  os << indent << "Level: " << this->Level << "\n";
  os << indent << "InverseVideo: " 
     << (this->InverseVideo ? "On\n" : "Off\n");
  os << indent << "MinimumTableValue : ("
     << this->MinimumTableValue[0] << ", "
     << this->MinimumTableValue[1] << ", "
     << this->MinimumTableValue[2] << ", "
     << this->MinimumTableValue[3] << ")\n";  
  os << indent << "MaximumTableValue : ("
     << this->MaximumTableValue[0] << ", "
     << this->MaximumTableValue[1] << ", "
     << this->MaximumTableValue[2] << ", "
     << this->MaximumTableValue[3] << ")\n";  
  this->GetMinimumColor();
  os << indent << "MinimumColor : ("
     << this->MinimumColor[0] << ", "
     << this->MinimumColor[1] << ", "
     << this->MinimumColor[2] << ", "
     << this->MinimumColor[3] << ")\n";
  this->GetMaximumColor();
  os << indent << "MaximumColor : ("
     << this->MaximumColor[0] << ", "
     << this->MaximumColor[1] << ", "
     << this->MaximumColor[2] << ", "
     << this->MaximumColor[3] << ")\n";
}




