/*=========================================================================

  Program:   Visualization Toolkit
  Module:    %M%
  Language:  C++
  Date:      %D%
  Version:   %V%

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include "vtkWindowLevelLookupTable.hh"

// Description:
// Construct with range=(0,1); and hsv ranges set up for rainbow color table 
// (from red to blue).
vtkWindowLevelLookupTable::vtkWindowLevelLookupTable(int sze, int ext):
vtkLookupTable(sze,ext)
{
  this->NumberOfColors = sze;
  this->Level = sze / 2.0;
  this->Window = sze;
  this->InverseVideo = 0;
  
  this->Table.Allocate(sze,ext);

  this->MinimumColor[0] = this->MinimumColor[1] = this->MinimumColor[2] = 0;
  this->MinimumColor[3] = 255;
  this->MaximumColor[0] = this->MaximumColor[1] = this->MaximumColor[2] = 255;
  this->MaximumColor[3] = 255;

  this->Table.ReferenceCountingOff();
};

// Description:
// Generate lookup table from window and level.
// Table is built as a linear ramp, centered at Level and of width Window.
void vtkWindowLevelLookupTable::Build()
{
  float rampStartsAt, rampEndsAt;
  float lowValue, highValue, incrValue;
  int indxStartsAt, indxEndsAt;
  int indx;
  int i;
  unsigned char rgba[4];
  unsigned char *minimum, *maximum;

  if ( this->Table.GetNumberOfColors() < 1 ||
  (this->GetMTime() > this->BuildTime && this->InsertTime < this->BuildTime) )
    {
    // determine where ramp starts and ends
    rampStartsAt = this->Level - this->Window / 2.0;
    rampEndsAt = this->Level + this->Window / 2.0;

    vtkDebugMacro (<< "Ramp starts at: " << rampStartsAt << "\n");
    vtkDebugMacro (<< "Ramp ends at: " << rampEndsAt << "\n");

    // calculate table indices from values
    indxStartsAt = this->MapScalarToIndex (rampStartsAt);
    if (indxStartsAt >= this->NumberOfColors) indxStartsAt = this->NumberOfColors - 1;
    if (indxStartsAt < 1) indxStartsAt = 1;

    indxEndsAt = this->MapScalarToIndex (rampEndsAt);
    if (indxEndsAt >= this->NumberOfColors) indxStartsAt = this->NumberOfColors - 1;
    if (indxEndsAt < 1) indxEndsAt = 1;

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
    for (i=0; i < 4; i++)
      {
      rgba[i] = minimum[i];
      }
    for (indx=0; indx < indxStartsAt; indx++)
      {
      this->Table.InsertColor(indx,rgba);
      }

    // now do the ramp
    incrValue = (float) (highValue - lowValue) / (indxEndsAt - indxStartsAt);
    for (indx=indxStartsAt; indx < indxEndsAt; indx++)
      {
      rgba[0] = rgba[1] = rgba[2] = (unsigned char) (lowValue + incrValue * (indx - indxStartsAt) + .5);
      rgba[3] = 255;
      this->Table.InsertColor(indx,rgba);
      }

    // finally do above the ramp
    for (i=0; i < 4; i++)
      {
      rgba[i] = maximum[i];
      }
    for (indx=indxEndsAt; indx < this->NumberOfColors; indx++)
      {
      this->Table.InsertColor(indx,rgba);
      }

  }
  this->BuildTime.Modified();
}

// Description::
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




