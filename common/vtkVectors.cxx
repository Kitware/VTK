/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectors.cxx
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
#include "vtkVectors.hh"
#include "vtkFloatVectors.hh"
#include "vtkIdList.hh"
#include "vtkMath.hh"

vtkVectors::vtkVectors()
{
  this->MaxNorm = 0.0;
}

void vtkVectors::GetVector(int id, float v[3])
{
  float *vp = this->GetVector(id);
  for (int i=0; i<3; i++) v[i] = vp[i];
}

// Description:
// Insert vector into position indicated.
void vtkVectors::InsertVector(int id, float vx, float vy, float vz)
{
  float v[3];

  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
  this->InsertVector(id,v);
}

// Description:
// Insert vector into position indicated.
int vtkVectors::InsertNextVector(float vx, float vy, float vz)
{
  float v[3];

  v[0] = vx;
  v[1] = vy;
  v[2] = vz;
  return this->InsertNextVector(v);
}

// Description:
// Given a list of pt ids, return an array of vectors.
void vtkVectors::GetVectors(vtkIdList& ptId, vtkFloatVectors& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertVector(i,this->GetVector(ptId.GetId(i)));
    }
}

// Description:
// Compute the largest norm for these vectors.
void vtkVectors::ComputeMaxNorm()
{
  int i;
  float *v, norm;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->MaxNorm = 0.0;
    for (i=0; i<this->GetNumberOfVectors(); i++)
      {
      v = this->GetVector(i);
      norm = vtkMath::Norm(v);
      if ( norm > this->MaxNorm ) this->MaxNorm = norm;
      }

    this->ComputeTime.Modified();
    }
}

// Description:
// Return the maximum norm for these vectors.
float vtkVectors::GetMaxNorm()
{
  this->ComputeMaxNorm();
  return this->MaxNorm;
}

void vtkVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Vectors: " << this->GetNumberOfVectors() << "\n";
  os << indent << "Maximum Euclidean Norm: " << this->GetMaxNorm() << "\n";
}
