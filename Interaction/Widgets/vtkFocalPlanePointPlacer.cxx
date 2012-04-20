/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFocalPlanePointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFocalPlanePointPlacer.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPlaneCollection.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkFocalPlanePointPlacer);


//----------------------------------------------------------------------
vtkFocalPlanePointPlacer::vtkFocalPlanePointPlacer()
{
  this->PointBounds[0] = this->PointBounds[2] = this->PointBounds[4] = 0;
  this->PointBounds[1] = this->PointBounds[3] = this->PointBounds[5] = -1;
  this->Offset = 0.0;
}

//----------------------------------------------------------------------
vtkFocalPlanePointPlacer::~vtkFocalPlanePointPlacer()
{
}

//----------------------------------------------------------------------
int vtkFocalPlanePointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                                    double displayPos[2],
                                                    double worldPos[3],
                                                    double worldOrient[9] )
{
  double fp[4];
  ren->GetActiveCamera()->GetFocalPoint(fp);
  fp[3] = 1.0;

  ren->SetWorldPoint(fp);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(fp);

  double tmp[4];
  tmp[0] = displayPos[0];
  tmp[1] = displayPos[1];
  tmp[2] = fp[2];
  ren->SetDisplayPoint(tmp);
  ren->DisplayToWorld();
  ren->GetWorldPoint(tmp);

  // Translate the focal point by "Offset" from the focal plane along the
  // viewing direction.

  double focalPlaneNormal[3];
  ren->GetActiveCamera()->GetDirectionOfProjection( focalPlaneNormal );
  if (ren->GetActiveCamera()->GetParallelProjection())
    {
    tmp[0] += (focalPlaneNormal[0] * this->Offset);
    tmp[1] += (focalPlaneNormal[1] * this->Offset);
    tmp[2] += (focalPlaneNormal[2] * this->Offset);
    }
  else
    {
    double camPos[3], viewDirection[3];
    ren->GetActiveCamera()->GetPosition( camPos );
    viewDirection[0] = tmp[0] - camPos[0];
    viewDirection[1] = tmp[1] - camPos[1];
    viewDirection[2] = tmp[2] - camPos[2];
    vtkMath::Normalize( viewDirection );
    double costheta = vtkMath::Dot( viewDirection, focalPlaneNormal ) /
        (vtkMath::Norm(viewDirection) * vtkMath::Norm(focalPlaneNormal));
    if (costheta != 0.0) // 0.0 Impossible in a perspective projection
      {
      tmp[0] += (viewDirection[0] * this->Offset / costheta);
      tmp[1] += (viewDirection[1] * this->Offset / costheta);
      tmp[2] += (viewDirection[2] * this->Offset / costheta);
      }
    }

  double tolerance[3] = { 1e-12, 1e-12, 1e-12 };
  if ( this->PointBounds[0] < this->PointBounds[1] &&
      !(vtkMath::PointIsWithinBounds( tmp, this->PointBounds, tolerance )))
    {
    return 0;
    }

  worldPos[0] = tmp[0];
  worldPos[1] = tmp[1];
  worldPos[2] = tmp[2];

  this->GetCurrentOrientation( worldOrient );

  return 1;
}

//----------------------------------------------------------------------
int vtkFocalPlanePointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                                    double displayPos[2],
                                                    double refWorldPos[3],
                                                    double worldPos[3],
                                                    double worldOrient[9] )
{
  double tmp[4];
  tmp[0] = refWorldPos[0];
  tmp[1] = refWorldPos[1];
  tmp[2] = refWorldPos[2];
  tmp[3] = 1.0;
  ren->SetWorldPoint(tmp);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(tmp);

  tmp[0] = displayPos[0];
  tmp[1] = displayPos[1];
  tmp[3] = 1.0;
  ren->SetDisplayPoint(tmp);
  ren->DisplayToWorld();
  ren->GetWorldPoint(tmp);

  // Translate the focal point by "Offset" from the focal plane along the
  // viewing direction.

  double focalPlaneNormal[3];
  ren->GetActiveCamera()->GetDirectionOfProjection( focalPlaneNormal );
  if (ren->GetActiveCamera()->GetParallelProjection())
    {
    tmp[0] += (focalPlaneNormal[0] * this->Offset);
    tmp[1] += (focalPlaneNormal[1] * this->Offset);
    tmp[2] += (focalPlaneNormal[2] * this->Offset);
    }
  else
    {
    double camPos[3], viewDirection[3];
    ren->GetActiveCamera()->GetPosition( camPos );
    viewDirection[0] = tmp[0] - camPos[0];
    viewDirection[1] = tmp[1] - camPos[1];
    viewDirection[2] = tmp[2] - camPos[2];
    vtkMath::Normalize( viewDirection );
    double costheta = vtkMath::Dot( viewDirection, focalPlaneNormal ) /
        (vtkMath::Norm(viewDirection) * vtkMath::Norm(focalPlaneNormal));
    if (costheta != 0.0) // 0.0 Impossible in a perspective projection
      {
      tmp[0] += (viewDirection[0] * this->Offset / costheta);
      tmp[1] += (viewDirection[1] * this->Offset / costheta);
      tmp[2] += (viewDirection[2] * this->Offset / costheta);
      }
    }

  double tolerance[3] = { 1e-12, 1e-12, 1e-12 };
  if ( this->PointBounds[0] < this->PointBounds[1] &&
      !(vtkMath::PointIsWithinBounds( tmp, this->PointBounds, tolerance )))
    {
    return 0;
    }

  worldPos[0] = tmp[0];
  worldPos[1] = tmp[1];
  worldPos[2] = tmp[2];

  this->GetCurrentOrientation( worldOrient );

  return 1;
}

//----------------------------------------------------------------------
int vtkFocalPlanePointPlacer::ValidateWorldPosition( double* worldPos )
{
  double tolerance[3] = { 1e-12, 1e-12, 1e-12 };
  if ( this->PointBounds[0] < this->PointBounds[1] &&
    !(vtkMath::PointIsWithinBounds( worldPos, this->PointBounds, tolerance )))
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------
int vtkFocalPlanePointPlacer::ValidateWorldPosition( double* worldPos,
                                                     double* vtkNotUsed(worldOrient) )
{
  double tolerance[3] = { 1e-12, 1e-12, 1e-12 };
  if ( this->PointBounds[0] < this->PointBounds[1] &&
    !(vtkMath::PointIsWithinBounds( worldPos, this->PointBounds, tolerance )))
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------
void vtkFocalPlanePointPlacer::GetCurrentOrientation( double worldOrient[9] )
{
  double *x = worldOrient;
  double *y = worldOrient+3;
  double *z = worldOrient+6;

  x[0] = 1.0;
  x[1] = 0.0;
  x[2] = 0.0;

  y[0] = 0.0;
  y[1] = 1.0;
  y[2] = 0.0;

  z[0] = 0.0;
  z[1] = 0.0;
  z[2] = 1.0;
}

//----------------------------------------------------------------------
void vtkFocalPlanePointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PointBounds: \n";
  os << indent << "  Xmin,Xmax: (" <<
    this->PointBounds[0] << ", " << this->PointBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" <<
    this->PointBounds[2] << ", " << this->PointBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" <<
    this->PointBounds[4] << ", " << this->PointBounds[5] << ")\n";
  os << indent << "Offset: " << this->Offset << endl;
}
