/*=========================================================================

  Program:   Visualization Library
  Module:    Camera.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

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

  vlDebugMacro(<< " Position set to ( " <<  this->Position[0] << ", "
  << this->Position[1] << ", " << this->Position[2] << ")\n");

  // recalculate distance
  this->CalcDistance();
  
  // recalculate view plane normal
  this->CalcViewPlaneNormal();
  
  this->Modified();
}
void vlCamera::SetPosition(float a[3])
{
  this->SetPosition(a[0],a[1],a[2]);
}

void vlCamera::SetFocalPoint(float X, float Y, float Z)
{
  this->FocalPoint[0] = X; 
  this->FocalPoint[1] = Y; 
  this->FocalPoint[2] = Z;

  vlDebugMacro(<< " FocalPoint set to ( " <<  this->FocalPoint[0] << ", "
  << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")\n");

  // recalculate distance
  this->CalcDistance();
  
  // recalculate view plane normal
  this->CalcViewPlaneNormal();
  
  this->Modified();
}
void vlCamera::SetFocalPoint(float a[3])
{
  this->SetFocalPoint(a[0],a[1],a[2]);
}

void vlCamera::SetViewUp(float X, float Y, float Z)
{
  float dx, dy, dz, norm;

  this->ViewUp[0] = X;
  this->ViewUp[1] = Y;
  this->ViewUp[2] = Z;

  // normalize ViewUp
  dx = this->ViewUp[0];
  dy = this->ViewUp[1];
  dz = this->ViewUp[2];
  norm = sqrt( dx * dx + dy * dy + dz * dz );
  
  if(norm != 0) 
    {
    this->ViewUp[0] /= norm;
    this->ViewUp[1] /= norm;
    this->ViewUp[2] /= norm;
    }
  else 
    {
    this->ViewUp[0] = 0;
    this->ViewUp[1] = 1;
    this->ViewUp[2] = 0;
    }
  
  vlDebugMacro(<< " ViewUp set to ( " <<  this->ViewUp[0] << ", "
  << this->ViewUp[1] << ", " << this->ViewUp[2] << ")\n");
  
  this->Modified();
}
void vlCamera::SetViewUp(float a[3])
{
  this->SetViewUp(a[0],a[1],a[2]);
}

void vlCamera::SetClippingRange(float X, float Y)
{
  this->ClippingRange[0] = X; 
  this->ClippingRange[1] = Y; 

  // check the order
  if(this->ClippingRange[0] > this->ClippingRange[1]) 
    {
    float temp;
    vlDebugMacro(<< " Front and back clipping range reversed\n");
    temp = this->ClippingRange[0];
    this->ClippingRange[0] = this->ClippingRange[1];
    this->ClippingRange[1] = temp;
    }
  
  // front should be greater than 0.001
  if (this->ClippingRange[0] < 0.001) 
    {
    this->ClippingRange[1] += 0.001 - this->ClippingRange[0];
    this->ClippingRange[0] = 0.001;
    vlDebugMacro(<< " Front clipping range is set to minimum.\n");
    }
  
  this->Thickness = this->ClippingRange[1] - this->ClippingRange[0];
  
  // thickness should be greater than THICKNESS_MIN
  if (this->Thickness < 0.002) 
    {
    this->Thickness = 0.002;
    vlDebugMacro(<< " ClippingRange thickness is set to minimum.\n");
    
    // set back plane
    this->ClippingRange[1] = this->ClippingRange[0] + this->Thickness;
    }
  
  vlDebugMacro(<< " ClippingRange set to ( " <<  this->ClippingRange[0] << ", "
  << this->ClippingRange[1] << ")\n");

  this->Modified();
}  
void vlCamera::SetClippingRange(float a[2])
{
  this->SetClippingRange(a[0],a[1]);
}

void vlCamera::SetThickness(float X)
{
  if (this->Thickness == X) return;

  this->Thickness = X; 

  // thickness should be greater than THICKNESS_MIN
  if (this->Thickness < 0.002) 
    {
    this->Thickness = 0.002;
    vlDebugMacro(<< " ClippingRange thickness is set to minimum.\n");
    }
  
  // set back plane
  this->ClippingRange[1] = this->ClippingRange[0] + this->Thickness;

  vlDebugMacro(<< " ClippingRange set to ( " <<  this->ClippingRange[0] << ", "
  << this->ClippingRange[1] << ")\n");

  this->Modified();
}  

void vlCamera::SetDistance(float X)
{
  if (this->Distance == X) return;

  this->Distance = X; 

  // Distance should be greater than .002
  if (this->Distance < 0.002) 
    {
    this->Distance = 0.002;
    vlDebugMacro(<< " Distance is set to minimum.\n");
    }
  
  // recalculate FocalPoint
  this->FocalPoint[0] = this->ViewPlaneNormal[0] * 
    this->Distance + this->Position[0];
  this->FocalPoint[1] = this->ViewPlaneNormal[1] * 
    this->Distance + this->Position[1];
  this->FocalPoint[2] = this->ViewPlaneNormal[2] * 
    this->Distance + this->Position[2];

  vlDebugMacro(<< " Distance set to ( " <<  this->Distance << ")\n");

  this->Modified();
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

  // compute: vn X ( vup X vn)
  // and:     vn X ( y-axis X vn)
  // then find the angle between the two projected vectors
  //
  y_axis[0] = y_axis[2] = 0.0; y_axis[1] = 1.0;

  // bump the view normal if it is parallel to the y-axis
  //
  if ((vn[0] == 0.0) && (vn[2] == 0.0))
    vn[2] = 0.01*vn[1];

  // first project the view_up onto the view_plane
  //
  cross(vup, vn, v1);
  cross(vn, v1, v1);

  // then project the y-axis onto the view plane
  //
  cross(y_axis, vn, v2);
  cross(vn, v2, v2);

  // then find the angle between the two projected vectors
  //
  dot = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
  mag = sqrt((double)(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]));
  mag *= sqrt((double)(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]));

  // make sure we dont divide by 0 
  if (mag != 0.0) 
    {
    cosang = dot / mag;
    if (cosang < -1.0) cosang = -1.0;
    if (cosang > 1.0) cosang = 1.0;
    theta = acos(cosang);
    }
  else
    theta = 0.0;

  // now see if the angle is positive or negative
  //
  cross(v1, v2, v1);
  dot = v1[0]*vn[0] + v1[1]*vn[1] + v1[2]*vn[2];
  
  twist = (theta);
  if (dot < 0.0)
    twist = -twist;
  
  return twist;
}

void vlCamera::CalcViewPlaneNormal()
{
  float dx,dy,dz;
  float distance;
  float *vpn = this->ViewPlaneNormal;

  // view plane normal is calculated from position and focal point
  //
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
  
  vlDebugMacro(<< "Calculating ViewPlaneNormal of (" << vpn[0] << " " << vpn[1] << " " << vpn[2] << ")\n");
}

void vlCamera::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlCamera::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);
    
    os << indent << "Clipping Range: (" << this->ClippingRange[0] << ", " 
      << this->ClippingRange[2] << ")\n";
    os << indent << "Distance: " << this->Distance << "\n";
    os << indent << "Eye Angle: " << this->EyeAngle << "\n";
    os << indent << "Focal Point: (" << this->FocalPoint[0] << ", " 
      << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")\n";
    os << indent << "Left Eye: " << this->LeftEye << "\n";
    os << indent << "Position: (" << this->Position[0] << ", " 
      << this->Position[1] << ", " << this->Position[2] << ")\n";
    os << indent << "Switch: " << (this->Switch ? "On\n" : "Off\n");
    os << indent << "Thickness: " << this->Thickness << "\n";
    os << indent << "Twist: " << this->GetTwist() << "\n";
    os << indent << "View Angle: " << this->ViewAngle << "\n";
    os << indent << "View Plane Normal: (" << this->ViewPlaneNormal[0] << ", " 
      << this->ViewPlaneNormal[1] << ", " << this->ViewPlaneNormal[2] << ")\n";
    os << indent << "View Up: (" << this->ViewUp[0] << ", " 
      << this->ViewUp[1] << ", " << this->ViewUp[2] << ")\n";
    }
}

void vlCamera::SetRoll(float roll)
{
  float current;
  float temp[4];

  // roll is a rotation of camera view up about view plane normal
  vlDebugMacro(<< " Setting Roll to " << roll << "\n");

  // get the current roll
  current = this->GetRoll();

  roll -= current;

  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PreMultiply();

  // rotate about view plane normal
  this->Transform.RotateWXYZ(roll,this->ViewPlaneNormal[0],
			     this->ViewPlaneNormal[1],
			     this->ViewPlaneNormal[2]);
  
  // now transform view up
  temp[0] = this->ViewUp[0];
  temp[1] = this->ViewUp[1];
  temp[2] = this->ViewUp[2];
  temp[3] = 1.0;
  this->Transform.VectorMultiply(temp,temp);
  
  // now store the result
  this->SetViewUp(temp);

  this->Transform.Pop();
}

float vlCamera::GetRoll()
{
  float	*orient;
  
  // set roll using orientation
  orient = this->GetOrientation();

  vlDebugMacro(<< " Returning Roll of " << orient[2] << "\n");

  return orient[2];
}

void vlCamera::CalcDistance ()
{
  float   *distance;
  float   dx, dy, dz;
  
  // pickup pointer to distance
  distance = &this->Distance;
  
  dx = this->FocalPoint[0] - this->Position[0];
  dy = this->FocalPoint[1] - this->Position[1];
  dz = this->FocalPoint[2] - this->Position[2];
  
  *distance = sqrt( dx * dx + dy * dy + dz * dz );
  
  // Distance should be greater than .002
  if (this->Distance < 0.002) 
    {
    this->Distance = 0.002;
    vlDebugMacro(<< " Distance is set to minimum.\n");

    // recalculate position
    this->Position[0] = 
      - this->ViewPlaneNormal[0] * *distance + this->FocalPoint[0];
    this->Position[1] = 
      - this->ViewPlaneNormal[1] * *distance + this->FocalPoint[1];
    this->Position[2] = 
      - this->ViewPlaneNormal[2] * *distance + this->FocalPoint[2];
    
    vlDebugMacro(<< " Position set to ( " <<  this->Position[0] << ", "
    << this->Position[1] << ", " << this->Position[2] << ")\n");
    
    vlDebugMacro(<< " Distance set to ( " <<  this->Distance << ")\n");
    this->Modified();
    }
  
  vlDebugMacro(<< " Distance set to ( " <<  this->Distance << ")\n");
  
  this->Modified();
} 

float *vlCamera::GetOrientation ()
{
  // calculate a new orientation
  this->CalcPerspectiveTransform();

  vlDebugMacro(<< " Returning Orientation of ( " <<  this->Orientation[0] 
  << ", " << this->Orientation[1] << ", " << this->Orientation[2] << ")\n");
  
  return this->Orientation;
}

void vlCamera::CalcPerspectiveTransform ()
{
  int     i, j;
  vlMatrix4x4  matrix;
  float   view_ratio;
  float   distance, distance_old;
  float   twist;
  float *temp;

  this->PerspectiveTransform.PostMultiply();  
  this->PerspectiveTransform.Identity();

  // translate to center of projection 
  this->PerspectiveTransform.Translate(-this->Position[0],
				       -this->Position[1],
				       -this->Position[2]);
  
  // first rotate y 
  // rotate around y axis so that result has no x component 
  distance = sqrt((this->Position[0]-this->FocalPoint[0])
		  *(this->Position[0]-this->FocalPoint[0]) +
		  (this->Position[2]-this->FocalPoint[2])*
		  (this->Position[2]-this->FocalPoint[2]));
  /* even with this check there seems to be a bug that causes picking to */
  /* be a little off when looking down the y axis */
  if (distance > 0.0)
    {
    matrix[0][0] = (this->Position[2]-this->FocalPoint[2])/distance;
    matrix[0][2] = -1.0*(this->FocalPoint[0] - 
			 this->Position[0])/distance;
    }
  else
    {
    if (this->Position[1] < this->FocalPoint[1])
      {
      matrix[0][0] = -1.0;
      }
    else
      {
      matrix[0][0] = 1.0;
      }
    matrix[0][2] = 0.0;
    }
  matrix[0][1] = matrix[0][3] = 0.0;
  matrix[1][1] = 1.0;
  matrix[1][0] = matrix[1][2] = matrix[1][3] = 0.0;
  matrix[2][0] = -1.0*matrix[0][2];
  matrix[2][2] = matrix[0][0];
  matrix[2][3] = 0.0;
  matrix[2][1] = 0.0;
  matrix[3][3] = 1.0;
  matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;

  this->PerspectiveTransform.Concatenate(matrix);
  
  // now rotate x 
  // rotate about x axis so that the result has no y component 
  distance_old = distance;
  distance = sqrt((this->Position[0]-this->FocalPoint[0])*
		  (this->Position[0]-this->FocalPoint[0]) +
		  (this->Position[1]-this->FocalPoint[1])*
		  (this->Position[1]-this->FocalPoint[1]) +
		  (this->Position[2]-this->FocalPoint[2])*
		  (this->Position[2]-this->FocalPoint[2]));
  matrix[0][0] = 1.0;
  matrix[0][1] = matrix[0][2] = matrix[0][3] = 0.0;
  matrix[1][1] = distance_old/distance;
  matrix[1][2] = (this->Position[1] - this->FocalPoint[1])/distance;
  matrix[1][0] = matrix[1][3] = 0.0;
  matrix[2][1] = -1.0*matrix[1][2];
  matrix[2][2] = matrix[1][1];
  matrix[2][3] = 0.0;
  matrix[2][0] = 0.0;
  matrix[3][3] = 1.0;
  matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;

  this->PerspectiveTransform.Concatenate(matrix);

  // now rotate z (twist) 
  // convert view up normal to gl twist 
  twist = this->GetTwist();

  matrix[0][0] = cos(-twist);
  matrix[0][1] = sin(-twist);
  matrix[0][2] = matrix[0][3] = 0.0;
  matrix[1][0] = -1.0*matrix[0][1];
  matrix[1][1] = matrix[0][0];
  matrix[1][2] = matrix[1][3] = 0.0;
  matrix[2][1] = 0.0;
  matrix[2][2] = 1.0;
  matrix[2][3] = 0.0;
  matrix[2][0] = 0.0;
  matrix[3][3] = 1.0;
  matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;

  this->PerspectiveTransform.Concatenate(matrix);

  // now set the orientation
  temp = this->PerspectiveTransform.GetOrientation();
  this->Orientation[0] = temp[0];
  this->Orientation[1] = temp[1];
  this->Orientation[2] = temp[2];

  view_ratio   = tan ((fabs (this->ViewAngle) / 2.0) * 
		      (3.1415926 / 180.0));
  matrix[0][0] = 1.0 / view_ratio;
  matrix[0][1] = matrix[0][2] = matrix[0][3] = 0.0;
  matrix[1][1] = 1.0 / view_ratio;
  matrix[1][0] = matrix[1][2] = matrix[1][3] = 0.0;
  matrix[2][2] = -1.0*(this->ClippingRange[1]+this->ClippingRange[0])
    /(this->ClippingRange[1]-this->ClippingRange[0]);
  matrix[2][3] = -1.0;
  matrix[2][0] = matrix[2][1] = 0.0;
  matrix[3][2] = -2.0*this->ClippingRange[1]*this->ClippingRange[0]/ 
    (this->ClippingRange[1]-this->ClippingRange[0]);
  matrix[3][0] = matrix[3][1] = 0.0;
  matrix[3][3] = 0.0;

  this->PerspectiveTransform.Concatenate(matrix);
}


