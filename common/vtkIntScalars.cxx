/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntScalars.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkIntScalars.h"

vtkIntScalars::vtkIntScalars()
{
  this->S = vtkIntArray::New();
}

vtkIntScalars::vtkIntScalars(const vtkIntScalars& ss)
{
  this->S = vtkIntArray::New();
  *(this->S) = *(ss.S);
}

vtkIntScalars::vtkIntScalars(const int sz, const int ext)
{
  this->S = vtkIntArray::New();
  this->S->Allocate(sz,ext);
}

vtkIntScalars::~vtkIntScalars()
{
  this->S->Delete();
}

vtkScalars *vtkIntScalars::MakeObject(int sze, int ext)
{
  return new vtkIntScalars(sze,ext);
}

// Description:
// Deep copy of scalars.
vtkIntScalars& vtkIntScalars::operator=(const vtkIntScalars& is)
{
  *(this->S) = *(is.S);
  return *this;
}

void vtkIntScalars::GetScalars(vtkIdList& ptId, vtkFloatScalars& fs)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fs.InsertScalar(i,(float)this->S->GetValue(ptId.GetId(i)));
    }
}

void vtkIntScalars::GetScalars(int p1, int p2, vtkFloatScalars& fs)
{
  float *fp=fs.GetPointer(0);
  int *ip=this->S->GetPointer(p1);
  
  for (int id=p1; id <= p2; id++)
    {
    *fp++ = (float)*ip++;
    }
}
