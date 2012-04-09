/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoundingBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMath.h"
#include "vtkBoundingBox.h"
#include "vtkSmartPointer.h"

#include <limits>

#define TestBoundingBoxFailMacro(b,msg) if(!(b)){std::cerr <<msg<<std::endl;return EXIT_FAILURE;}

int TestBoundingBox(int,char *[])
{
    {
    double n[3]={-1,0.5,0};
    double p[3]={-1,-1,-1};
    double bb[6] = {-1,1,-1,1,-1,1};
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p,n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro(res && bb[0]==-1 && bb[1]==0,"Intersect Plane Failed!")
    }
    {
    double n[3]={0,0,1};
    double p[3]={0,0,0};
    double bb[6] = {-1,1,-1,1,-1,1};
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p,n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro(res && bb[4]==0 && bb[5]==1,"Intersect Plane Failed!")
    }
    {
    double n[3]={0,0,-1};
    double p[3]={0,0,0};
    double bb[6] = {-1,1,-1,1,-1,1};
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p,n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro(res && bb[4]==-1 && bb[5]==0,"Intersect Plane Failed!")
    }
    {
    double n[3]={0,-1,0};
    double p[3]={0,0,0};
    double bb[6] = {-1,1,-1,1,-1,1};
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p,n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro(res && bb[2]==-1 && bb[3]==0,"Intersect Plane Failed!")
    }

    {
    double n[3]={1,1,1};
    double p[3]={0,0,0};
    double bb[6] = {-1,1,-1,1,-1,1};
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p,n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro( !res
      && bb[0] ==-1 && bb[1]==1
      && bb[2] ==-1 && bb[3]==1
      && bb[4] ==-1 && bb[5] ==1,"Intersect Plane Failed!")
    }
  return EXIT_SUCCESS;
}