vlMatrix4x4 &vlCamera::GetPerspectiveTransform()
{
  // update transform 
  this->CalcPerspectiveTransform();
  
  // return the transform 
  return this->PerspectiveTransform.GetMatrix();
}

#define SQ_MAG(x) ( (x)[0]*(x)[0] + (x)[1]*(x)[1] + (x)[2]*(x)[2] )

void vlCamera::OrthogonalizeViewUp()
{
  float *normal,*up,temp[3],new_up[3];
  float temp_mag_sq,new_mag_sq,ratio;

  normal=this->ViewPlaneNormal;
  up=this->ViewUp;
  cross(normal,up,temp);

  temp_mag_sq = (SQ_MAG(up));
  cross(temp,normal,new_up);
  new_mag_sq = (SQ_MAG(new_up));
  ratio = sqrt(new_mag_sq/temp_mag_sq);
  this->SetViewUp(new_up[0]*ratio,new_up[1]*ratio,new_up[2]*ratio);
}


void vlCamera::Zoom(float amount)
{
  float	distance;
  
  if (amount <= 0.0) return;
  
  // zoom moves position along view plane normal by a specified ratio
  distance = -this->Distance / amount;
  
  this->SetPosition(this->FocalPoint[0] +distance * this->ViewPlaneNormal[0],
		    this->FocalPoint[1] +distance * this->ViewPlaneNormal[1],
		    this->FocalPoint[2] +distance * this->ViewPlaneNormal[2]);
}


