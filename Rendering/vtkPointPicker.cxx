/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointPicker.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointPicker.h"
#include "vtkMath.h"
#include "vtkVolumeMapper.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPointPicker, "1.27");
vtkStandardNewMacro(vtkPointPicker);

vtkPointPicker::vtkPointPicker()
{
  this->PointId = -1;
}

float vtkPointPicker::IntersectWithLine(float p1[3], float p2[3], float tol, 
                                        vtkAssemblyPath *path, vtkProp3D *p, 
                                        vtkAbstractMapper3D *m)
{
  vtkIdType numPts;
  vtkIdType ptId, minPtId;
  int i;
  float ray[3], rayFactor, tMin, x[3], t, projXYZ[3], minXYZ[3];
  vtkDataSet *input;
  vtkMapper *mapper;
  vtkVolumeMapper *volumeMapper;

  // Get the underlying dataset
  //
  if ( (mapper=vtkMapper::SafeDownCast(m)) != NULL )
    {
    input = mapper->GetInput();
    }
  else if ( (volumeMapper=vtkVolumeMapper::SafeDownCast(m)) != NULL )
    {
    input = volumeMapper->GetInput();
    }
  else
    {
    return 2.0;
    }

  if ( (numPts = input->GetNumberOfPoints()) < 1 )
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
  for (minPtId=(-1),tMin=VTK_LARGE_FLOAT,ptId=0; ptId<numPts; ptId++) 
    {
    input->GetPoint(ptId,x);

    t = (ray[0]*(x[0]-p1[0]) + ray[1]*(x[1]-p1[1]) + ray[2]*(x[2]-p1[2])) 
        / rayFactor;

    //  If we find a point closer than we currently have, see whether it
    //  lies within the pick tolerance and clipping planes.
    //
    if ( t >= 0.0 && t <= 1.0 && t < tMin ) 
      {
      for(i=0; i<3; i++) 
        {
        projXYZ[i] = p1[i] + t*ray[i];
        if ( fabs(x[i]-projXYZ[i]) > tol )
          {
          break;
          }
        }
      if ( i > 2 ) // within tolerance
        {
        minPtId = ptId;
        minXYZ[0]=x[0]; minXYZ[1]=x[1]; minXYZ[2]=x[2];
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
