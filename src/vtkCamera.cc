/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCamera.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include "vtkCamera.hh"
#include "vtkMath.hh"
#include "vtkRenderer.hh"
#include "vtkRenderWindow.hh"
#include "vtkCameraDevice.hh"

// Description:
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
  this->Device = NULL;
  this->Stereo = 0;
}

vtkCamera::~vtkCamera()
{
  if (this->Device)
    {
    this->Device->Delete();
    }
}

void vtkCamera::Render(vtkRenderer *ren)
{
  if (!this->Device)
    {
    this->Device = ren->GetRenderWindow()->MakeCamera();
    }
  
  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();
  
  this->Device->Render(this,ren);
  
  // if we have a stereo renderer, draw other eye next time 
  if (this->Stereo)
    {
    if (this->LeftEye) this->LeftEye = 0;
    else this->LeftEye = 1;
    }
}

void vtkCamera::SetPosition(float X, float Y, float Z)
{
  this->Position[0] = X;
  this->Position[1] = Y;
  this->Position[2] = Z;

  vtkDebugMacro(<< " Position set to ( " <<  this->Position[0] << ", "
  << this->Position[1] << ", " << this->Position[2] << ")");

  // recalculate distance
  this->CalcDistance();
  
  this->Modified();
}

void vtkCamera::SetPosition(float a[3])
{
  this->SetPosition(a[0],a[1],a[2]);
}

void vtkCamera::SetFocalPoint(float X, float Y, float Z)
{
  this->FocalPoint[0] = X; 
  this->FocalPoint[1] = Y; 
  this->FocalPoint[2] = Z;

  vtkDebugMacro(<< " FocalPoint set to ( " <<  this->FocalPoint[0] << ", "
  << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")");

  // recalculate distance
  this->CalcDistance();
  
  this->Modified();
}

void vtkCamera::SetFocalPoint(float a[3])
{
  this->SetFocalPoint(a[0],a[1],a[2]);
}

void vtkCamera::SetViewUp(float X, float Y, float Z)
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
  
  vtkDebugMacro(<< " ViewUp set to ( " <<  this->ViewUp[0] << ", "
  << this->ViewUp[1] << ", " << this->ViewUp[2] << ")");
  
  this->Modified();
}

void vtkCamera::SetViewUp(float a[3])
{
  this->SetViewUp(a[0],a[1],a[2]);
}

void vtkCamera::SetClippingRange(float X, float Y)
{
  this->ClippingRange[0] = X; 
  this->ClippingRange[1] = Y; 

  // check the order
  if(this->ClippingRange[0] > this->ClippingRange[1]) 
    {
    float temp;
    vtkDebugMacro(<< " Front and back clipping range reversed");
    temp = this->ClippingRange[0];
    this->ClippingRange[0] = this->ClippingRange[1];
    this->ClippingRange[1] = temp;
    }
  
  // front should be greater than 0.0001
  if (this->ClippingRange[0] < 0.0001) 
    {
    this->ClippingRange[1] += 0.0001 - this->ClippingRange[0];
    this->ClippingRange[0] = 0.0001;
    vtkDebugMacro(<< " Front clipping range is set to minimum.");
    }
  
  this->Thickness = this->ClippingRange[1] - this->ClippingRange[0];
  
  // thickness should be greater than 0.0001
  if (this->Thickness < 0.0001) 
    {
    this->Thickness = 0.0001;
    vtkDebugMacro(<< " ClippingRange thickness is set to minimum.");
    
    // set back plane
    this->ClippingRange[1] = this->ClippingRange[0] + this->Thickness;
    }
  
  vtkDebugMacro(<< " ClippingRange set to ( " <<  this->ClippingRange[0] << ", "
  << this->ClippingRange[1] << ")");

  this->Modified();
}  

void vtkCamera::SetClippingRange(float a[2])
{
  this->SetClippingRange(a[0],a[1]);
}

