/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCamera.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTimeStamp.h"
#include "vtkGraphicsFactory.h"

// Construct camera instance with its focal point at the origin, 
// and position=(0,0,1). The view up is along the y-axis, 
// view angle is 30 degrees, and the clipping range is (.1,1000).
vtkCamera::vtkCamera()
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

  this->ParallelProjection = 0;
  this->ParallelScale = 1.0;
  this->LeftEye = 1;
  this->EyeAngle = 2.0;

  this->Thickness = 1000.0;
  this->Distance = 1.0;

  this->ViewPlaneNormal[0] = 0.0;
  this->ViewPlaneNormal[1] = 0.0;
  this->ViewPlaneNormal[2] = 1.0;

  this->Orientation[0] = 0.0;
  this->Orientation[1] = 0.0;
  this->Orientation[2] = 0.0;
  
  this->WindowCenter[0] = 0.0;
  this->WindowCenter[1] = 0.0;
  
  this->FocalDisk = 1.0;
  this->Stereo = 0;
  this->VPNDotDOP = 0.0;

  this->Transform = vtkTransform::New();
  this->PerspectiveTransform = vtkProjectionTransform::New();
}


vtkCamera::~vtkCamera()
{
  this->Transform->Delete();
  this->PerspectiveTransform->Delete();
}


// return the correct type of Camera 
vtkCamera *vtkCamera::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkCamera");
  return (vtkCamera*)ret;
}

void vtkCamera::SetPosition(double X, double Y, double Z)
{
  if (X == this->Position[0] && Y == this->Position[1] 
      && Z == this->Position[2])
    {
    return;
    }
  
  this->Position[0] = X;
  this->Position[1] = Y;
  this->Position[2] = Z;

  vtkDebugMacro(<< " Position set to ( " <<  this->Position[0] << ", "
  << this->Position[1] << ", " << this->Position[2] << ")");

  // recalculate distance
  this->ComputeDistance();
  
  this->Modified();
}

void vtkCamera::SetFocalPoint(double X, double Y, double Z)
{
  if (X == this->FocalPoint[0] && Y == this->FocalPoint[1] 
      && Z == this->FocalPoint[2])
    {
    return;
    }

  this->FocalPoint[0] = X; 
  this->FocalPoint[1] = Y; 
  this->FocalPoint[2] = Z;

  vtkDebugMacro(<< " FocalPoint set to ( " <<  this->FocalPoint[0] << ", "
  << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")");

  // recalculate distance
  this->ComputeDistance();
  
  this->Modified();
}

void vtkCamera::SetViewUp(double X, double Y, double Z)
{
  double norm;

  // normalize ViewUp
  norm = sqrt( X * X + Y * Y + Z * Z );
  
  if(norm != 0) 
    {
    X /= norm;
    Y /= norm;
    Z /= norm;
    }
  else 
    {
    X = 0;
    Y = 1;
    Z = 0;
    }
  
  if (X == this->ViewUp[0] && Y == this->ViewUp[1] 
      && Z == this->ViewUp[2])
    {
    return;
    }

  this->ViewUp[0] = X;
  this->ViewUp[1] = Y;
  this->ViewUp[2] = Z;

  vtkDebugMacro(<< " ViewUp set to ( " <<  this->ViewUp[0] << ", "
  << this->ViewUp[1] << ", " << this->ViewUp[2] << ")");
  
  this->Modified();
}

void vtkCamera::SetClippingRange(double X, double Y)
{
  double thickness;
  
  // check the order
  if(X > Y) 
    {
    double temp;
    vtkDebugMacro(<< " Front and back clipping range reversed");
    temp = X;
    X = Y;
    Y = temp;
    }
  
  // front should be greater than 0.0001
  if (X < 0.0001) 
    {
    Y += 0.0001 - X;
    X = 0.0001;
    vtkDebugMacro(<< " Front clipping range is set to minimum.");
    }
  
  thickness = Y - X;
  
  // thickness should be greater than 0.0001
  if (thickness < 0.0001) 
    {
    thickness = 0.0001;
    vtkDebugMacro(<< " ClippingRange thickness is set to minimum.");
    
    // set back plane
    Y = X + thickness;
    }
  
  if (X == this->ClippingRange[0] && Y == this->ClippingRange[1] &&
      this->Thickness == thickness)
    {
    return;
    }

  this->ClippingRange[0] = X; 
  this->ClippingRange[1] = Y; 
  this->Thickness = thickness;
  
  vtkDebugMacro(<< " ClippingRange set to ( " <<  this->ClippingRange[0] << ", "
  << this->ClippingRange[1] << ")");

  this->Modified();
}  

