/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAGraymap.cc
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
#include "vtkAGraymap.hh"

vtkScalars *vtkAGraymap::MakeObject(int sze, int ext)
{
  return new vtkAGraymap(sze,ext);
}

float vtkAGraymap::GetScalar(int i)
{
  return (float)(this->S[2*i]);
}

// Description:
// Return an unsigned char rgba color value for a particular point id.
unsigned char *vtkAGraymap::GetColor(int id)
{
  static unsigned char rgba[4];
  rgba[0] = rgba[1] = rgba[2] = this->S[2*id];
  rgba[3] = this->S[2*id+1];
  
  return rgba;
}

// Description:
// Copy rgba color value components into user provided array for specified
// point id.
void vtkAGraymap::GetColor(int id, unsigned char rgba[4])
{
  rgba[0] = rgba[1] = rgba[2] = this->S[2*id];
  rgba[3] = this->S[2*id+1];
}

// Description:
// Deep copy of scalars.
vtkAGraymap& vtkAGraymap::operator=(const vtkAGraymap& fs)
{
  this->S = fs.S;
  return *this;
}

// Description:
// Return a unsigned char gray-alpha value for a particular point id.
unsigned char *vtkAGraymap::GetAGrayValue(int id)
{
  static unsigned char ga[2];
  ga[0] = this->S[2*id];
  ga[1] = this->S[2*id+1];
  
  return ga;
}

// Description:
// Copy gray-alpha components into user provided array for specified
// point id.
void vtkAGraymap::GetAGrayValue(int id, unsigned char ga[2])
{
  ga[0] = this->S[2*id];
  ga[1] = this->S[2*id+1];
}

// Description:
// Set a gray-alpha value at a particular array location. Does not do 
// range checking.
void vtkAGraymap::SetAGrayValue(int i, unsigned char ga[2]) 
{
  i *= 2; 
  this->S[i] = ga[0];
  this->S[i+1] = ga[1]; 
}

// Description:
// Insert a gray-alpha value at a particular array location. Does range 
// checking and will allocate additional memory if necessary.
void vtkAGraymap::InsertAGrayValue(int i, unsigned char ga[2]) 
{
  this->S.InsertValue(2*i+1, ga[1]);
  this->S[2*i] = ga[0];
}

// Description:
// Insert a gray-alpha value at the next available slot in the array. Will
// allocate memory if necessary.
int vtkAGraymap::InsertNextAGrayValue(unsigned char ga[2]) 
{
  int id;

  id = this->S.InsertNextValue(ga[0]);
  this->S.InsertNextValue(ga[1]);

  return id/2;
}