// Description:
// Set the distance between clipping planes. A side effect of this method is
// to adjust the back clipping plane to be equal to the front clipping plane 
// plus the thickness.
void vtkCamera::SetThickness(float X)
{
  if (this->Thickness == X) return;

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

// Description:
// Set the distance of the focal point from the camera. The focal point is 
// modified accordingly. This should be positive.
void vtkCamera::SetDistance(float X)
{
  if (this->Distance == X) return;

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

// Description:
// Compute the view plane normal from the position and focal point.
void vtkCamera::CalcViewPlaneNormal()
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
    vpn[0] = dx / distance;
    vpn[1] = dy / distance;
    vpn[2] = dz / distance;
    }
  
  vtkDebugMacro(<< "Calculating ViewPlaneNormal of (" << vpn[0] << " " << vpn[1] << " " << vpn[2] << ")");
}


// Description:
// Set the roll angle of the camera about the view plane normal.
void vtkCamera::SetRoll(float roll)
{
  float current;
  float temp[4];

  // roll is a rotation of camera view up about view plane normal
  vtkDebugMacro(<< " Setting Roll to " << roll << "");

  // get the current roll
  current = this->GetRoll();

  roll -= current;

  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PreMultiply();

  // rotate about view plane normal
  this->Transform.RotateWXYZ(-1.0*roll,this->ViewPlaneNormal[0],
			     this->ViewPlaneNormal[1],
			     this->ViewPlaneNormal[2]);
  
  // now transform view up
  temp[0] = this->ViewUp[0];
  temp[1] = this->ViewUp[1];
  temp[2] = this->ViewUp[2];
  temp[3] = 1.0;
  this->Transform.PointMultiply(temp,temp);
  
  // now store the result
  this->SetViewUp((float *)temp);

  this->Transform.Pop();
}

// Description:
// Returns the roll of the camera.
float vtkCamera::GetRoll()
{
  float	*orient;
  
  // set roll using orientation
  orient = this->GetOrientation();

  vtkDebugMacro(<< " Returning Roll of " << orient[2] << "");

  return orient[2];
}

// Description:
// Compute the camera distance, which is the distance between the 
// focal point and position.
void vtkCamera::CalcDistance ()
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

// Description:
// Returns the orientation of the camera. This is a vector of X,Y and Z 
// rotations that when performed in the order RotateZ, RotateX, and finally
// RotateY, will yield the same 3x3 rotation matrix for the camera.
float *vtkCamera::GetOrientation ()
{
  // calculate a new orientation
//  this->CalcPerspectiveTransform(1,0,1);
  this->CalcViewTransform();
  this->PerspectiveTransform.GetOrientation (this->Orientation[0],
					     this->Orientation[1],
					     this->Orientation[2]);

  vtkDebugMacro(<< " Returning Orientation of ( " <<  this->Orientation[0] 
  << ", " << this->Orientation[1] << ", " << this->Orientation[2] << ")");
  
  return this->Orientation;
}

// Description:
// Compute the view transform matrix. This is used in converting 
// between view and world coordinates. It does not include any 
// perspective effects but it does include shearing and scaling.
void vtkCamera::CalcViewTransform()
{
  vtkMatrix4x4  matrix;
  float *Rz, Rx[3], Ry[3];
  float p1[4],PRP[4];
  
  this->PerspectiveTransform.PostMultiply();  
  this->PerspectiveTransform.Identity();

  // translate to VRP
  this->PerspectiveTransform.Translate(-this->FocalPoint[0],
				       -this->FocalPoint[1],
				       -this->FocalPoint[2]);
  
  // do the rotation
  // Rz just equals the VPN
  Rz = this->ViewPlaneNormal;
  vtkMath::Cross(this->ViewUp,Rz,Rx);
  vtkMath::Normalize(Rx);
  vtkMath::Cross(Rz,Rx,Ry);
  
  matrix[0][0] = Rx[0];
  matrix[0][1] = Rx[1];
  matrix[0][2] = Rx[2];
  matrix[1][0] = Ry[0];
  matrix[1][1] = Ry[1];
  matrix[1][2] = Ry[2];
  matrix[2][0] = Rz[0];
  matrix[2][1] = Rz[1];
  matrix[2][2] = Rz[2];
  
  this->PerspectiveTransform.Concatenate(matrix);

  // translate to projection reference point PRP
  // this is the camera's position blasted through
  // the current matrix
  p1[0] = this->Position[0];
  p1[1] = this->Position[1];
  p1[2] = this->Position[2];
  p1[3] = 1.0;
  this->PerspectiveTransform.MultiplyPoint(p1,PRP);
  
  // dist is really VRP'n so it will be negative
  PRP[0] /= PRP[3];
  PRP[1] /= PRP[3];
  PRP[2] /= PRP[3];

  // also need to take into account stereo rendering
  if (this->Stereo)
    {
    if (this->LeftEye)
      {
      PRP[0] -= PRP[2]*tan(this->EyeAngle*3.1415926/360.0);
      }
    else
      {
      PRP[0] += PRP[2]*tan(this->EyeAngle*3.1415926/360.0);
      }
    }
  
  this->PerspectiveTransform.Translate(-PRP[0],-PRP[1],-PRP[2]);
}

