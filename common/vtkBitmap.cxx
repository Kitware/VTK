/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBitmap.cxx
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
#include "vtkBitmap.h"

vtkBitmap::vtkBitmap()
{
  this->S = new vtkBitArray;
}

vtkBitmap::vtkBitmap(const vtkBitmap& fs)
{
  this->S = new vtkBitArray;
  *(this->S) = *(fs.S);
}

vtkBitmap::vtkBitmap(const int sz, const int ext=1000)
{
  this->S = new vtkBitArray(sz,ext);
}

vtkBitmap::~vtkBitmap()
{
  this->S->Delete();
}

vtkScalars *vtkBitmap::MakeObject(int sze, int ext)
{
  return new vtkBitmap(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkBitmap& vtkBitmap::operator=(const vtkBitmap& fs)
{
  *(this->S) = *(fs.S);
  return *this;
}

// Description:
// Return a rgba color for a particular point id.
unsigned char *vtkBitmap::GetColor(int id)
{
  static unsigned char rgba[4];
  rgba[0] = rgba[1] = rgba[2] = this->S->GetValue(id);
  rgba[3] = 255;
  return rgba;
}

// Description:
// Get rgba color value for id indicated.
void vtkBitmap::GetColor(int id, unsigned char rgba[4])
{
  rgba[0] = rgba[1] = rgba[2] = this->S->GetValue(id);
  rgba[3] = 255;
}

// Description:
// Insert rgba color value into object. No range checking performed (fast!).
void vtkBitmap::SetColor(int id, unsigned char rgba[4])
{
  unsigned char b=rgba[0]|rgba[1]|rgba[2];
  this->S->SetValue(id,b);
}

// Description:
// Insert rgba color value into object. Range checking performed and memory
// allocated as necessary.
void vtkBitmap::InsertColor(int id, unsigned char rgba[4])
{
  unsigned char b=rgba[0]|rgba[1]|rgba[2];
  this->S->InsertValue(id,b);
}

// Description:
// Insert rgba color value into next available slot. Returns point id of slot.
int vtkBitmap::InsertNextColor(unsigned char rgba[4])
{
  unsigned char b=rgba[0]|rgba[1]|rgba[2];
  int id = this->S->InsertNextValue(b);
  return id;
}

