/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOculusRenderer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

Parts Copyright Valve Coproration from hellovr_opengl_main.cpp
under their BSD license found here:
https://github.com/ValveSoftware/Oculus/blob/master/LICENSE

=========================================================================*/
#include "vtkOculusRenderer.h"
#include "vtkOculusCamera.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkOculusRenderer);

vtkOculusRenderer::vtkOculusRenderer()
{
  // better default
  this->ClippingRangeExpansion = 0.05;
}

vtkOculusRenderer::~vtkOculusRenderer()
{
}

// Automatically set up the camera based on the visible actors.
// The camera will reposition itself to view the center point of the actors,
// and move along its initial view plane normal (i.e., vector defined from
// camera position to focal point) so that all of the actors can be seen.
void vtkOculusRenderer::ResetCamera()
{
  this->Superclass::ResetCamera();
}


// Automatically set up the camera based on a specified bounding box
// (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
// that its focal point is the center of the bounding box, and adjust its
// distance and position to preserve its initial view plane normal
// (i.e., vector defined from camera position to focal point). Note: if
// the view plane is parallel to the view up axis, the view up axis will
// be reset to one of the three coordinate axes.
void vtkOculusRenderer::ResetCamera(double bounds[6])
{
  double center[3];
  double distance;
  double vn[3], *vup;

  this->GetActiveCamera();
  if ( this->ActiveCamera != NULL )
  {
    this->ActiveCamera->GetViewPlaneNormal(vn);
  }
  else
  {
    vtkErrorMacro(<< "Trying to reset non-existant camera");
    return;
  }

  // Reset the perspective zoom factors, otherwise subsequent zooms will cause
  // the view angle to become very small and cause bad depth sorting.
  this->ActiveCamera->SetViewAngle(110.0);

  this->ExpandBounds(bounds, this->ActiveCamera->GetModelTransformMatrix());

  center[0] = (bounds[0] + bounds[1])/2.0;
  center[1] = (bounds[2] + bounds[3])/2.0;
  center[2] = (bounds[4] + bounds[5])/2.0;

  double w1 = bounds[1] - bounds[0];
  double w2 = bounds[3] - bounds[2];
  double w3 = bounds[5] - bounds[4];
  w1 *= w1;
  w2 *= w2;
  w3 *= w3;
  double radius = w1 + w2 + w3;

  // If we have just a single point, pick a radius of 1.0
  radius = (radius==0)?(1.0):(radius);

  // compute the radius of the enclosing sphere
  radius = sqrt(radius)*0.5;

  // default so that the bounding sphere fits within the view fustrum

  // compute the distance from the intersection of the view frustum with the
  // bounding sphere. Basically in 2D draw a circle representing the bounding
  // sphere in 2D then draw a horizontal line going out from the center of
  // the circle. That is the camera view. Then draw a line from the camera
  // position to the point where it intersects the circle. (it will be tangent
  // to the circle at this point, this is important, only go to the tangent
  // point, do not draw all the way to the view plane). Then draw the radius
  // from the tangent point to the center of the circle. You will note that
  // this forms a right triangle with one side being the radius, another being
  // the target distance for the camera, then just find the target dist using
  // a sin.
  double angle=vtkMath::RadiansFromDegrees(this->ActiveCamera->GetViewAngle());

  this->ComputeAspect();
  double aspect[2];
  this->GetAspect(aspect);

  if(aspect[0]>=1.0) // horizontal window, deal with vertical angle|scale
  {
    if(this->ActiveCamera->GetUseHorizontalViewAngle())
    {
      angle=2.0*atan(tan(angle*0.5)/aspect[0]);
    }
  }
  else // vertical window, deal with horizontal angle|scale
  {
    if(!this->ActiveCamera->GetUseHorizontalViewAngle())
    {
      angle=2.0*atan(tan(angle*0.5)*aspect[0]);
    }
  }
  distance =radius/sin(angle*0.5);

  // check view-up vector against view plane normal
  vup = this->ActiveCamera->GetViewUp();
  if ( fabs(vtkMath::Dot(vup,vn)) > 0.999 )
  {
    vtkWarningMacro(<<"Resetting view-up since view plane normal is parallel");
    this->ActiveCamera->SetViewUp(-vup[2], vup[0], vup[1]);
  }

  // update the camera
  this->ActiveCamera->SetFocalPoint(center[0],center[1],center[2]);
  this->ActiveCamera->SetPosition(center[0]+distance*vn[0],
                                  center[1]+distance*vn[1],
                                  center[2]+distance*vn[2]);

  // now set the cameras shift and scale to the HMD space
  // since the oculus is always in meters (or something like that)
  // we use a shift scale to map view space into hmd view space
  // that way the solar system can be modelled in its units
  // while the shift scale maps it into meters.  This can also
  // be done in the actors but then it requires every actor
  // to be adjusted.  It cannot be done with the camera model
  // matrix as that is broken.
  // The -distance in the Z translation is because we want
  // the center of the world to be in front of the user
  vtkOculusCamera *cam = vtkOculusCamera::SafeDownCast(this->ActiveCamera);
  cam->SetTranslation(-center[0],-center[1],-center[2]-distance);
}

// Alternative version of ResetCamera(bounds[6]);
void vtkOculusRenderer::ResetCamera(double xmin, double xmax,
                              double ymin, double ymax,
                              double zmin, double zmax)
{
  double bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->ResetCamera(bounds);
}
