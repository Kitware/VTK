/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCamera.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
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
#include "vtkTimeStamp.h"
#include "vtkGraphicsFactory.h"

//----------------------------------------------------------------------------
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

  this->DirectionOfProjection[0] = 0.0;
  this->DirectionOfProjection[1] = 0.0;
  this->DirectionOfProjection[2] = 0.0;

  this->ViewAngle = 30.0;

  this->ClippingRange[0] = 0.01;
  this->ClippingRange[1] = 1000.01;
  this->Thickness = 1000.0;

  this->ParallelProjection = 0;
  this->ParallelScale = 1.0;
 
  this->EyeAngle = 2.0;
  this->Stereo = 0;
  this->LeftEye = 1;

  this->WindowCenter[0] = 0.0;
  this->WindowCenter[1] = 0.0;
  
  this->ViewShear[0] = 0.0;
  this->ViewShear[1] = 0.0;
  this->ViewShear[2] = 1.0;

  this->FocalDisk = 1.0;

  this->Transform = vtkPerspectiveTransform::New();
  this->ViewTransform = vtkTransform::New();
  this->PerspectiveTransform = vtkPerspectiveTransform::New();
  this->CameraLightTransform = vtkTransform::New();

  // initialize the ViewTransform
  this->ComputeViewTransform();
  this->ComputeDistance();
  this->ComputeCameraLightTransform();
}

//----------------------------------------------------------------------------
vtkCamera::~vtkCamera()
{
  this->Transform->Delete();
  this->ViewTransform->Delete();
  this->PerspectiveTransform->Delete();
  this->CameraLightTransform->Delete();
}

//----------------------------------------------------------------------------
// return the correct type of Camera 
vtkCamera *vtkCamera::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkCamera");
  return (vtkCamera*)ret;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// The first set of methods deal exclusively with the ViewTransform, which
