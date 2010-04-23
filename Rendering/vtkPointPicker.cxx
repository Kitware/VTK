/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointPicker.h"

#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMapper.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkImageActor.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPointPicker);

vtkPointPicker::vtkPointPicker()
{
  this->PointId = -1;
}

double vtkPointPicker::IntersectWithLine(double p1[3], double p2[3], double tol, 
                                        vtkAssemblyPath *path, vtkProp3D *p, 
                                        vtkAbstractMapper3D *m)
{
  vtkIdType numPts;
  vtkIdType ptId, minPtId;
  int i;
  double ray[3], rayFactor, tMin, x[3], t, projXYZ[3], minXYZ[3];
  vtkDataSet *input;
  vtkMapper *mapper;
  vtkAbstractVolumeMapper *volumeMapper;
  vtkImageActor *imageActor = NULL;

  // Get the underlying dataset
  //
  if ( (mapper=vtkMapper::SafeDownCast(m)) != NULL )
    {
    input = mapper->GetInput();
    }
  else if ( (volumeMapper=vtkAbstractVolumeMapper::SafeDownCast(m)) != NULL )
    {
    input = volumeMapper->GetDataSetInput();
    }
  else if ( (imageActor=vtkImageActor::SafeDownCast(p)) != NULL )
    {
    input = imageActor->GetInput();
    }
  else
    {
    return 2.0;
    }

  ptId = 0;
  numPts = input->GetNumberOfPoints();

  if ( imageActor != NULL )
    {
    // Restrict the search to the points in the displayed slice
    int extent[6], displayExtent[6], kMin, kMax;
    vtkImageData *imageData = static_cast<vtkImageData *>(input);
    imageData->GetExtent(extent);
    imageActor->GetDisplayExtent(displayExtent);
    kMin = ((displayExtent[4] > extent[4]) ? displayExtent[4] : extent[4]); 
    kMax = ((displayExtent[5] < extent[5]) ? displayExtent[5] : extent[5]); 
    ptId = kMin - extent[4];
    ptId *= extent[3] - extent[2] + 1;
    ptId *= extent[1] - extent[0] + 1;
    numPts = kMax - extent[4] + 1;
    numPts *= extent[3] - extent[2] + 1;
    numPts *= extent[1] - extent[0] + 1;
    }

  if ( numPts <= ptId )
    {
    return 2.0;
    }

  //   Determine appropriate info
  //
  for (i=0; i<3; i++)
    {
    ray[i] = p2[i] - p1[i];
    }
  if (( rayFactor = vtkMath::Dot(ray,ray)) == 0.0 ) 
    {
    vtkErrorMacro("Cannot process points");
    return 2.0;
    }

  //  Project each point onto ray.  Keep track of the one within the
  //  tolerance and closest to the eye (and within the clipping range).
  //
  double dist, maxDist, minPtDist=VTK_DOUBLE_MAX;
  for (minPtId=(-1),tMin=VTK_DOUBLE_MAX; ptId<numPts; ptId++) 
    {
    input->GetPoint(ptId,x);

    t = (ray[0]*(x[0]-p1[0]) + ray[1]*(x[1]-p1[1]) + ray[2]*(x[2]-p1[2])) 
        / rayFactor;

    // If we find a point closer than we currently have, see whether it
    // lies within the pick tolerance and clipping planes. We keep track
    // of the point closest to the line (use a fudge factor for points
    // nearly the same distance away.)
    //
    if ( t >= 0.0 && t <= 1.0 && t <= (tMin+this->Tolerance) ) 
      {
      for(maxDist=0.0, i=0; i<3; i++) 
        {
        projXYZ[i] = p1[i] + t*ray[i];
        dist = fabs(x[i]-projXYZ[i]);
        if ( dist > maxDist )
          {
          maxDist = dist;
          }
        }
      if ( maxDist <= tol && maxDist < minPtDist ) // within tolerance
        {
        minPtId = ptId;
        minXYZ[0]=x[0]; minXYZ[1]=x[1]; minXYZ[2]=x[2];
        minPtDist = maxDist;
        tMin = t; 
       }
      }
    }

  //  Now compare this against other actors.
  //
  if ( minPtId>(-1) && tMin < this->GlobalTMin ) 
    {
    this->MarkPicked(path, p, m, tMin, minXYZ);
    this->PointId = minPtId;
    vtkDebugMacro("Picked point id= " << minPtId);
    }

  return tMin;
}

void vtkPointPicker::Initialize()
{
  this->PointId = (-1);
  this->vtkPicker::Initialize();
}

void vtkPointPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Point Id: " << this->PointId << "\n";
}
