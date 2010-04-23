/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=============================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkObjectFactory.h"
#include "vtkGeoCamera.h"
#include "vtkGeoMath.h"
#include "vtkGeoTerrainNode.h"
#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkTransform.h"
#include <float.h>

vtkStandardNewMacro(vtkGeoCamera);


//----------------------------------------------------------------------------
vtkGeoCamera::vtkGeoCamera()
{
  this->VTKCamera = vtkSmartPointer<vtkCamera>::New();
  this->Transform = vtkSmartPointer<vtkTransform>::New();

  this->OriginLongitude = 0.0;
  this->OriginLatitude = 0.0;

  // Intial state will be looking at earth over the American continent.
  //this->Longitude = -90.0;
  this->Longitude = 0.0;
  this->Latitude = 0.0;
  this->Distance = vtkGeoMath::EarthRadiusMeters() * 5.0;
  this->Heading = 0.0;
  this->Tilt = 90.0;
  this->LockHeading = true;

  this->ComputeRectilinearOrigin();

  this->UpdateAngleRanges();
  this->UpdateVTKCamera();

  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
}

//-----------------------------------------------------------------------------
vtkGeoCamera::~vtkGeoCamera()
{
}

//-----------------------------------------------------------------------------
void vtkGeoCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Distance: " << this->Distance << endl;
  os << indent << "Tilt: " << this->Tilt << endl;
  os << indent << "Heading: " << this->Heading << endl;
  os << indent << "Latitude: " << this->Latitude << endl;
  os << indent << "Longitude: " << this->Longitude << endl;
  os << indent << "LockHeading: " << (this->LockHeading ? "on" : "off");
  os << indent << "Origin: {" << this->Origin[0] << ", " << this->Origin[1] 
    << ", " << this->Origin[2] << "}" << endl;
  os << indent << "OriginLatitude: " << this->OriginLatitude << endl;
  os << indent << "OriginLongitude: " << this->OriginLongitude << endl;
  os << indent << "Position: {" << this->Position[0] << ", " << this->Position[1] 
    << ", " << this->Position[2] << "}" << endl;
  os << indent << "VTKCamera: " << endl;
  
  this->VTKCamera->PrintSelf(os, indent.GetNextIndent());
}
//-----------------------------------------------------------------------------
void vtkGeoCamera::SetOriginLatitude(double oLat)
{
  if (this->OriginLatitude == oLat)
    {
    return;
    }
  this->OriginLatitude = oLat;
  this->Modified();
  this->ComputeRectilinearOrigin();
}

//-----------------------------------------------------------------------------
void vtkGeoCamera::SetOriginLongitude(double oLon)
{
  if (this->OriginLongitude == oLon)
    {
    return;
    }
  this->OriginLongitude = oLon;
  this->Modified();
  this->ComputeRectilinearOrigin();
}

//-----------------------------------------------------------------------------
void vtkGeoCamera::ComputeRectilinearOrigin()
{
  double tmp = cos(this->OriginLatitude * vtkMath::Pi() / 180.0);

  this->Origin[1] = tmp * cos(this->OriginLongitude * vtkMath::Pi() / 180.0);
  this->Origin[0] = -tmp * sin(this->OriginLongitude * vtkMath::Pi() / 180.0);

  this->Origin[2] = sin(this->OriginLatitude * vtkMath::Pi() / 180.0);

  this->Origin[0] *= vtkGeoMath::EarthRadiusMeters();
  this->Origin[1] *= vtkGeoMath::EarthRadiusMeters();
  this->Origin[2] *= vtkGeoMath::EarthRadiusMeters();

  this->UpdateVTKCamera();
}


//-----------------------------------------------------------------------------
void vtkGeoCamera::SetLongitude(double longitude)
{
  if (this->Longitude == longitude)
    {
    return;
    }
  this->Modified();
  this->Longitude = longitude;
  this->UpdateAngleRanges();
  this->UpdateVTKCamera();
}

//-----------------------------------------------------------------------------
void vtkGeoCamera::SetLatitude(double latitude)
{
  if (this->Latitude == latitude)
    {
    return;
    }
  this->Modified();
  this->Latitude = latitude;
  this->UpdateAngleRanges();
  this->UpdateVTKCamera();
}

//-----------------------------------------------------------------------------
void vtkGeoCamera::SetDistance(double altitude)
{
  if (this->Distance == altitude)
    {
    return;
    }
  this->Modified();
  this->Distance = altitude;
  this->UpdateVTKCamera();
}

//-----------------------------------------------------------------------------
void vtkGeoCamera::SetHeading(double heading)
{
  if (this->Heading == heading)
    {
    return;
    }
  while (heading < -180)
    {
    heading += 360;
    }
  while (heading > 180)
    {
    heading -= 360;
    }
  this->Modified();
  this->Heading = heading;
  this->UpdateAngleRanges();
  this->UpdateVTKCamera();
}