// Set the distance between clipping planes. A side effect of this method is
// to adjust the back clipping plane to be equal to the front clipping plane 
// plus the thickness.
void vtkCamera::SetThickness(double X)
{
  if (this->Thickness == X)
    {
    return;
    }

  this->Thickness = X; 

  // thickness should be greater than 0.0001
  if (this->Thickness < 0.0001) 
    {
    this->Thickness = 0.0001;
    vtkDebugMacro(<< " ClippingRange thickness is set to minimum.");
    }
  
  // set back plane
  this->ClippingRange[1] = this->ClippingRange[0] + this->Thickness;

  vtkDebugMacro(<< " ClippingRange set to ( " <<  this->ClippingRange[0] << ", "
  << this->ClippingRange[1] << ")");

  this->Modified();
}  

// Set the distance of the focal point from the camera. The focal point is 
// modified accordingly. This should be positive.
void vtkCamera::SetDistance(double X)
{
  if (this->Distance == X)
    {
    return;
    }

  this->Distance = X; 

  // Distance should be greater than .0002
  if (this->Distance < 0.0002) 
    {
    this->Distance = 0.0002;
    vtkDebugMacro(<< " Distance is set to minimum.");
    }
  
  // recalculate FocalPoint
  this->FocalPoint[0] = this->Position[0] - 
    this->ViewPlaneNormal[0] * this->Distance;
  this->FocalPoint[1] = this->Position[1] - 
    this->ViewPlaneNormal[1] * this->Distance;
  this->FocalPoint[2] = this->Position[2] - 
    this->ViewPlaneNormal[2] * this->Distance;

  vtkDebugMacro(<< " Distance set to ( " <<  this->Distance << ")");

  this->Modified();
}  

// Compute the view plane normal from the position and focal point.
void vtkCamera::ComputeViewPlaneNormal()
{
  double dx,dy,dz;
  double distance;
  double *vpn = this->ViewPlaneNormal;

  // view plane normal is calculated from position and focal point
  //
  dx = this->Position[0] - this->FocalPoint[0];
  dy = this->Position[1] - this->FocalPoint[1];
  dz = this->Position[2] - this->FocalPoint[2];
  
  distance = sqrt(dx*dx+dy*dy+dz*dz);

  if (distance > 0.0) 
    {
    vpn[0] = dx / distance;
    vpn[1] = dy / distance;
    vpn[2] = dz / distance;
    }
  
  vtkDebugMacro(<< "Calculating ViewPlaneNormal of (" << vpn[0] << " " << vpn[1] << " " << vpn[2] << ")");
}


// Set the roll angle of the camera about the view plane normal.
void vtkCamera::SetRoll(double roll)
{
  double current;
  double temp[3];

  // roll is a rotation of camera view up about view plane normal
  vtkDebugMacro(<< " Setting Roll to " << roll << "");

  // get the current roll
  current = this->GetRoll();

  if (fabs(current - roll) < 0.00001)
    {
    return;
    }
  
  roll -= current;

  this->Transform->Push();
  this->Transform->Identity();
  this->Transform->PreMultiply();

  // rotate about view plane normal
  this->Transform->RotateWXYZ(-roll,this->ViewPlaneNormal[0],
			      this->ViewPlaneNormal[1],
			      this->ViewPlaneNormal[2]);

  this->Transform->TransformVector(this->ViewUp,temp);
  this->SetViewUp(temp);
  
  this->Transform->Pop();
}

// Returns the roll of the camera.
double vtkCamera::GetRoll()
{
  double *orient;
  
  // set roll using orientation
  orient = this->GetOrientation();

  vtkDebugMacro(<< " Returning Roll of " << orient[2] << "");

  return orient[2];
}

// Compute the camera distance, which is the distance between the 
// focal point and position.
void vtkCamera::ComputeDistance()
{
  double *distance;
  double dx, dy, dz;
  
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
    vtkDebugMacro(<< " Distance is set to minimum.");

    // recalculate position
    this->Position[0] = 
      this->ViewPlaneNormal[0] * *distance + this->FocalPoint[0];
    this->Position[1] = 
      this->ViewPlaneNormal[1] * *distance + this->FocalPoint[1];
    this->Position[2] = 
      this->ViewPlaneNormal[2] * *distance + this->FocalPoint[2];
    
    vtkDebugMacro(<< " Position set to ( " <<  this->Position[0] << ", "
    << this->Position[1] << ", " << this->Position[2] << ")");
    
    vtkDebugMacro(<< " Distance set to ( " <<  this->Distance << ")");
    this->Modified();
    }
  
  vtkDebugMacro(<< " Distance set to ( " <<  this->Distance << ")");
  
  this->Modified();
} 