// Description:
// Compute the perspective transform matrix. This is used in converting 
// between view and world coordinates.
void vtkCamera::CalcPerspectiveTransform(float aspect, 
					 float nearz, float farz)
{
  vtkMatrix4x4  matrix;
  float DOP[3];
  float ftemp;
  float *Rz, Rx[3], Ry[3];
  float p1[4],PRP[4];
  
  this->PerspectiveTransform.Push();  
  this->PerspectiveTransform.PostMultiply();  
  this->PerspectiveTransform.Identity();

  // translate to VRP
  this->PerspectiveTransform.Translate(-this->FocalPoint[0],
				       -this->FocalPoint[1],
				       -this->FocalPoint[2]);
  
  // do the rotation
  // Rz just equals the VPN
  Rz = this->ViewPlaneNormal;
  vtkMath::Cross(this->ViewUp,Rz,Rx);
  vtkMath::Normalize(Rx);
  vtkMath::Cross(Rz,Rx,Ry);
  
  matrix[0][0] = Rx[0];
  matrix[0][1] = Rx[1];
  matrix[0][2] = Rx[2];
  matrix[1][0] = Ry[0];
  matrix[1][1] = Ry[1];
  matrix[1][2] = Ry[2];
  matrix[2][0] = Rz[0];
  matrix[2][1] = Rz[1];
  matrix[2][2] = Rz[2];
  
  this->PerspectiveTransform.Concatenate(matrix);

  // translate to projection reference point PRP
  // this is the camera's position blasted through
  // the current matrix
  p1[0] = this->Position[0];
  p1[1] = this->Position[1];
  p1[2] = this->Position[2];
  p1[3] = 1.0;
  this->PerspectiveTransform.MultiplyPoint(p1,PRP);
  
  // dist is really VRP'n so it will be negative
  PRP[0] /= PRP[3];
  PRP[1] /= PRP[3];
  PRP[2] /= PRP[3];

  // also need to take into account stereo rendering
  if (this->Stereo)
    {
    if (this->LeftEye)
      {
      PRP[0] -= PRP[2]*tan(this->EyeAngle*3.1415926/360.0);
      }
    else
      {
      PRP[0] += PRP[2]*tan(this->EyeAngle*3.1415926/360.0);
      }
    }
  
  this->PerspectiveTransform.Translate(-PRP[0],-PRP[1],-PRP[2]);
  
  // restore the original matrix
  this->PerspectiveTransform.Pop();
  
  // now do the shear to get the z axis to go through the
  // center of the window
  ftemp = PRP[2]*tan(this->ViewAngle*aspect*3.1415926/360.0);
  DOP[0] = ftemp*this->WindowCenter[0] - PRP[0];
  ftemp = PRP[2]*tan(this->ViewAngle*3.1415926/360.0);
  DOP[1] = ftemp*this->WindowCenter[1] - PRP[1];
  DOP[2] = - PRP[2];
  
  matrix[0][0] = 1;
  matrix[0][1] = 0;
  matrix[0][2] = -DOP[0]/DOP[2];
  matrix[0][3] = 0;
  matrix[1][0] = 0;
  matrix[1][1] = 1;
  matrix[1][2] = -DOP[1]/DOP[2];
  matrix[1][3] = 0;
  matrix[2][0] = 0;
  matrix[2][1] = 0;
  matrix[2][2] = 1;
  matrix[2][3] = 0;
  matrix[3][0] = 0;
  matrix[3][1] = 0;
  matrix[3][2] = 0;
  matrix[3][3] = 1;
  
  this->PerspectiveTransform.Concatenate(matrix);

  if (this->ParallelProjection)
    {
    // now scale according to page 269 Foley & VanDam 2nd Edition
    this->PerspectiveTransform.Scale(1.0/(this->ParallelScale*aspect),
				     1.0/this->ParallelScale,
				     1.0/(this->ClippingRange[1]-
					  this->ClippingRange[0]));
    }
  else
    {
    // now scale according to page 269 Foley & VanDam 2nd Edition
    this->PerspectiveTransform.Scale(1.0/(tan(this->ViewAngle*3.1415926/360.0)*
					  this->ClippingRange[1]*aspect),
				     1.0/(tan(this->ViewAngle*3.1415926/360.0)*
					  this->ClippingRange[1]),
				     1.0/this->ClippingRange[1]);
    }
  
  // now set the orientation
  Rz = this->PerspectiveTransform.GetOrientation();
  this->Orientation[0] = Rz[0];
  this->Orientation[1] = Rz[1];
  this->Orientation[2] = Rz[2];

  if (this->ParallelProjection)
    {
    matrix[0][2] = 0;
    matrix[1][2] = 0;
    matrix[2][2] = (nearz - farz);
    matrix[2][3] = nearz;
    matrix[3][2] = 0;
    matrix[3][3] = 1;
    }
  else
    {
    ftemp = this->ClippingRange[0]/this->ClippingRange[1];
    matrix[0][2] = 0;
    matrix[1][2] = 0;
    matrix[2][2] = (nearz - farz)/(1 - ftemp) - nearz;
    matrix[2][3] = (nearz - farz)*ftemp/(1 - ftemp);
    matrix[3][2] = -1;
    matrix[3][3] = 0;
    }
  
  this->PerspectiveTransform.Concatenate(matrix);
}


