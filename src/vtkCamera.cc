/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCamera.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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

  this->Switch = 1;
  this->LeftEye = 1;
  this->EyeAngle = 2.0;

  this->Thickness = 1000.0;
  this->Distance = 1.0;

  this->ViewPlaneNormal[0] = 0.0;
  this->ViewPlaneNormal[1] = 0.0;
  this->ViewPlaneNormal[2] = -1.0;

  this->Orientation[0] = 0.0;
  this->Orientation[1] = 0.0;
  this->Orientation[2] = 0.0;
  
  this->FocalDisk = 1.0;
  this->Device = NULL;
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
  this->Device->Render(this,ren);
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
  
  // recalculate view plane normal
  this->CalcViewPlaneNormal();
  
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
  
  // recalculate view plane normal
  this->CalcViewPlaneNormal();
  
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
  
  // front should be greater than 0.001
  if (this->ClippingRange[0] < 0.001) 
    {
    this->ClippingRange[1] += 0.001 - this->ClippingRange[0];
    this->ClippingRange[0] = 0.001;
    vtkDebugMacro(<< " Front clipping range is set to minimum.");
    }
  
  this->Thickness = this->ClippingRange[1] - this->ClippingRange[0];
  
  // thickness should be greater than THICKNESS_MIN
  if (this->Thickness < 0.002) 
    {
    this->Thickness = 0.002;
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
// adjust the back clipping plane to be equal to the front clipping plane 
// plus the thickness.
void vtkCamera::SetThickness(float X)
{
  if (this->Thickness == X) return;

  this->Thickness = X; 

  // thickness should be greater than THICKNESS_MIN
  if (this->Thickness < 0.002) 
    {
    this->Thickness = 0.002;
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

  // Distance should be greater than .002
  if (this->Distance < 0.002) 
    {
    this->Distance = 0.002;
    vtkDebugMacro(<< " Distance is set to minimum.");
    }
  
  // recalculate FocalPoint
  this->FocalPoint[0] = this->ViewPlaneNormal[0] * 
    this->Distance + this->Position[0];
  this->FocalPoint[1] = this->ViewPlaneNormal[1] * 
    this->Distance + this->Position[1];
  this->FocalPoint[2] = this->ViewPlaneNormal[2] * 
    this->Distance + this->Position[2];

  vtkDebugMacro(<< " Distance set to ( " <<  this->Distance << ")");

  this->Modified();
}  

// Description:
// This returns the twist of the camera.  The twist corrisponds to Roll and
// represents the angle of rotation about the z axis to achieve the 
// current view-up vector.
float vtkCamera::GetTwist()
{
  float *vup, *vn;
  float twist = 0;
  float v1[3], v2[3], y_axis[3];
  double the