// Returns the orientation of the camera. This is a vector of X,Y and Z 
// rotations that when performed in the order RotateZ, RotateX, and finally
// RotateY, will yield the same 3x3 rotation matrix for the camera.
double *vtkCamera::GetOrientation()
{
  // calculate a new orientation
  this->Transform->SetMatrix(this->GetViewTransformMatrix());

  float tmp[3];
  this->Transform->GetOrientation(tmp[0],tmp[1],tmp[2]);
  this->Orientation[0] = tmp[0];
  this->Orientation[1] = tmp[1];
  this->Orientation[2] = tmp[2];

  vtkDebugMacro(<< " Returning Orientation of ( " <<  this->Orientation[0] 
  << ", " << this->Orientation[1] << ", " << this->Orientation[2] << ")");
  
  return this->Orientation;
}

// Returns the WXYZ orientation of the camera. 
float *vtkCamera::GetOrientationWXYZ()
{
  this->Transform->SetMatrix(this->GetViewTransformMatrix());
  return this->Transform->GetOrientationWXYZ();
}

// Compute the view transform matrix. This is used in converting 
// between view and world coordinates.  It is a rigid-body matrix:
// only translation and rotation.
void vtkCamera::ComputeViewTransform()
{
  this->PerspectiveTransform->SetupCamera(this->Position, 
					  this->FocalPoint,
					  this->ViewUp);
}

// Compute the perspective transform matrix. This is used in converting 
// between view and world coordinates.
void vtkCamera::ComputePerspectiveTransform(double aspect, 
					    double nearz, double farz)
{
  // adjust Z-buffer range
  this->PerspectiveTransform->AdjustZBuffer(-1, +1, nearz, farz);

  if (this->ParallelProjection)
    {
    // set up a rectangular parallelipiped

    double width = this->ParallelScale*aspect;
    double height = this->ParallelScale;

    double xmin = (this->WindowCenter[0]-1.0)*width;
    double xmax = (this->WindowCenter[0]+1.0)*width;
    double ymin = (this->WindowCenter[1]-1.0)*height;
    double ymax = (this->WindowCenter[1]+1.0)*height;

    this->PerspectiveTransform->Ortho(xmin,xmax,ymin,ymax,
				      this->ClippingRange[0],
				      this->ClippingRange[1]);
    }
  else if (this->WindowCenter[0] != 0.0 || this->WindowCenter[1] != 0.0)
    {
    // set up an off-axis frustum

    double tmp = tan(this->ViewAngle*vtkMath::DoubleDegreesToRadians()/2);
    double width = this->ClippingRange[0]*tmp*aspect;
    double height = this->ClippingRange[0]*tmp;

    double xmin = (this->WindowCenter[0]-1.0)*width;
    double xmax = (this->WindowCenter[0]+1.0)*width;
    double ymin = (this->WindowCenter[1]-1.0)*height;
    double ymax = (this->WindowCenter[1]+1.0)*height;

    this->PerspectiveTransform->Frustum(xmin, xmax, ymin, ymax,
					this->ClippingRange[0],
					this->ClippingRange[1]);
    }
  else
    {
    // set up an on-axis frustum

    this->PerspectiveTransform->Perspective(this->ViewAngle,aspect,
					    this->ClippingRange[0],
					    this->ClippingRange[1]);
    }

  if (this->Stereo)
    {
    // set up a shear for stereo views

    if (this->LeftEye)
      {
      this->PerspectiveTransform->Stereo(-this->EyeAngle/2,this->Distance);
      }
    else
      {
      this->PerspectiveTransform->Stereo(+this->EyeAngle/2,this->Distance);
      }
    }
}


// Return the perspective transform matrix. See ComputePerspectiveTransform.
vtkMatrix4x4 *vtkCamera::GetPerspectiveTransformMatrix(double aspect,
						       double nearz,
						       double farz)
{
  this->PerspectiveTransform->Identity();
  this->ComputePerspectiveTransform(aspect, nearz, farz);
  
  // return the transform 
  return this->PerspectiveTransform->GetMatrixPointer();
}

