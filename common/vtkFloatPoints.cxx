/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatPoints.cxx
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
#include "vtkFloatPoints.h"


vtkFloatPoints::vtkFloatPoints()
{
  this->P = vtkFloatArray::New();
}

vtkFloatPoints::vtkFloatPoints(const vtkFloatPoints& fp)
{
  this->P = vtkFloatArray::New();
  *(this->P) = *(fp.P);
}

vtkFloatPoints::vtkFloatPoints(const int sz, const int ext)
{
  this->P = new vtkFloatArray(3*sz,3*ext);
}

vtkFloatPoints::~vtkFloatPoints()
{
  this->P->Delete();
}


vtkPoints *vtkFloatPoints::MakeObject(int sze, int ext)
{
  return new vtkFloatPoints(sze,ext);
}

// Description:
// Deep copy of points.
vtkFloatPoints& vtkFloatPoints::operator=(const vtkFloatPoints& fp)
{
  *(this->P) = *(fp.P);
  return *this;
}

void vtkFloatPoints::GetPoints(vtkIdList& ptId, vtkFloatPoints& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertPoint(i,this->P->GetPtr(3*ptId.GetId(i)));
    }
}

