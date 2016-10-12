/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowLevelLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWindowLevelLookupTable.h"
#include "vtkObjectFactory.h"

#include <cmath>

vtkStandardNewMacro(vtkWindowLevelLookupTable);

//----------------------------------------------------------------------------
vtkWindowLevelLookupTable::vtkWindowLevelLookupTable(int sze, int ext)
  : vtkLookupTable(sze, ext)
{
  this->Level = (this->TableRange[0] + this->TableRange[1])/2;
  this->Window = (this->TableRange[1] - this->TableRange[0]);

  this->InverseVideo = 0;

  this->MinimumTableValue[0] = 0.0;
  this->MinimumTableValue[1] = 0.0;
  this->MinimumTableValue[2] = 0.0;
  this->MinimumTableValue[3] = 1.0;

  this->MaximumTableValue[0] = 1.0;
  this->MaximumTableValue[1] = 1.0;
  this->MaximumTableValue[2] = 1.0;
  this->MaximumTableValue[3] = 1.0;
}

//----------------------------------------------------------------------------
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
    double start[4], incr[4];

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
          rgba[j] = static_cast<unsigned char> \
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
          rgba[j] = static_cast<unsigned char>(start[j] + i*incr[j] + 0.5);
        }
      }
    }
  }
  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkWindowLevelLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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
}