// Return the perspective transform matrix. See ComputePerspectiveTransform.
vtkMatrix4x4 *vtkCamera::GetViewTransformMatrix()
{
  this->PerspectiveTransform->Identity();
  this->ComputeViewTransform();
  
  // return the transform 
  return this->PerspectiveTransform->GetMatrixPointer();
}

// Return the perspective transform matrix. See ComputePerspectiveTransform.
vtkMatrix4x4 *vtkCamera::GetCompositePerspectiveTransformMatrix(double aspect,
								double nearz,
								double farz)
{
  this->PerspectiveTransform->Identity();
  this->ComputePerspectiveTransform(aspect, nearz, farz);
  this->ComputeViewTransform();
  
  // return the transform 
  return this->PerspectiveTransform->GetMatrixPointer();
}

// Recompute the view up vector so that it is perpendicular to the
// view plane normal.
void vtkCamera::OrthogonalizeViewUp()
{
  double vec[3];

  vtkMath::Cross(this->ViewPlaneNormal,this->ViewUp,vec);
  vtkMath::Cross(vec,this->ViewPlaneNormal,vec);
  vtkMath::Normalize(vec);

  this->SetViewUp(vec);
}

// Move the position of the camera along the view plane normal. Moving
// towards the focal point (e.g., > 1) is a dolly-in, moving away 
// from the focal point (e.g., < 1) is a dolly-out.
void vtkCamera::Dolly(double amount)
{
  if (amount <= 0.0)
    {
    return;
    }
  
  // zoom moves position along view plane normal by a specified ratio
  double distance =  this->Distance / amount;
  
  this->SetPosition(this->FocalPoint[0] + distance * this->ViewPlaneNormal[0],
		    this->FocalPoint[1] + distance * this->ViewPlaneNormal[1],
		    this->FocalPoint[2] + distance * this->ViewPlaneNormal[2]);
}

// Change the ViewAngle of the camera so that more or less of a scene 
// occupies the viewport. A value > 1 is a zoom-in. A value < 1 is a zoom-out.
void vtkCamera::Zoom(double amount)
{
  if (amount <= 0.0)
    {
    return;
    }
  
  this->ViewAngle = this->ViewAngle/amount;

  this->ViewingRaysModified();
}


// Rotate the camera about the view up vector centered at the focal point.
void vtkCamera::Azimuth(double angle)
{
  double temp[3];

  // azimuth is a rotation of camera position about view up vector
  this->Transform->Push();
  this->Transform->Identity();
  this->Transform->PostMultiply();
  
  // translate to focal point
  this->Transform->Translate(-this->FocalPoint[0],
			     -this->FocalPoint[1],
			     -this->FocalPoint[2]);
   
  // rotate about view up
  this->Transform->RotateWXYZ(angle,this->ViewUp[0],this->ViewUp[1],
			      this->ViewUp[2]);
  
  // translate to focal point
  this->Transform->Translate(this->FocalPoint[0],
			     this->FocalPoint[1],
			     this->FocalPoint[2]);
   

  // now transform position
  this->Transform->TransformPoint(this->Position,temp);
  this->SetPosition(temp);

  // also azimuth the vpn
  this->Transform->TransformNormal(this->ViewPlaneNormal,temp);
  this->SetViewPlaneNormal(temp);
  
  this->Transform->Pop();
}

// Rotate the camera about the cross product of the view plane normal and 
// the view up vector centered on the focal point.
void vtkCamera::Elevation(double angle)
{
  double axis[3], temp[3];
  
  // elevation is a rotation of camera position about cross between
  // view plane normal and view up
  axis[0] = (this->ViewPlaneNormal[1] * this->ViewUp[2] -
	     this->ViewPlaneNormal[2] * this->ViewUp[1]);
  axis[1] = (this->ViewPlaneNormal[2] * this->ViewUp[0] -
	     this->ViewPlaneNormal[0] * this->ViewUp[2]);
  axis[2] = (this->ViewPlaneNormal[0] * this->ViewUp[1] -
	     this->ViewPlaneNormal[1] * this->ViewUp[0]);

  this->Transform->Push();
  this->Transform->Identity();
  this->Transform->PostMultiply();
  
  // translate to focal point
  this->Transform->Translate(-this->FocalPoint[0],
			     -this->FocalPoint[1],
			     -this->FocalPoint[2]);
   
  // rotate about view up
  this->Transform->RotateWXYZ(angle,axis[0],axis[1],axis[2]);
  
  
  // translate to focal point
  this->Transform->Translate(this->FocalPoint[0],
			     this->FocalPoint[1],
			     this->FocalPoint[2]);
   
  // now transform position
  this->Transform->TransformPoint(this->Position,temp);
  this->SetPosition(temp);

  // also elevate the vpn
  this->Transform->TransformNormal(this->ViewPlaneNormal,temp);
  this->SetViewPlaneNormal(temp);
  
  this->Transform->Pop();
}

