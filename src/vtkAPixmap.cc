/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAPixmap.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkAPixmap.hh"

vtkScalars *vtkAPixmap::MakeObject(int sze, int ext)
{
  return new vtkAPixmap(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkAPixmap& vtkAPixmap::operator=(const vtkAPixmap& fs)
{
  this->S = fs.S;
  return *this;
}

// Description:
// Copy rgba components into user provided array rgba[4] for specified
// point id.
void vtkAPixmap::GetColor(int id, unsigned char rgba[4])
{
  unsigned char *_rgba;

  _rgba = this->S.GetPtr(4*id);
  rgba[0] = _rgba[0];
  rgba[1] = _rgba[1];
  rgba[2] = _rgba[2];
  rgba[3] = _rgba[3];
}

// Description:
// Insert color into object. No range checking performed (fast!).
void vtkAPixmap::SetColor(int id, unsigned char rgba[4])
{
  id *= 4;
  for(int j=0; j<4; j++) this->S[id+j] = rgba[j];
}

// Description:
// Insert color into object. Range checking performed and memory
// allocated as necessary.
void vtkAPixmap::InsertColor(int id, unsigned char rgba[4])
{
  id *= 4;
  for(int j=0; j<4; j++) this->S.InsertValue(id+j,rgba[j]);
}

// Description:
// Insert color into next available slot. Returns point id of slot.
int vtkAPixmap::InsertNextColor(unsigned char rgba[4])
{
  int id = this->S.InsertNextValue(rgba[0]);
  for(int j=1; j<4; j++) this->S.InsertNextValue(rgba[j]);
  return id/4;
}

