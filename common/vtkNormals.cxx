/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNormals.cxx
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
#include "vtkNormals.h"
#include "vtkIdList.h"
#include "vtkFloatNormals.h"

void vtkNormals::GetNormal(int id, float n[3])
{
  float *np = this->GetNormal(id);
  for (int i=0; i<3; i++) n[i] = np[i];
}

// Description:
// Insert normal into position indicated.
void vtkNormals::InsertNormal(int id, float nx, float ny, float nz)
{
  float n[3];

  n[0] = nx;
  n[1] = ny;
  n[2] = nz;
  this->InsertNormal(id,n);
}

// Description:
// Insert normal into position indicated.
int vtkNormals::InsertNextNormal(float nx, float ny, float nz)
{
  float n[3];

  n[0] = nx;
  n[1] = ny;
  n[2] = nz;
  return this->InsertNextNormal(n);
}

// Description:
// Given a list of pt ids, return an array of corresponding normals.
void vtkNormals::GetNormals(vtkIdList& ptId, vtkFloatNormals& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertNormal(i,this->GetNormal(ptId.GetId(i)));
    }
}

void vtkNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkReferenceCount::PrintSelf(os,indent);

  os << indent << "Number Of Normals: " << this->GetNumberOfNormals() << "\n";
}