// Rotate the focal point about the view up vector centered at the camera's 
// position. 
void vtkCamera::Yaw(double angle)
{
  double temp[3];

  // yaw is a rotation of camera focal_point about view up vector
  this->Transform->Push();
  this->Transform->Identity();
  this->Transform->PostMultiply();
  
  // translate to position
  this->Transform->Translate(-this->Position[0],
			     -this->Position[1],
			     -this->Position[2]);

  // rotate about view up
  this->Transform->RotateWXYZ(angle,this->ViewUp[0],this->ViewUp[1],
			      this->ViewUp[2]);
  
  // translate to position
  this->Transform->Translate(this->Position[0],
			     this->Position[1],
			     this->Position[2]);

  // now transform focal point
  this->Transform->TransformPoint(this->FocalPoint,temp);
  this->SetFocalPoint(temp);

  // also yaw the vpn
  this->Transform->TransformNormal(this->ViewPlaneNormal,temp);
  this->SetViewPlaneNormal(temp);

  this->Transform->Pop();
}

// Rotate the focal point about the cross product of the view up vector 
// and the view plane normal, centered at the camera's position.
void vtkCamera::Pitch(double angle)
{
  double axis[3],temp[3];

  
  // pitch is a rotation of camera focal point about cross between
  // view up and view plane normal
  axis[0] = (this->ViewUp[1] * this->ViewPlaneNormal[2] -
	     this->ViewUp[2] * this->ViewPlaneNormal[1]);
  axis[1] = (this->ViewUp[2] * this->ViewPlaneNormal[0] -
	     this->ViewUp[0] * this->ViewPlaneNormal[2]);
  axis[2] = (this->ViewUp[0] * this->ViewPlaneNormal[1] -
	     this->ViewUp[1] * this->ViewPlaneNormal[0]);

  this->Transform->Push();
  this->Transform->Identity();
  this->Transform->PostMultiply();
  
  // translate to position
  this->Transform->Translate(-this->Position[0],
			     -this->Position[1],
			     -this->Position[2]);

  // rotate about view up
  this->Transform->RotateWXYZ(angle,axis[0],axis[1],axis[2]);
  
   
  // translate to position
  this->Transform->Translate(this->Position[0],
			     this->Position[1],
			     this->Position[2]);

  // now transform focal point
  this->Transform->TransformPoint(this->FocalPoint,temp);
  this->SetFocalPoint(temp);

  // also pitch the vpn
  this->Transform->TransformNormal(this->ViewPlaneNormal,temp);
  this->SetViewPlaneNormal(temp);
  
  this->Transform->Pop();
}

// Rotate the camera around the view plane normal.
void vtkCamera::Roll(double angle)
{
  double temp[3];

  // roll is a rotation of camera view up about view plane normal
  this->Transform->Push();
  this->Transform->Identity();
  this->Transform->PreMultiply();

  // rotate about view plane normal
  this->Transform->RotateWXYZ(-angle,this->ViewPlaneNormal[0],
			      this->ViewPlaneNormal[1],
			      this->ViewPlaneNormal[2]);
  
  // now transform view up
  this->Transform->TransformVector(this->ViewUp,temp);
  this->SetViewUp(temp);

  this->Transform->Pop();
}