void vlCamera::Azimuth (float angle)
{
  // azimuth is a rotation of camera position about view up vector
  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PreMultiply();
  
  // translate to focal point
  this->Transform.Translate(this->FocalPoint[0],
			    this->FocalPoint[1],
			    this->FocalPoint[2]);
   
  // rotate about view up
  this->Transform.RotateWXYZ(angle,this->ViewUp[0],this->ViewUp[1],
			     this->ViewUp[2]);
  
  
  // translate to focal point
  this->Transform.Translate(-this->FocalPoint[0],
			    -this->FocalPoint[1],
			    -this->FocalPoint[2]);
   
  // now transform position
  this->Transform.SetVector(this->Position[0],this->Position[1],
			    this->Position[2],1.0);
  
  // now store the result
  this->SetPosition(this->Transform.GetVector());
  
  this->Transform.Pop();
}

void vlCamera::Elevation (float angle)
{
  double	axis[3];
  
  // elevation is a rotation of camera position about cross between
  // view up and view plane normal
  axis[0] = (this->ViewUp[1] * this->ViewPlaneNormal[2] -
	     this->ViewUp[2] * this->ViewPlaneNormal[1]);
  axis[1] = (this->ViewUp[2] * this->ViewPlaneNormal[0] -
	     this->ViewUp[0] * this->ViewPlaneNormal[2]);
  axis[2] = (this->ViewUp[0] * this->ViewPlaneNormal[1] -
	     this->ViewUp[1] * this->ViewPlaneNormal[0]);

  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PreMultiply();
  
  // translate to focal point
  this->Transform.Translate(this->FocalPoint[0],
			    this->FocalPoint[1],
			    this->FocalPoint[2]);
   
  // rotate about view up
  this->Transform.RotateWXYZ(angle,axis[0],axis[1],axis[2]);
  
  
  // translate to focal point
  this->Transform.Translate(-this->FocalPoint[0],
			    -this->FocalPoint[1],
			    -this->FocalPoint[2]);
   
  // now transform position
  this->Transform.SetVector(this->Position[0],this->Position[1],
			    this->Position[2],1.0);
  
  // now store the result
  this->SetPosition(this->Transform.GetVector());
  
  this->Transform.Pop();
}