// is the only transform which is set up entirely in the camera.  The
// perspective transform must be set up by the Renderer because the 
// Camera doesn't know the Renderer's aspect ratio.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkCamera::SetPosition(double x, double y, double z)
{
  if (x == this->Position[0] && 
      y == this->Position[1] &&
      z == this->Position[2])
    {
    return;
    }
  
  this->Position[0] = x;
  this->Position[1] = y;
  this->Position[2] = z;

  vtkDebugMacro(<< " Position set to ( " <<  this->Position[0] << ", " 
                << this->Position[1] << ", " << this->Position[2] << ")");

  this->ComputeViewTransform();
  // recompute the focal distance
  this->ComputeDistance();
  this->ComputeCameraLightTransform();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCamera::SetFocalPoint(double x, double y, double z)
{
  if (x == this->FocalPoint[0] && 
      y == this->FocalPoint[1] && 
      z == this->FocalPoint[2])
    {
    return;
    }

  this->FocalPoint[0] = x; 
  this->FocalPoint[1] = y; 
  this->FocalPoint[2] = z;

  vtkDebugMacro(<< " FocalPoint set to ( " <<  this->FocalPoint[0] << ", " << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")");

  this->ComputeViewTransform();
  // recompute the focal distance
  this->ComputeDistance();
  this->ComputeCameraLightTransform();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCamera::SetViewUp(double x, double y, double z)
{
  // normalize ViewUp, but do _not_ orthogonalize it by default
  double norm = sqrt(x*x + y*y + z*z);
  
  if(norm != 0) 
    {
    x /= norm; 
    y /= norm; 
    z /= norm;
    }
  else 
    {
    x = 0; 
    y = 1; 
    z = 0;
    }
  
  if (x == this->ViewUp[0] && 
      y == this->ViewUp[1] &&
      z == this->ViewUp[2])
    {
    return;
    }

  this->ViewUp[0] = x;
  this->ViewUp[1] = y;
  this->ViewUp[2] = z;

  vtkDebugMacro(<< " ViewUp set to ( " <<  this->ViewUp[0] << ", " << this->ViewUp[1] << ", " << this->ViewUp[2] << ")");
  
  this->ComputeViewTransform();
  this->ComputeCameraLightTransform();
  this->Modified();
}

//----------------------------------------------------------------------------
// The ViewTransform depends on only three ivars:  the Position, the
// FocalPoint, and the ViewUp vector.  All the other methods are there
// simply for the sake of the users' convenience.
void vtkCamera::ComputeViewTransform()
{
  // main view through the camera
  this->Transform->Identity();
  this->Transform->SetupCamera(this->Position, this->FocalPoint, this->ViewUp);
  this->ViewTransform->SetMatrix(this->Transform->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkCamera::ComputeCameraLightTransform()
{
  vtkTransform *t;
  double d;

  // assumes a valid view transform and valid camera distance

  t = this->CameraLightTransform;
  t->Identity();
  t->SetMatrix(this->ViewTransform->GetMatrix());
  t->Inverse();

  d = this->Distance;
  t->Scale(d, d, d);
  t->Translate(0.0, 0.0, -1.0);
}

//----------------------------------------------------------------------------
void vtkCamera::OrthogonalizeViewUp()
{
  // the orthogonalized ViewUp is just the second row of the view matrix
  vtkMatrix4x4 *matrix = this->ViewTransform->GetMatrix();
  this->ViewUp[0] = matrix->GetElement(1,0);
  this->ViewUp[1] = matrix->GetElement(1,1);
  this->ViewUp[2] = matrix->GetElement(1,2);

  this->Modified();
}

//----------------------------------------------------------------------------
// Set the distance of the focal point from the camera. The focal point is 
// modified accordingly. This should be positive.
void vtkCamera::SetDistance(double d)
{
  if (this->Distance == d)
    {
    return;
    }

  this->Distance = d; 

  // Distance should be greater than .0002
  if (this->Distance < 0.0002) 
    {
    this->Distance = 0.0002;
    vtkDebugMacro(<< " Distance is set to minimum.");
    }
  
  // we want to keep the camera pointing in the same direction
  double *vec = this->DirectionOfProjection;

  // recalculate FocalPoint
  this->FocalPoint[0] = this->Position[0] + vec[0]*this->Distance;
  this->FocalPoint[1] = this->Position[1] + vec[1]*this->Distance;
  this->FocalPoint[2] = this->Position[2] + vec[2]*this->Distance;

  vtkDebugMacro(<< " Distance set to ( " <<  this->Distance << ")");

  this->ComputeViewTransform();
  this->ComputeCameraLightTransform();
  this->Modified();
}  

//----------------------------------------------------------------------------
// This method must be called when the focal point or camera position changes
void vtkCamera::ComputeDistance()
{
  double dx = this->FocalPoint[0] - this->Position[0];
  double dy = this->FocalPoint[1] - this->Position[1];
  double dz = this->FocalPoint[2] - this->Position[2];

  this->Distance = sqrt(dx*dx + dy*dy + dz*dz);

  if (this->Distance < 0.0002) 
    {
    this->Distance = 0.0002;
    vtkDebugMacro(<< " Distance is set to minimum.");

    double *vec = this->DirectionOfProjection;

    // recalculate FocalPoint
    this->FocalPoint[0] = this->Position[0] + vec[0]*this->Distance;
    this->FocalPoint[1] = this->Position[1] + vec[1]*this->Distance;
    this->FocalPoint[2] = this->Position[2] + vec[2]*this->Distance;
    }

  this->DirectionOfProjection[0] = dx/this->Distance;
  this->DirectionOfProjection[1] = dy/this->Distance;
  this->DirectionOfProjection[2] = dz/this->Distance;

  this->ComputeViewPlaneNormal();
} 

//----------------------------------------------------------------------------
// Move the position of the camera along the view plane normal. Moving
// towards the focal point (e.g., > 1) is a dolly-in, moving away 
// from the focal point (e.g., < 1) is a dolly-out.
void vtkCamera::Dolly(double amount)
{
  if (amount <= 0.0)
    {
    return;
    }
  
  // dolly moves the camera towards the focus
  double d = this->Distance/amount;
  
  this->SetPosition(this->FocalPoint[0] - d*this->DirectionOfProjection[0],
		    this->FocalPoint[1] - d*this->DirectionOfProjection[1],
		    this->FocalPoint[2] - d*this->DirectionOfProjection[2]);
}

//----------------------------------------------------------------------------
// Set the roll angle of the camera about the direction of projection
void vtkCamera::SetRoll(double roll)
{
  // roll is a rotation of camera view up about the direction of projection
  vtkDebugMacro(<< " Setting Roll to " << roll << "");

  // subtract the current roll
  roll -= this->GetRoll();

  if (fabs(roll) < 0.00001)
    {
    return;
    }

  this->Roll(roll);
}

//----------------------------------------------------------------------------
// Returns the roll of the camera.
double vtkCamera::GetRoll()
{
  double orientation[3];
  this->ViewTransform->GetOrientation(orientation);
  return orientation[2];
}

//----------------------------------------------------------------------------
// Rotate the camera around the view plane normal.
void vtkCamera::Roll(double angle)
{
  double newViewUp[3];
  this->Transform->Identity();

  // rotate ViewUp about the Direction of Projection
  this->Transform->RotateWXYZ(angle,this->DirectionOfProjection);

  // okay, okay, TransformPoint shouldn't be used on vectors -- but
  // the transform is rotation with no translation so this works fine.
  this->Transform->TransformPoint(this->ViewUp,newViewUp);
  this->SetViewUp(newViewUp);
}

//----------------------------------------------------------------------------
// Rotate the focal point about the view up vector centered at the camera's 
// position. 
void vtkCamera::Yaw(double angle)
{
  double newFocalPoint[3];
  double *pos = this->Position;
  this->Transform->Identity();

  // translate the camera to the origin,
  // rotate about axis,
  // translate back again
  this->Transform->Translate(+pos[0],+pos[1],+pos[2]);   
  this->Transform->RotateWXYZ(angle,this->ViewUp);
  this->Transform->Translate(-pos[0],-pos[1],-pos[2]);
  
  // now transform focal point
  this->Transform->TransformPoint(this->FocalPoint,newFocalPoint);
  this->SetFocalPoint(newFocalPoint);
}

//----------------------------------------------------------------------------
// Rotate the focal point about the cross product of the view up vector 
// and the negative of the , centered at the camera's position.
void vtkCamera::Pitch(double angle)
{
  double axis[3], newFocalPoint[3];
  double *pos = this->Position;
  this->Transform->Identity();

  // the axis is the first row of the view transform matrix
  axis[0] = this->ViewTransform->GetMatrix()->GetElement(0,0);
  axis[1] = this->ViewTransform->GetMatrix()->GetElement(0,1);
  axis[2] = this->ViewTransform->GetMatrix()->GetElement(0,2);
  
  // translate the camera to the origin,
  // rotate about axis,
  // translate back again
  this->Transform->Translate(+pos[0],+pos[1],+pos[2]);   
  this->Transform->RotateWXYZ(angle,axis);
  this->Transform->Translate(-pos[0],-pos[1],-pos[2]);
  
  // now transform focal point
  this->Transform->TransformPoint(this->FocalPoint,newFocalPoint);
  this->SetFocalPoint(newFocalPoint);
}

//----------------------------------------------------------------------------
// Rotate the camera about the view up vector centered at the focal point.
void vtkCamera::Azimuth(double angle)
{
  double newPosition[3];
  double *fp = this->FocalPoint;
  this->Transform->Identity();

  // translate the focal point to the origin,
  // rotate about view up,
  // translate back again  
  this->Transform->Translate(+fp[0],+fp[1],+fp[2]);   
  this->Transform->RotateWXYZ(angle,this->ViewUp);
  this->Transform->Translate(-fp[0],-fp[1],-fp[2]);
  
  // apply the transform to the position
  this->Transform->TransformPoint(this->Position,newPosition);
  this->SetPosition(newPosition);
}

//----------------------------------------------------------------------------
// Rotate the camera about the cross product of the negative of the
// direction of projection and the view up vector centered on the focal point.
void vtkCamera::Elevation(double angle)
{
  double axis[3], newPosition[3];
  double *fp = this->FocalPoint;
  this->Transform->Identity();

  // snatch the axis from the view transform matrix
  axis[0] = -this->ViewTransform->GetMatrix()->GetElement(0,0);
  axis[1] = -this->ViewTransform->GetMatrix()->GetElement(0,1);
  axis[2] = -this->ViewTransform->GetMatrix()->GetElement(0,2);
  
  // translate the focal point to the origin,
  // rotate about axis,
  // translate back again
  this->Transform->Translate(+fp[0],+fp[1],+fp[2]);   
  this->Transform->RotateWXYZ(angle,axis);
  this->Transform->Translate(-fp[0],-fp[1],-fp[2]);
  
  // now transform position
  this->Transform->TransformPoint(this->Position,newPosition);
  this->SetPosition(newPosition);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// The following methods set up the information that the Renderer needs
// to set up the perspective transform.  The transformation matrix is
// created using the GetPerspectiveTransformMatrix method.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void vtkCamera::SetParallelProjection(int flag)
{
  if ( this->ParallelProjection != flag ) 
    {
    this->ParallelProjection = flag;
    this->Modified();
    this->ViewingRaysModified(); 
    } 
}

//----------------------------------------------------------------------------
void vtkCamera::SetViewAngle(double angle)
{
  double min =   1.0;
  double max = 179.0;

  if ( this->ViewAngle != angle )
    {
    this->ViewAngle = (angle<min?min:(angle>max?max:angle));
    this->Modified();
    this->ViewingRaysModified();
    }
}

//----------------------------------------------------------------------------
void vtkCamera::SetParallelScale(double scale)
{
  if ( this->ParallelScale != scale )
    {
    this->ParallelScale = scale;
    this->Modified();
    this->ViewingRaysModified();
    }
}

//----------------------------------------------------------------------------
// Change the ViewAngle (for perspective) or the ParallelScale (for parallel)
// so that more or less of a scene occupies the viewport.  A value > 1 is a 
// zoom-in. A value < 1 is a zoom-out.
void vtkCamera::Zoom(double amount)
{
  if (amount <= 0.0)
    {
    return;
    }
  
  if (this->ParallelProjection)
    {
    this->SetParallelScale(this->ParallelScale/amount);
    }
  else
    {
    this->SetViewAngle(this->ViewAngle/amount);
    }
}

//----------------------------------------------------------------------------
void vtkCamera::SetClippingRange(double nearz, double farz)
{
  double thickness;
  
  // check the order
  if ( nearz > farz )
    {
    vtkDebugMacro(<< " Front and back clipping range reversed");
    double temp = nearz;
    nearz = farz;
    farz = temp;
    }
  
  // front should be greater than 0.0001
  if (nearz < 0.0001) 
    {
    farz += 0.0001 - nearz;
    nearz = 0.0001;
    vtkDebugMacro(<< " Front clipping range is set to minimum.");
    }
  
  thickness = farz - nearz;
  
  // thickness should be greater than 0.0001
  if (thickness < 0.0001) 
    {
    thickness = 0.0001;
    vtkDebugMacro(<< " ClippingRange thickness is set to minimum.");
    
    // set back plane
    farz = nearz + thickness;
    }
  
  if (nearz == this->ClippingRange[0] && 
      farz == this->ClippingRange[1] && 
      this->Thickness == thickness)
    {
    return;
    }

  this->ClippingRange[0] = nearz; 
  this->ClippingRange[1] = farz; 
  this->Thickness = thickness;
  
  vtkDebugMacro(<< " ClippingRange set to ( " <<  this->ClippingRange[0] << ", "  << this->ClippingRange[1] << ")");

  this->Modified();
}  

//----------------------------------------------------------------------------
// Set the distance between clipping planes. 
// This method adjusts the back clipping plane to the specified thickness
// behind the front clipping plane 
void vtkCamera::SetThickness(double s)
{
  if (this->Thickness == s)
    {
    return;
    }

  this->Thickness = s; 

  // thickness should be greater than 0.0001
  if (this->Thickness < 0.0001) 
    {
    this->Thickness = 0.0001;
    vtkDebugMacro(<< " ClippingRange thickness is set to minimum.");
    }
  
  // set back plane
  this->ClippingRange[1] = this->ClippingRange[0] + this->Thickness;

  vtkDebugMacro(<< " ClippingRange set to ( " <<  this->ClippingRange[0] << ", " << this->ClippingRange[1] << ")");

  this->Modified();
}  

//----------------------------------------------------------------------------
void vtkCamera::SetWindowCenter(double x, double y)
{
  if (this->WindowCenter[0] != x || this->WindowCenter[1] != y)
    {
    this->Modified();
    this->ViewingRaysModified();
    this->WindowCenter[0] = x;
    this->WindowCenter[1] = y;
    }
}

//----------------------------------------------------------------------------
void vtkCamera::SetObliqueAngles(double alpha, double beta)
{
  alpha *= vtkMath::DoubleDegreesToRadians();
  beta *= vtkMath::DoubleDegreesToRadians();

  double cotbeta = cos(beta) / sin(beta);
  double dxdz = cos(alpha) * cotbeta;
  double dydz = sin(alpha) * cotbeta;

  this->SetViewShear(dxdz, dydz, 1.0);
}

//----------------------------------------------------------------------------
// Set the shear transform of the viewing frustum.  Parameters are
// dx/dz, dy/dz, and center.  center is a factor that describes where
// to shear around. The distance dshear from the camera where
// no shear occurs is given by (dshear = center * FocalDistance).
//
void vtkCamera::SetViewShear(double dxdz, double dydz, double center)
{
  if(dxdz != this->ViewShear[0] || 
     dydz != this->ViewShear[1] ||
     center != this->ViewShear[2])
    {
      this->Modified();
      this->ViewingRaysModified();

      this->ViewShear[0] = dxdz;
      this->ViewShear[1] = dydz;
      this->ViewShear[2] = center;
      
      this->ComputeViewPlaneNormal();
    }
}
//----------------------------------------------------------------------------

void vtkCamera::SetViewShear(double d[3]) 
{
  this->SetViewShear(d[0], d[1], d[2]);
}

//----------------------------------------------------------------------------
// Compute the perspective transform matrix. This is used in converting 
// between view and world coordinates.
void vtkCamera::ComputePerspectiveTransform(double aspect, 
					    double nearz, double farz)
{
  this->PerspectiveTransform->Identity();

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
  else
    {
    // set up a perspective frustum

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

  if (this->Stereo)
    {
    // set up a shear for stereo views

    if (this->LeftEye)
      {
      this->PerspectiveTransform->Stereo(-this->EyeAngle/2,
					 this->Distance);
      }
    else
      {
      this->PerspectiveTransform->Stereo(+this->EyeAngle/2,
					 this->Distance);
      }
    }

  if (this->ViewShear[0] != 0.0 || this->ViewShear[1] != 0.0) {
    this->PerspectiveTransform->Shear(this->ViewShear[0],
				      this->ViewShear[1],
				      this->ViewShear[2]*this->Distance);
  }

}

//----------------------------------------------------------------------------
// Return the perspective transform matrix. See ComputePerspectiveTransform.
vtkMatrix4x4 *vtkCamera::GetPerspectiveTransformMatrix(double aspect,
						       double nearz,
						       double farz)
{
  this->ComputePerspectiveTransform(aspect, nearz, farz);
  
  // return the transform 
  return this->PerspectiveTransform->GetMatrix();
}

//----------------------------------------------------------------------------
// Return the perspective transform matrix. See ComputePerspectiveTransform.
vtkMatrix4x4 *vtkCamera::GetCompositePerspectiveTransformMatrix(double aspect,
								double nearz,
								double farz)
{
  // turn off stereo, the CompositePerspectiveTransformMatrix is used for
  // picking, not for rendering.
  int stereo = this->Stereo;
  this->Stereo = 0;

  this->Transform->Identity();
  this->Transform->Concatenate(this->GetPerspectiveTransformMatrix(aspect,
								   nearz,
								   farz));
  this->Transform->Concatenate(this->GetViewTransformMatrix());

  this->Stereo = stereo;
  
  // return the transform 
  return this->Transform->GetMatrix();
}

//----------------------------------------------------------------------------
// Return the attached light transform matrix.
vtkMatrix4x4 *vtkCamera::GetCameraLightTransformMatrix()
{
  // return the transform 
  return this->CameraLightTransform->GetMatrix();
}


//----------------------------------------------------------------------------
void vtkCamera::ComputeViewPlaneNormal()
{
  if (this->ViewShear[0] != 0.0 || this->ViewShear[1] != 0.0)
    {
    // set the VPN in camera coordinates
    this->ViewPlaneNormal[0] = this->ViewShear[0];
    this->ViewPlaneNormal[1] = this->ViewShear[1];
    this->ViewPlaneNormal[2] = 1.0;
    // transform the VPN to world coordinates using inverse of view transform
    this->ViewTransform->GetLinearInverse()->TransformNormal(
					      this->ViewPlaneNormal,
					      this->ViewPlaneNormal);
    }
  else
    {
    // VPN is -DOP
    this->ViewPlaneNormal[0] = -this->DirectionOfProjection[0];
    this->ViewPlaneNormal[1] = -this->DirectionOfProjection[1];
    this->ViewPlaneNormal[2] = -this->DirectionOfProjection[2];
    }
}

//----------------------------------------------------------------------------
void vtkCamera::SetViewPlaneNormal(double vtkNotUsed(x), 
                                   double vtkNotUsed(y),
                                   double vtkNotUsed(z))
{
  vtkWarningMacro(<< "SetViewPlaneNormal:  This method is deprecated, the view plane normal is calculated automatically.");
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
unsigned long int vtkCamera::GetViewingRaysMTime()
{
  return this->ViewingRaysMTime.GetMTime();
}

//----------------------------------------------------------------------------
void vtkCamera::ViewingRaysModified()
{
  this->ViewingRaysMTime.Modified();
}

//----------------------------------------------------------------------------
void vtkCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "ClippingRange: (" << this->ClippingRange[0] << ", " 
     << this->ClippingRange[1] << ")\n";
  os << indent << "DirectionOfProjection: (" << this->DirectionOfProjection[0]
     << ", " << this->DirectionOfProjection[1] 
     << ", " << this->DirectionOfProjection[2] << ")\n";
  os << indent << "Distance: " << this->Distance << "\n";
  os << indent << "EyeAngle: " << this->EyeAngle << "\n";
  os << indent << "FocalDisk: " << this->FocalDisk << "\n";
  os << indent << "FocalPoint: (" << this->FocalPoint[0] << ", " 
     << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")\n";
  os << indent << "ViewShear: (" << this->ViewShear[0]
     << ", " << this->ViewShear[1] 
     << ", " << this->ViewShear[2] << ")\n";
  os << indent << "ParallelProjection: " << 
    (this->ParallelProjection ? "On\n" : "Off\n");
  os << indent << "ParallelScale: " << this->ParallelScale << "\n";
  os << indent << "Position: (" << this->Position[0] << ", " 
     << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Stereo: " << (this->Stereo ? "On\n" : "Off\n");
  os << indent << "Thickness: " << this->Thickness << "\n";
  os << indent << "ViewAngle: " << this->ViewAngle << "\n";
  os << indent << "ViewPlaneNormal: (" << this->ViewPlaneNormal[0]
     << ", " << this->ViewPlaneNormal[1] 
     << ", " << this->ViewPlaneNormal[2] << ")\n";
  os << indent << "ViewUp: (" << this->ViewUp[0] << ", " 
     << this->ViewUp[1] << ", " << this->ViewUp[2] << ")\n";
  os << indent << "WindowCenter: (" << this->WindowCenter[0] << ", " 
     << this->WindowCenter[1] << ")\n";
}