// Set the direction that the camera points.
// Adjusts position to be consistent with the view plane normal.
void vtkCamera::SetViewPlaneNormal(double X,double Y,double Z)
{
  double norm;
  double dx, dy, dz;
  double dot_product;

  // normalize it
  norm = sqrt( X * X + Y * Y + Z * Z );
  if (norm == 0.0)
    {
    vtkErrorMacro(<< "SetViewPlaneNormal of (0,0,0)");
    return;
    }
  
  X = X/norm;
  Y = Y/norm;
  Z = Z/norm;
  
  if (X == this->ViewPlaneNormal[0] && Y == this->ViewPlaneNormal[1] 
      && Z == this->ViewPlaneNormal[2])
    {
    return;
    }
  
  this->ViewPlaneNormal[0] = X;
  this->ViewPlaneNormal[1] = Y;
  this->ViewPlaneNormal[2] = Z;

  vtkDebugMacro(<< " ViewPlaneNormal set to ( " << X << ", "
    << Y << ", " << Z << ")");
 
  // Compute the dot product between the view plane normal and the 
  // direction of projection. If this has changed, the viewing rays need 
  // to be recalculated.
  dx = this->Position[0] - this->FocalPoint[0];
  dy = this->Position[1] - this->FocalPoint[1];
  dz = this->Position[2] - this->FocalPoint[2];
  
  norm = sqrt(dx*dx+dy*dy+dz*dz);

  if (norm > 0.0) 
    {
    dx = dx / norm;
    dy = dy / norm;
    dz = dz / norm;
    }
  
  dot_product = dx*this->ViewPlaneNormal[0] +
		dy*this->ViewPlaneNormal[1] +
		dz*this->ViewPlaneNormal[2];

  if( fabs((this->VPNDotDOP - dot_product)) > 0.001 )
    {
    this->VPNDotDOP = dot_product;
    this->ViewingRaysModified();
    }

  this->Modified();
}

// Return the 6 planes (Ax + By + Cz + D = 0) that bound
// the view frustum. 
void vtkCamera::GetFrustumPlanes(float aspect, float planes[24])
{
  int i;
  double f, normals[6][4], matrix[4][4];

  // set up the normals
  for (i = 0; i < 6; i++)
    {
    normals[i][0] = 0.0;
    normals[i][1] = 0.0;
    normals[i][2] = 0.0;
    normals[i][3] = 1.0;
    // if i is even set to -1, if odd set to +1 
    normals[i][i/2] = 1 - (i%2)*2;
    }

  // get the composite perspective matrix
  vtkMatrix4x4::DeepCopy(*matrix, 
	this->GetCompositePerspectiveTransformMatrix(aspect,-1,+1));
  
  // transpose the matrix for use with normals
  vtkMatrix4x4::Transpose(*matrix,*matrix);
  
  // transform the normals to world coordinates
  for (i = 0; i < 6; i++)
    {
    vtkMatrix4x4::MultiplyPoint(*matrix,normals[i],normals[i]);

    f = 1.0/sqrt(normals[i][0]*normals[i][0] +
		 normals[i][1]*normals[i][1] +
		 normals[i][2]*normals[i][2]);

    planes[4*i + 0] = normals[i][0]*f;
    planes[4*i + 1] = normals[i][1]*f;
    planes[4*i + 2] = normals[i][2]*f;
    planes[4*i + 3] = normals[i][3]*f;
    }
}

unsigned long int vtkCamera::GetViewingRaysMTime()
{
  return this->ViewingRaysMTime.GetMTime();
}

void vtkCamera::ViewingRaysModified()
{
  this->ViewingRaysMTime.Modified();
}

void vtkCamera::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  // update orientation
  this->GetOrientation();
  
  os << indent << "Clipping Range: (" << this->ClippingRange[0] << ", " 
    << this->ClippingRange[1] << ")\n";
  os << indent << "Distance: " << this->Distance << "\n";
  os << indent << "Eye Angle: " << this->EyeAngle << "\n";
  os << indent << "Focal Disk: " << this->FocalDisk << "\n";
  os << indent << "Focal Point: (" << this->FocalPoint[0] << ", " 
    << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")\n";
  os << indent << "Left Eye: " << this->LeftEye << "\n";
  os << indent << "Orientation: (" << this->Orientation[0] << ", " 
    << this->Orientation[1] << ", " << this->Orientation[2] << ")\n";
  os << indent << "Position: (" << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "ParallelProjection: " << 
    (this->ParallelProjection ? "On\n" : "Off\n");
  os << indent << "Parallel Scale: " << this->ParallelScale << "\n";
  os << indent << "Stereo: " << (this->Stereo ? "On\n" : "Off\n");
  os << indent << "Thickness: " << this->Thickness << "\n";
  os << indent << "View Angle: " << this->ViewAngle << "\n";
  os << indent << "View Plane Normal: (" << this->ViewPlaneNormal[0] << ", " 
    << this->ViewPlaneNormal[1] << ", " << this->ViewPlaneNormal[2] << ")\n";
  os << indent << "View Up: (" << this->ViewUp[0] << ", " 
    << this->ViewUp[1] << ", " << this->ViewUp[2] << ")\n";
  os << indent << "Window Center: (" << this->WindowCenter[0] << ", " 
    << this->WindowCenter[1] << ")\n";
}