void vlCamera::Yaw (float angle)
{
  // yaw is a rotation of camera focal_point about view up vector
  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PreMultiply();
  
  // translate to position
  this->Transform.Translate(this->Position[0],
			    this->Position[1],
			    this->Position[2]);
   
  // rotate about view up
  this->Transform.RotateWXYZ(angle,this->ViewUp[0],this->ViewUp[1],
			     this->ViewUp[2]);
  
  
  // translate to position
  this->Transform.Translate(-this->Position[0],
			    -this->Position[1],
			    -this->Position[2]);
   
  // now transform focal point
  this->Transform.SetVector(this->FocalPoint[0],this->FocalPoint[1],
			    this->FocalPoint[2],1.0);
  
  // now store the result
  this->SetFocalPoint(this->Transform.GetVector());
  
  this->Transform.Pop();
}

void vlCamera::Pitch (float angle)
{
  float	axis[3];

  
  // pitch is a rotation of camera focal point about cross between
  // view up and view plane normal
  axis[0] = (this->ViewUp[1] * this->ViewPlaneNormal[2] -
	     this->ViewUp[2] * this->ViewPlaneNormal[1]);
  axis[1] = (this->ViewUp[2] * this->ViewPlaneNormal[0] -
	     this->ViewUp[0] * this->ViewPlaneNormal[2]);
  axis[2] = (this->ViewUp[0] * this->ViewPlaneNormal[1] -
	     this->ViewUp[1] * this->ViewPlaneNormal[0]);

  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PreMultiply();
  
  // translate to position
  this->Transform.Translate(this->Position[0],
			    this->Position[1],
			    this->Position[2]);
   
  // rotate about view up
  this->Transform.RotateWXYZ(angle,axis[0],axis[1],axis[2]);
  
  // translate to position
  this->Transform.Translate(-this->Position[0],
			    -this->Position[1],
			    -this->Position[2]);
   
  // now transform focal point
  this->Transform.SetVector(this->FocalPoint[0],this->FocalPoint[1],
			    this->FocalPoint[2],1.0);
  
  // now store the result
  this->SetFocalPoint(this->Transform.GetVector());
  
  this->Transform.Pop();
}

void vlCamera::Roll (float angle)
{
  
  // roll is a rotation of camera view up about view plane normal
  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PreMultiply();

  // rotate about view plane normal
  this->Transform.RotateWXYZ(angle,this->ViewPlaneNormal[0],
			     this->ViewPlaneNormal[1],
			     this->ViewPlaneNormal[2]);
  
  // now transform view up
  this->Transform.SetVector(this->ViewUp[0],this->ViewUp[1],
			    this->ViewUp[2],1.0);
  
  // now store the result
  this->SetViewUp(this->Transform.GetVector());

  this->Transform.Pop();
}



