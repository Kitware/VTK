/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraymap.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkGraymap.h"


vtkGraymap::vtkGraymap()
{
  this->S = vtkUnsignedCharArray::New();
}

vtkGraymap::vtkGraymap(const vtkGraymap& fs)
{
  this->S = vtkUnsignedCharArray::New();
  *(this->S) = *(fs.S);
}

vtkGraymap::vtkGraymap(const int sz, const int ext)
{
  this->S = vtkUnsignedCharArray::New();
  this->S->Allocate(sz, ext);
}

vtkGraymap::~vtkGraymap()
{
  this->S->Delete();
}



vtkScalars *vtkGraymap::MakeObject(int sze, int ext)
{
  return new vtkGraymap(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkGraymap& vtkGraymap::operator=(const vtkGraymap& fs)
{
  *(this->S) = *(fs.S);
  return *this;
}

// Description:
// Return a rgba color for a particular point id.
// (Note: gray value converted into full rgba.)
unsigned char *vtkGraymap::GetColor(int id)
{
  static unsigned char rgba[4];
  rgba[0] = rgba[1] = rgba[2] = this->S->GetValue(id);
  rgba[3] = 255;
  return rgba;
}

// Description:
// Copy gray components into user provided array for specified
// point id. (Note: gray value converted into full rgba color value.)
void vtkGraymap::GetColor(int id, unsigned char rgba[4])
{
  rgba[0] = rgba[1] = rgba[2] = this->S->GetValue(id);
  rgba[3] = 255;
}

// Description:
// Specify the number of colors for this object to hold. Does an
// allocation as well as setting the MaxId ivar. Used in conjunction with
// SetColor() method for fast insertion.
void vtkGraymap::SetNumberOfColors(int number)
{
  this->S->SetNumberOfValues(number);
}

// Description:
// Insert gray value into object. No range checking performed (fast!).
// (Note: rgba color value converted to grayscale.)
void vtkGraymap::SetColor(int id, unsigned char rgba[4])
{
  float g = 0.30*rgba[0] + 0.59*rgba[1] + 0.11*rgba[2];
  g = (g > 255.0 ? 255.0 : g);

  this->S->SetValue(id, (unsigned char)g);
}

// Description:
// Insert gray value into object. No range checking performed (fast!).
// Make sure you use SetNumberOfColors() to allocate memory prior
// to using SetColor().
void vtkGraymap::SetGrayValue(int id, unsigned char g)
{
  this->S->SetValue(id,g);
}

// Description:
// Insert rgba color value into object. Range checking performed and memory
// allocated as necessary. (Note: rgba converted to gray value using luminance
// equation - see text.)
void vtkGraymap::InsertColor(int id, unsigned char rgba[4])
{
  float g = 0.30*rgba[0] + 0.59*rgba[1] + 0.11*rgba[2];
  g = (g > 255.0 ? 255.0 : g);

  this->S->InsertValue(id,(unsigned char)g);
}

// Description:
// Insert rgba color value into next available slot. Returns point id of slot.
// (Note: rgba converted to gray value.)
int vtkGraymap::InsertNextColor(unsigned char rgba[4])
{
  float g = 0.30*rgba[0] + 0.59*rgba[1] + 0.11*rgba[2];
  g = (g > 255.0 ? 255.0 : g);

  int id = this->S->InsertNextValue((unsigned char)g);
  return id;
}

// Description:
// Return a gray value for a particular point id.
unsigned char vtkGraymap::GetGrayValue(int id)
{
  return this->S->GetValue(id);
}

// Description:
// Insert gray value into object. Range checking performed and memory
// allocated as necessary.
void vtkGraymap::InsertGrayValue(int id, unsigned char g)
{
  this->S->InsertValue(id,g);
}

// Description:
// Insert gray value into next available slot. Returns point id of slot.
int vtkGraymap::InsertNextGrayValue(unsigned char g)
{
  int id = this->S->InsertNextValue(g);
  return id;
}