// Description:
// Return the perspective transform matrix. See CalcPerspectiveTransform.
vtkMatrix4x4 &vtkCamera::GetPerspectiveTransform(float aspect,
						 float nearz, float farz)
{
  // update transform 
  this->PerspectiveTransform.PostMultiply();  
  this->PerspectiveTransform.Identity();
  this->CalcPerspectiveTransform(aspect, nearz,farz);
  
  // return the transform 
  return this->PerspectiveTransform.GetMatrix();
}

// Description:
// Return the perspective transform matrix. See CalcPerspectiveTransform.
vtkMatrix4x4 &vtkCamera::GetViewTransform()
{
  // update transform 
  this->CalcViewTransform();
  
  // return the transform 
  return this->PerspectiveTransform.GetMatrix();
}

// Description:
// Return the perspective transform matrix. See CalcPerspectiveTransform.
vtkMatrix4x4 &vtkCamera::GetCompositePerspectiveTransform(float aspect,
							  float nearz,
							  float farz)
{
  // update transform 
  this->CalcViewTransform();
  this->CalcPerspectiveTransform(aspect, nearz,farz);
  
  // return the transform 
  return this->PerspectiveTransform.GetMatrix();
}

#define SQ_MAG(x) ( (x)[0]*(x)[0] + (x)[1]*(x)[1] + (x)[2]*(x)[2] )

// Description:
// Recompute the view up vector so that it is perpendicular to the
// view plane normal.
void vtkCamera::OrthogonalizeViewUp()
{
  float *normal,*up,temp[3],new_up[3];
  float temp_mag_sq,new_mag_sq,ratio;

  normal=this->ViewPlaneNormal;
  up=this->ViewUp;
  vtkMath::Cross(normal,up,temp);

  temp_mag_sq = (SQ_MAG(up));
  vtkMath::Cross(temp,normal,new_up);
  new_mag_sq = (SQ_MAG(new_up));
  ratio = sqrt(new_mag_sq/temp_mag_sq);
  this->SetViewUp(new_up[0]*ratio,new_up[1]*ratio,new_up[2]*ratio);
}

