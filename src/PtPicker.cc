/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PtPicker.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PtPicker.hh"
#include "vtkMath.hh"

vtkPointPicker::vtkPointPicker()
{
  this->PointId = -1;
}

void vtkPointPicker::IntersectWithLine(float p1[3], float p2[3], float tol, 
                                      vtkActor *a, vtkMapper *m)
{
  static vtkMath math;
  vtkDataSet *input=m->GetInput();
  int numPts;
  int ptId, i, minPtId;
  float ray[3], rayFactor, tMin, *p, t, projXYZ[3], minXYZ[3];

  if ( (numPts = input->GetNumberOfPoints()) < 1 ) return;
//
//   Determine appropriate info
//
  for (i=0; i<3; i++) ray[i] = p2[i] - p1[i];
  if (( rayFactor = math.Dot(ray,ray)) == 0.0 ) 
    {
    vtkErrorMacro("Cannot process points");
    return;
    }
//
//  Project each point onto ray.  Keep track of the one within the
//  tolerance and closest to the eye (and within the clipping range).
//
  for (minPtId=(-1),tMin=LARGE_FLOAT,ptId=0; ptId<numPts; ptId++) 
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
    this->MarkPicked(a, m, tMin, minXYZ);
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
