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



//------------------------------------------------------------------------------
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




// Construct with range=(0,1); and hsv ranges set up for rainbow color table 
// (from red to blue).
vtkWindowLevelLookupTable::vtkWindowLevelLookupTable(int sze, int ext):
vtkLookupTable(sze,ext)
{
  this->NumberOfColors = sze;
  this->Level = sze / 2.0;
  this->Window = sze;
  this->InverseVideo = 0;
  
  this->Table->Allocate(sze,ext);

  this->MinimumColor[0] = this->MinimumColor[1] = this->MinimumColor[2] = 0;
  this->MinimumColor[3] = 255;
  this->MaximumColor[0] = this->MaximumColor[1] = this->MaximumColor[2] = 255;
  this->MaximumColor[3] = 255;
}

// Generate lookup table from window and level.
// Table is built as a linear ramp, centered at Level and of width Window.
void vtkWindowLevelLookupTable::Build()
{
  float rampStartsAt, rampEndsAt;
  float lowValue, highValue, incrValue;
  int indxStartsAt, indxEndsAt;
  int indx;
  int i;
  unsigned char *rgba;
  unsigned char *minimum, *maximum;

  if ( this->Table->GetNumberOfTuples() < 1 ||
  (this->GetMTime() > this->BuildTime && this->InsertTime < this->BuildTime) )
    {
    // determine where ramp starts and ends
    rampStartsAt = this->Level - this->Window / 2.0;
    rampEndsAt = this->Level + this->Window / 2.0;

    vtkDebugMacro (<< "Ramp starts at: " << rampStartsAt << "\n");
    vtkDebugMacro (<< "Ramp ends at: " << rampEndsAt << "\n");

    // calculate table indices from values
    indxStartsAt = this->MapScalarToIndex (rampStartsAt);
    if (indxStartsAt >= this->NumberOfColors)
      {
      indxStartsAt = this->NumberOfColors - 1;
      }
    if (indxStartsAt < 1)
      {
      indxStartsAt = 1;
      }

    indxEndsAt = this->MapScalarToIndex (rampEndsAt);
    if (indxEndsAt >= this->NumberOfColors)
      {
      indxStartsAt = this->NumberOfColors - 1;
      }
    if (indxEndsAt < 1)
      {
      indxEndsAt = 1;
      }

    vtkDebugMacro (<< "Index starts at: " << indxStartsAt << "\n");
    vtkDebugMacro (<< "Index ends at: " << indxEndsAt << "\n");

    if (this->InverseVideo) 
      {
      lowValue = 255;
      highValue = 0;
      minimum = this->MaximumColor;
      maximum = this->MinimumColor;
      }
    else
      {
      lowValue = 0;
      highValue = 255;
      minimum = this->MinimumColor;
      maximum = this->MaximumColor;
      }

    // first do below the ramp
    for (indx=0; indx < indxStartsAt; indx++)
      {
      rgba = this->Table->WritePointer(4*indx,4);
      for (i=0; i < 4; i++)
	{
	rgba[i] = minimum[i];
	}
      }

    // now do the ramp
    incrValue = (float) (highValue - lowValue) / (indxEndsAt - indxStartsAt);
    for (indx=indxStartsAt; indx < indxEndsAt; indx++)
      {
      rgba = this->Table->WritePointer(4*indx,4);
      rgba[0] = rgba[1] = rgba[2] = (unsigned char) (lowValue + incrValue * (indx - indxStartsAt) + .5);
      rgba[3] = 255;
      }

    // finally do above the ramp
    for (indx=indxEndsAt; indx < this->NumberOfColors; indx++)
      {
      rgba = this->Table->WritePointer(4*indx,4);
      for (i=0; i < 4; i++)
	{
	rgba[i] = maximum[i];
	}
      }
  }
  this->BuildTime.Modified();
}

// Convert scalar value to a table index
int vtkWindowLevelLookupTable:: MapScalarToIndex (float scalar)
{
  int indx;

  indx = (int)((scalar-this->TableRange[0])/(this->TableRange[1]-this->TableRange[0]) * this->NumberOfColors);
  indx = (indx < 0 ? 0 : (indx >= this->NumberOfColors ? this->NumberOfColors-1 : indx));

  return indx;
}
  
void vtkWindowLevelLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLookupTable::PrintSelf(os,indent);

  os << indent << "Window: " <<this->Window << "\n";
  os << indent << "Level: " <<this->Level << "\n";
  os << indent << "Inverse Video: " << (this->InverseVideo ? "On\n" : "Off\n");

  os << indent << "Minimum Color : ("
     << (int) this->MinimumColor[0] << ", "
     << (int) this->MinimumColor[1] << ", "
     << (int) this->MinimumColor[2] << ", "
     << (int) this->MinimumColor[3] << ")\n";
  os << indent << "MaximumColor : ("
     << (int) this->MaximumColor[0] << ", "
     << (int) this->MaximumColor[1] << ", "
     << (int) this->MaximumColor[2] << ", "
     << (int) this->MaximumColor[3] << ")\n";
}