// Description:
// Move the position of the camera along the view plane normal. Moving
// towards the focal point (e.g., > 1) is a dolly-in, moving away 
// from the focal point (e.g., < 1) is a dolly-out.
void vtkCamera::Dolly(float amount)
{
  float	distance;
  
  if (amount <= 0.0) return;
  
  // zoom moves position along view plane normal by a specified ratio
  distance =  this->Distance / amount;
  
  this->SetPosition(this->FocalPoint[0] + distance * this->ViewPlaneNormal[0],
		    this->FocalPoint[1] + distance * this->ViewPlaneNormal[1],
		    this->FocalPoint[2] + distance * this->ViewPlaneNormal[2]);
}

// Description:
// Change the ViewAngle of the camera so that more or less of a scene 
// occupies the viewport. A value > 1 is a zoom-in. A value < 1 is a zoom-out.
void vtkCamera::Zoom(float amount)
{
  if (amount <= 0.0) return;
  
  this->ViewAngle = this->ViewAngle/amount;
}


// Description:
// Rotate the camera about the view up vector centered at the focal point.
void vtkCamera::Azimuth (float angle)
{
  // azimuth is a rotation of camera position about view up vector
  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PostMultiply();
  
  // translate to focal point
  this->Transform.Translate(-this->FocalPoint[0],
			    -this->FocalPoint[1],
			    -this->FocalPoint[2]);
   
  // rotate about view up
  this->Transform.RotateWXYZ(angle,this->ViewUp[0],this->ViewUp[1],
			     this->ViewUp[2]);
  
  // translate to focal point
  this->Transform.Translate(this->FocalPoint[0],
			    this->FocalPoint[1],
			    this->FocalPoint[2]);
   

  // now transform position
  this->Transform.SetPoint(this->Position[0],this->Position[1],
			    this->Position[2],1.0);
  
  // now store the result
  this->SetPosition(this->Transform.GetPoint());

  // also azimuth the vpn
  this->Transform.Identity();
  this->Transform.RotateWXYZ(angle,this->ViewUp[0],this->ViewUp[1],
			     this->ViewUp[2]);
  this->Transform.SetPoint(this->ViewPlaneNormal[0],this->ViewPlaneNormal[1],
			   this->ViewPlaneNormal[2],1.0);
  this->SetViewPlaneNormal(this->Transform.GetPoint());
  
  this->Transform.Pop();
}

// Description:
// Rotate the camera about the cross product of the view plane normal and 
// the view up vector centered on the focal point.
void vtkCamera::Elevation (float angle)
{
  double	axis[3];
  
  // elevation is a rotation of camera position about cross between
  // view plane normal and view up
  axis[0] = (this->ViewPlaneNormal[1] * this->ViewUp[2] -
	     this->ViewPlaneNormal[2] * this->ViewUp[1]);
  axis[1] = (this->ViewPlaneNormal[2] * this->ViewUp[0] -
	     this->ViewPlaneNormal[0] * this->ViewUp[2]);
  axis[2] = (this->ViewPlaneNormal[0] * this->ViewUp[1] -
	     this->ViewPlaneNormal[1] * this->ViewUp[0]);

  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PostMultiply();
  
  // translate to focal point
  this->Transform.Translate(-this->FocalPoint[0],
			    -this->FocalPoint[1],
			    -this->FocalPoint[2]);
   
  // rotate about view up
  this->Transform.RotateWXYZ(angle,axis[0],axis[1],axis[2]);
  
  
  // translate to focal point
  this->Transform.Translate(this->FocalPoint[0],
			    this->FocalPoint[1],
			    this->FocalPoint[2]);
   
  // now transform position
  this->Transform.SetPoint(this->Position[0],this->Position[1],
			    this->Position[2],1.0);

  // now store the result
  this->SetPosition(this->Transform.GetPoint());

  // also elevate the vpn
  this->Transform.Identity();
  this->Transform.RotateWXYZ(angle,axis[0],axis[1],axis[2]);
  this->Transform.SetPoint(this->ViewPlaneNormal[0],this->ViewPlaneNormal[1],
			   this->ViewPlaneNormal[2],1.0);
  this->SetViewPlaneNormal(this->Transform.GetPoint());
  
  
  this->Transform.Pop();
}

