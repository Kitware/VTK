/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointPicker.cxx
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
#include "vtkPointPicker.h"
#include "vtkMath.h"

vtkPointPicker::vtkPointPicker()
{
  this->PointId = -1;
}

void vtkPointPicker::IntersectWithLine(float p1[3], float p2[3], float tol, 
                                    vtkActor *assem, vtkActor *a, vtkMapper *m)
{
  vtkDataSet *input=m->GetInput();
  int numPts;
  int ptId, i, minPtId;
  float ray[3], rayFactor, tMin, *p, t, projXYZ[3], minXYZ[3];

  if ( (numPts = input->GetNumberOfPoints()) < 1 ) return;
//
//   Determine appropriate info
//
  for (i=0; i<3; i++) ray[i] = p2[i] - p1[i];
  if (( rayFactor = vtkMath::Dot(ray,ray)) == 0.0 ) 
    {
    vtkErrorMacro("Cannot process points");
    return;
    }
//
//  Project each point onto ray.  Keep track of the one within the
//  tolerance and closest to the eye (and within the clipping range).
//
  for (minPtId=(-1),tMin=VTK_LARGE_FLOAT,ptId=0; ptId<numPts; ptId++) 
    {
    p = input->GetPoint(ptId);

    t = (ray[0]*(p[0]-p1[0]) + ray[1]*(p[1]-p1[1]) + ray[2]*(p[2]-p1[2])) 
        / rayFactor;
//
//  If we find a point closer than we currently have, see whether it
//  lies within the pick tolerance and clipping planes.
//
    if ( t >= 0.0 && t <= 1.0 && t < tMin ) 
      {
      for(i=0; i<3; i++) 
        {
        projXYZ[i] = p1[i] + t*ray[i];
        if ( fabs(p[i]-projXYZ[i]) > tol ) break;
        }
      if ( i > 2 ) // within tolerance
        {
        minPtId = ptId;
        minXYZ[0]=p[0]; minXYZ[1]=p[1]; minXYZ[2]=p[2];
        tMin = t;
        }
      }
    }
//
//  Now compare this against other actors.
//
  if ( minPtId>(-1) && tMin < this->GlobalTMin ) 
    {
    this->MarkPicked(assem, a, m, tMin, minXYZ);
    this->PointId = minPtId;
    vtkDebugMacro("Picked point id= " << minPtId);
    }
}

void vtkPointPicker::Initialize()
{
  this->PointId = (-1);
  this->vtkPicker::Initialize();
}

void vtkPointPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkPicker::PrintSelf(os,indent);

  os << indent << "Point Id: " << this->PointId << "\n";
}
