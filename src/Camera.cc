/*=========================================================================

  Program:   OSCAR 
  Module:    Camera.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#include <math.h>
#include "Camera.hh"

vlCamera::vlCamera()
{
  this->FocalPoint[0] = 0.0;
  this->FocalPoint[1] = 0.0;
  this->FocalPoint[2] = 0.0;

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 1.0;

  this->ViewUp[0] = 0.0;
  this->ViewUp[1] = 1.0;
  this->ViewUp[2] = 0.0;

  this->ViewAngle = 30.0;

  this->ClippingRange[0] = 0.01;
  this->ClippingRange[1] = 1000.01;

  this->Switch = 1;
  this->LeftEye = 1;
  this->EyeAngle = 2.0;
}

void vlCamera::SetPosition(float X, float Y, float Z)
{
  this->Position[0] = X;
  this->Position[1] = Y;
  this->Position[2] = Z;
}
void vlCamera::SetPosition(float a[3])
{
  this->Position[0] = a[0];
  this->Position[1] = a[1];
  this->Position[2] = a[2];
}
float *vlCamera::GetPosition()
{
  return (this->Position);
}

void vlCamera::SetFocalPoint(float X, float Y, float Z)
{
  this->FocalPoint[0] = X; 
  this->FocalPoint[1] = Y; 
  this->FocalPoint[2] = Z;
}
void vlCamera::SetFocalPoint(float a[3])
{
  this->FocalPoint[0] = a[0];
  this->FocalPoint[1] = a[1];
  this->FocalPoint[2] = a[2];
}
float *vlCamera::GetFocalPoint()
{
  return (this->FocalPoint);
}

void vlCamera::SetViewUp(float X, float Y, float Z)
{
  this->ViewUp[0] = X;
  this->ViewUp[1] = Y;
  this->ViewUp[2] = Z;
}
void vlCamera::SetViewUp(float a[3])
{
  this->ViewUp[0] = a[0];
  this->ViewUp[1] = a[1];
  this->ViewUp[2] = a[2];
}
float *vlCamera::GetViewUp()
{
  return (this->ViewUp);
}


static void cross(float *a, float *b,float *r)
{
  float x,y,z;

  x = a[1]*b[2] - a[2]*b[1];
  y = a[2]*b[0] - a[0]*b[2];
  z = a[0]*b[1] - a[1]*b[0];

  r[0] = x; r[1] = y; r[2] = z;
}

float vlCamera::GetTwist()
{
  float *vup, *vn;
  float twist = 0;
  float v1[3], v2[3], y_axis[3];
  double theta, dot, mag;
  double cosang;

  vup = this->ViewUp;
  vn = this->GetViewPlaneNormal();

  /*
   * compute: vn X ( vup X vn)
   * and:     vn X ( y-axis X vn)
   * then find the angle between the two projected vectors
   */
  y_axis[0] = y_axis[2] = 0.0; y_axis[1] = 1.0;

  /*
   * bump the view normal if it is parallel to the y-axis
   */
  if ((vn[0] == 0.0) && (vn[2] == 0.0))
    vn[2] = 0.01*vn[1];

  /*
   * first project the view_up onto the view_plane
   */
  cross(vup, vn, v1);
  cross(vn, v1, v1);

  /*
   * then project the y-axis onto the view plane
   */
  cross(y_axis, vn, v2);
  cross(vn, v2, v2);

  /*
   * then find the angle between the two projected vectors
   */
  dot = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
  mag = sqrt((double)(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]));
  mag *= sqrt((double)(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]));

  /* make sure we dont divide by 0 */
  if (mag != 0.0) 
    {
    cosang = dot / mag;
    if (cosang < -1.0) cosang = -1.0;
    if (cosang > 1.0) cosang = 1.0;
    theta = acos(cosang);
    }
  else
    theta = 0.0;

  /*
   * now see if the angle is positive or negative
   */
  cross(v1, v2, v1);
  dot = v1[0]*vn[0] + v1[1]*vn[1] + v1[2]*vn[2];
  
  twist = (theta);
  if (dot < 0.0)
    twist = -twist;
  
  return twist;
}

float *vlCamera::GetViewPlaneNormal()
{
  float dx,dy,dz;
  float distance;
  float vpn[3];

  /*
   * view plane normal is calculated from position and focal point
   */
  dx = this->Position[0] - this->FocalPoint[0];
  dy = this->Position[1] - this->FocalPoint[1];
  dz = this->Position[2] - this->FocalPoint[2];
  
  distance = sqrt(dx*dx+dy*dy+dz*dz);

  if (distance > 0.0) 
    {
    vpn[0] = -dx / distance;
    vpn[1] = -dy / distance;
    vpn[2] = -dz / distance;
    }
  
  vlDebugMacro(<< "Returning ViewPlaneNormal of (" << vpn[0] << " " << vpn[1] << " " << vpn[2] << ")\n");

  return vpn;
}