// Description:
// Rotate the focal point about the view up vector centered at the camera's 
// position. 
void vtkCamera::Yaw (float angle)
{
  // yaw is a rotation of camera focal_point about view up vector
  this->Transform.Push();
  this->Transform.Identity();
  this->Transform.PostMultiply();
  
  // translate to position
  this->Transform.Translate(-this->Position[0],
			    -this->Position[1],
			    -this->Position[2]);

  // rotate about view up
  this->Transform.RotateWXYZ(angle,this->ViewUp[0],this->ViewUp[1],
			     this->ViewUp[2]);
  
  // translate to position
  this->Transform.Translate(this->Position[0],
			    this->Position[1],
			    this->Position[2]);

  // now transform focal point
  this->Transform.SetPoint(this->FocalPoint[0],this->FocalPoint[1],
			    this->FocalPoint[2],1.0);
  
  // now store the result
  this->SetFocalPoint(this->Transform.GetPoint());

  // also yaw the vpn
  this->Transform.Identity();
  this->Transform.RotateWXYZ(angle,this->ViewUp[0],this->ViewUp[1],
			     this->ViewUp[2]);
  this->Transform.SetPoint(this->ViewPlaneNormal[0],this->ViewPlaneNormal[1],
			   this->ViewPlaneNormal[2],1.0);
  this->SetViewPlaneNormal(this->Transform.GetPoint());
  
  this->Transform.Pop();
}

// Description:
// Rotate the focal point about the cross product of the view up vector 
// and the view plane normal, centered at the camera's position.
void vtkCamera::Pitch (float angle)
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
  this->Transform.PostMultiply();
  
  // translate to position
  this->Transform.Translate(-this->Position[0],
			    -this->Position[1],
			    -this->Position[2]);

  // rotate about view up
  this->Transform.RotateWXYZ(angle,axis[0],axis[1],axis[2]);
  
   
  // translate to position
  this->Transform.Translate(this->Position[0],
			    this->Position[1],
			    this->Position[2]);

  // now transform focal point
  this->Transform.SetPoint(this->FocalPoint[0],this->FocalPoint[1],
			    this->FocalPoint[2],1.0);
  
  // now store the result
  this->SetFocalPoint(this->Transform.GetPoint());

  // also pitch the vpn
  this->Transform.Identity();
  this->Transform.RotateWXYZ(angle,axis[0],axis[1],axis[2]);
  this->Transform.SetPoint(this->ViewPlaneNormal[0],this->ViewPlaneNormal[1],
			   this->ViewPlaneNormal[2],1.0);
  this->SetViewPlaneNormal(this->Transform.GetPoint());
  
  this->Transform.Pop();
}

// Description:
// Rotate the camera around the view plane normal.
void vtkCamera::Roll (float angle)
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
  this->Transform.SetPoint(this->ViewUp[0],this->ViewUp[1],
			    this->ViewUp[2],1.0);
  
  // now store the result
  this->SetViewUp(this->Transform.GetPoint());

  this->Transform.Pop();
}

// Description:
// Set the direction that the camera points.
// Adjusts position to be consistent with the view plane normal.
void vtkCamera::SetViewPlaneNormal(float X,float Y,float Z)
{
  float norm;

  // normalize it
  norm = sqrt( X * X + Y * Y + Z * Z );
  if (!norm)
    {
    vtkErrorMacro(<< "SetViewPlaneNormal of (0,0,0)");
    return;
    }
  
  this->ViewPlaneNormal[0] = X/norm;
  this->ViewPlaneNormal[1] = Y/norm;
  this->ViewPlaneNormal[2] = Z/norm;

  vtkDebugMacro(<< " ViewPlaneNormal set to ( " << X/norm << ", "
  << Y/norm << ", " << Z/norm << ")");
  
  this->Modified();
}

void vtkCamera::SetViewPlaneNormal(float a[3])
{
  this->SetViewPlaneNormal(a[0],a[1],a[2]);
}

void vtkCamera::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Thickness: " << this->Thickness << "\n";
  os << indent << "View Angle: " << this->ViewAngle << "\n";
  os << indent << "View Plane Normal: (" << this->ViewPlaneNormal[0] << ", " 
    << this->ViewPlaneNormal[1] << ", " << this->ViewPlaneNormal[2] << ")\n";
  os << indent << "View Up: (" << this->ViewUp[0] << ", " 
    << this->ViewUp[1] << ", " << this->ViewUp[2] << ")\n";
}