//-----------------------------------------------------------------------------
void vtkGeoCamera::SetTilt(double tilt)
{
  if (this->Tilt == tilt)
    {
    return;
    }
  this->Modified();
  this->Tilt = tilt;
  this->UpdateVTKCamera();
}

//-----------------------------------------------------------------------------
void vtkGeoCamera::UpdateAngleRanges()
{
  while (this->Heading > 180.0)
    {
    this->Heading -= 360.0;
    }
  while (this->Heading < -180.0)
    {
    this->Heading += 360.0;
    }
  while (this->Longitude > 180.0)
    {
    this->Longitude -= 360.0;
    }
  while (this->Longitude < -180.0)
    {
    this->Longitude += 360.0;
    }
  if (this->Latitude > 90.0)
    {
    this->Latitude = 180.0 - this->Latitude;  
    }    
  if (this->Latitude < -90.0)
    {
    this->Latitude = -180.0 - this->Latitude;  
    }    
}


//-----------------------------------------------------------------------------
void vtkGeoCamera::UpdateVTKCamera()
{
  this->Transform->PostMultiply();
  this->Transform->Identity();
  // Tilt
  this->Transform->RotateX(this->Tilt);
  // Heading
  this->Transform->RotateY(-this->Heading);
  // to surface of earth
  this->Transform->Translate(0.0, vtkGeoMath::EarthRadiusMeters(), 0.0);
  // Latitude
  this->Transform->RotateX(this->Latitude);
  // Longitude
  this->Transform->RotateZ(this->Longitude);

  // Consider origin.
  this->Transform->Translate(-this->Origin[0],
                             -this->Origin[1],
                             -this->Origin[2]);


  double* pt;
  double tmp[3];
  // Find focal point of the camera.
  pt = this->Transform->TransformDoublePoint(0.0, 0.0, 0.0);
  this->VTKCamera->SetFocalPoint(pt);
  // Find position
  this->VTKCamera->GetFocalPoint(tmp);
  // Use arbitrary magnitude.
  pt = this->Transform->TransformDoublePoint(0.0, 0.0, -this->Distance);
  this->VTKCamera->SetPosition(pt);
  // Save the position with out the origin shift so the evaluation works.
  this->Position[0] = pt[0] + this->Origin[0];
  this->Position[1] = pt[1] + this->Origin[1];
  this->Position[2] = pt[2] + this->Origin[2];

  if (this->LockHeading)
    {
    // Now find view up using heading.
    pt = this->Transform->TransformDoublePoint(0.0, 1.0, 0.0);
    tmp[0] = pt[0] - tmp[0];
    tmp[1] = pt[1] - tmp[1];
    tmp[2] = pt[2] - tmp[2];
    this->VTKCamera->SetViewUp(tmp);    
    }
  else
    {
    // Find heading using view up.
    this->VTKCamera->OrthogonalizeViewUp();
    double up[3];
    this->VTKCamera->GetViewUp(up);
    // Project vector to north pole and view up to the following plane:
    // Normal = dir. out of center of earth from focal point, Point = origin.

    // Compute the plane normal (center of earth at -origin).
    double dir[3];
    dir[0] = -tmp[0] - this->Origin[0];
    dir[1] = -tmp[1] - this->Origin[1];
    dir[2] = -tmp[2] - this->Origin[2];
    vtkMath::Normalize(dir);

    // Compute direction to north pole (at -origin + (0,0,earth_radius))
    // from focal point.
    double north[3];
    north[0] = -this->Origin[0] - tmp[0];
    north[1] = -this->Origin[1] - tmp[1];
    north[2] = vtkGeoMath::EarthRadiusMeters() - this->Origin[2] - tmp[2];
    double northDot = vtkMath::Dot(north, dir);

    // Project direction to north pole to our plane.
    double northProj[3];
    northProj[0] = north[0] - northDot*dir[0];
    northProj[1] = north[1] - northDot*dir[1];
    northProj[2] = north[2] - northDot*dir[2];
    vtkMath::Normalize(northProj);

    // Project view up vector to the same plane.
    double upDot = vtkMath::Dot(up, dir);
    double upProj[3];
    upProj[0] = up[0] - upDot*dir[0];
    upProj[1] = up[1] - upDot*dir[1];
    upProj[2] = up[2] - upDot*dir[2];
    vtkMath::Normalize(upProj);
    
    // Determine the angle between the vectors.
    // Do some sin/cos trickery to get it to the full range
    // [-180, 180].
    double dotProd = vtkMath::Dot(northProj, upProj);
    double crossProd[3];
    vtkMath::Cross(northProj, upProj, crossProd);
    this->Heading = vtkMath::DegreesFromRadians( asin( vtkMath::Norm( crossProd ) ) );
    if (dotProd < 0)
      {
      this->Heading = 180.0 - this->Heading;
      }
    if (vtkMath::Dot(crossProd, dir) < 0)
      {
      this->Heading = -this->Heading;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkGeoCamera::InitializeNodeAnalysis(int rendererSize[2])
{
  this->Aspect[1] = tan( vtkMath::RadiansFromDegrees( this->VTKCamera->GetViewAngle() ) * 0.5 );
  this->Aspect[0] = this->Aspect[1] * rendererSize[0]
                       / static_cast<double>( rendererSize[1] );

  this->VTKCamera->GetViewPlaneNormal(this->ForwardNormal);
  this->ForwardNormal[0] = - this->ForwardNormal[0];
  this->ForwardNormal[1] = - this->ForwardNormal[1];
  this->ForwardNormal[2] = - this->ForwardNormal[2];
  
  this->VTKCamera->GetViewUp(this->UpNormal);
  vtkMath::Normalize(this->UpNormal);
  
  vtkMath::Cross(this->ForwardNormal, this->UpNormal, this->RightNormal);
  
  // It may not be necessary to keep the above as instance variables.
  for (int ii = 0; ii < 3; ++ii)
    {
    this->LeftPlaneNormal[ii] =  - this->ForwardNormal[ii]*this->Aspect[0]
                                   - this->RightNormal[ii];
    this->RightPlaneNormal[ii] = - this->ForwardNormal[ii]*this->Aspect[0]
                                   + this->RightNormal[ii];
    this->DownPlaneNormal[ii] =  - this->ForwardNormal[ii]*this->Aspect[1]
                                   - this->UpNormal[ii];
    this->UpPlaneNormal[ii] =    - this->ForwardNormal[ii]*this->Aspect[1]
                                   + this->UpNormal[ii];
    }
  vtkMath::Normalize(this->LeftPlaneNormal);
  vtkMath::Normalize(this->RightPlaneNormal);
  vtkMath::Normalize(this->DownPlaneNormal);
  vtkMath::Normalize(this->UpPlaneNormal);
  
}


//-----------------------------------------------------------------------------
// This has to be as fast as possible.
// It is called for each node every render.
// These comparisons in rectilinear coordinates are not perfect.
// I look at a viewplain containing the sphere center.
// I compate a circle with the view bounds.
// Another option is to save 4 frustum plane normals in the camera.
// This would require 5 dot products (15 multiplications).
double vtkGeoCamera::GetNodeCoverage(vtkGeoTerrainNode* node)
{
  double sphereCenter[3];
  double sphereRadius;
  double camPosition[3];
  
  this->GetPosition(camPosition);
  
  // Lets take care of nodes on the opposite side of the earth.
  // I think there should be a better way of figuring this out.
  if (vtkMath::Dot(this->ForwardNormal,node->GetCornerNormal00())>0.0 &&
      vtkMath::Dot(this->ForwardNormal,node->GetCornerNormal01())>0.0 &&
      vtkMath::Dot(this->ForwardNormal,node->GetCornerNormal10())>0.0 &&
      vtkMath::Dot(this->ForwardNormal,node->GetCornerNormal11())>0.0)
    { // Node is hidden by the earth.
    return 0.0;
    }
  
  sphereRadius = node->GetBoundingSphereRadius();
  node->GetBoundingSphereCenter(sphereCenter);
  
  // Put the camera's position at the origin.
  sphereCenter[0] = sphereCenter[0] - camPosition[0];
  sphereCenter[1] = sphereCenter[1] - camPosition[1];
  sphereCenter[2] = sphereCenter[2] - camPosition[2];
  
  double left  = vtkMath::Dot(this->LeftPlaneNormal,sphereCenter);
  double right = vtkMath::Dot(this->RightPlaneNormal,sphereCenter);
  double down  = vtkMath::Dot(this->DownPlaneNormal,sphereCenter);
  double up    = vtkMath::Dot(this->UpPlaneNormal,sphereCenter);
  double forward = vtkMath::Dot(this->ForwardNormal,sphereCenter);
  if ( left > sphereRadius || right > sphereRadius ||
       down > sphereRadius || up > sphereRadius ||
       forward < -sphereRadius)
    {
    return 0.0;
    }
    
  if (forward < sphereRadius)
    {  // Camera is probably in the sphere.
    return 1.0;
    }

  left = -left;
  if (left > sphereRadius) { left = sphereRadius;} 
  up = -up;
  if (up > sphereRadius) { up = sphereRadius;} 
  right = -right;
  if (right > sphereRadius) { right = sphereRadius;} 
  down = -down;
  if (down > sphereRadius) { down = sphereRadius;} 


  double coverage = (left+right)*(up+down) / (4.0 * forward*forward*this->Aspect[0]*this->Aspect[1]);
  return coverage;
}

//-----------------------------------------------------------------------------
vtkCamera* vtkGeoCamera::GetVTKCamera()
{
  return this->VTKCamera;
}

