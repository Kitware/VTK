/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackball.cxx
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
#include "vtkInteractorStyleTrackball.h"
#include "vtkMath.h"
#include "vtkPropPicker.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

//---------------------------------------------------------------------------
vtkInteractorStyleTrackball* vtkInteractorStyleTrackball::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInteractorStyleTrackball");
  if(ret)
    {
    return (vtkInteractorStyleTrackball*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleTrackball;
}

vtkInteractorStyleTrackball::vtkInteractorStyleTrackball()
{
  // for actor interactions
  this->MotionFactor = 10.0;
  this->InteractionPicker = vtkPropPicker::New();
  this->PropPicked = 0;
  this->InteractionProp = NULL;

  // set to default modes
  this->TrackballMode = VTKIS_JOY;
  this->ActorMode = VTKIS_CAMERA;
  this->ControlMode = VTKIS_CONTROL_OFF;
  this->OldX = 0.0;
  this->OldY = 0.0;
  this->Preprocess = 1;
  this->RadianToDegree = 180.0 / vtkMath::Pi();

  this->NewPickPoint[0] = 0.0;
  this->NewPickPoint[1] = 0.0;
  this->NewPickPoint[2] = 0.0;
  this->NewPickPoint[3] = 1.0;
  this->OldPickPoint[0] = 0.0;
  this->OldPickPoint[1] = 0.0;
  this->OldPickPoint[2] = 0.0;
  this->OldPickPoint[3] = 1.0;
  this->MotionVector[0] = 0.0;
  this->MotionVector[1] = 0.0;
  this->MotionVector[2] = 0.0;
  this->ViewLook[0] = 0.0;
  this->ViewLook[1] = 0.0;
  this->ViewLook[2] = 0.0;
  this->ViewPoint[0] = 0.0;
  this->ViewPoint[1] = 0.0;
  this->ViewPoint[2] = 0.0;
  this->ViewFocus[0] = 0.0;
  this->ViewFocus[1] = 0.0;
  this->ViewFocus[2] = 0.0;
  this->ViewUp[0] = 0.0;
  this->ViewUp[1] = 0.0;
  this->ViewUp[2] = 0.0;
  this->ViewRight[0] = 0.0;
  this->ViewRight[1] = 0.0;
  this->ViewRight[2] = 0.0;  

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 0.0;
  this->ObjCenter[0] = 0.0;
  this->ObjCenter[1] = 0.0;
  this->ObjCenter[2] = 0.0;  
  this->DispObjCenter[0] = 0.0;
  this->DispObjCenter[1] = 0.0;
  this->DispObjCenter[2] = 0.0;
  this->Radius = 0.0;
}

vtkInteractorStyleTrackball::~vtkInteractorStyleTrackball() 
{
  this->InteractionPicker->Delete();
}

//----------------------------------------------------------------------------
// Implementations of Joystick Camera/Actor motions follow
//----------------------------------------------------------------------------
// Description:
// rotate the camera in trackball (motion sensitive) style
void vtkInteractorStyleTrackball::TrackballRotateCamera(int x, int y)
{
  if ((this->OldX != x) || (this->OldY != y))
    {
    double rxf = (double)(x - this->OldX) * this->DeltaAzimuth *
      this->MotionFactor;
    double ryf = (double)(y - this->OldY) * this->DeltaElevation *
      this->MotionFactor;
    
    this->CurrentCamera->Azimuth(rxf);
    this->CurrentCamera->Elevation(ryf);
    this->CurrentCamera->OrthogonalizeViewUp();
    this->CurrentRenderer->ResetCameraClippingRange();
    vtkRenderWindowInteractor *rwi = this->Interactor;
    if (rwi->GetLightFollowCamera())
      {
      // get the first light
      this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
      this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
      } 
    this->OldX = x;
    this->OldY = y;
    rwi->Render();
    }
}

// Description:
// spin the camera in trackball (motion sensitive) style
void vtkInteractorStyleTrackball::TrackballSpinCamera(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if ((this->OldX != x) || (this->OldY != y))
    {
    double newAngle = atan2((double)(y - this->Center[1]),
                           (double)(x - this->Center[0]));
    double oldAngle = atan2((double)(this->OldY -this->Center[1]),
                           (double)(this->OldX - this->Center[0]));
  
    newAngle *= this->RadianToDegree;
    oldAngle *= this->RadianToDegree;

    this->CurrentCamera->Roll(newAngle - oldAngle);
    this->CurrentCamera->OrthogonalizeViewUp();
      
    this->OldX = x;
    this->OldY = y;
    rwi->Render();
    }
}


// Description:
// pan the camera in trackball (motion sensitive) style
void vtkInteractorStyleTrackball::TrackballPanCamera(int x, int y)
{
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      // calculate the focal depth since we'll be using it a lot
      this->CurrentCamera->GetFocalPoint(this->ViewFocus);
      this->ComputeWorldToDisplay(this->ViewFocus[0], this->ViewFocus[1],
                                  this->ViewFocus[2], this->ViewFocus);
      this->FocalDepth = this->ViewFocus[2];

      this->Preprocess = 0;
      }

    this->ComputeDisplayToWorld(double(x), double(y),
                                this->FocalDepth,
                                this->NewPickPoint);
    
    // has to recalc old mouse point since the viewport has moved,
    // so can't move it outside the loop
    this->ComputeDisplayToWorld(double(this->OldX),double(this->OldY),
                                this->FocalDepth, this->OldPickPoint);

    // camera motion is reversed
    this->MotionVector[0] = this->OldPickPoint[0] - this->NewPickPoint[0];
    this->MotionVector[1] = this->OldPickPoint[1] - this->NewPickPoint[1];
    this->MotionVector[2] = this->OldPickPoint[2] - this->NewPickPoint[2];
    
    this->CurrentCamera->GetFocalPoint(this->ViewFocus);
    this->CurrentCamera->GetPosition(this->ViewPoint);
    this->CurrentCamera->SetFocalPoint(this->MotionVector[0] +
                                       this->ViewFocus[0],
                                       this->MotionVector[1] +
                                       this->ViewFocus[1],
                                       this->MotionVector[2] +
                                       this->ViewFocus[2]);
    this->CurrentCamera->SetPosition(this->MotionVector[0] +
                                     this->ViewPoint[0],
                                     this->MotionVector[1] +
                                     this->ViewPoint[1],
                                     this->MotionVector[2] +
                                     this->ViewPoint[2]);
      
    vtkRenderWindowInteractor *rwi = this->Interactor;
    if (rwi->GetLightFollowCamera())
      {
      /* get the first light */
      this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
      this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
      }
    
    this->OldX = x;
    this->OldY = y;
    rwi->Render();
    }
}

// Description:
// dolly the camera in trackball (motion sensitive) style
// dolly is based on distance from center of screen,
// and the upper half is positive, lower half is negative
void vtkInteractorStyleTrackball::TrackballDollyCamera(int x, int y)
{
  if (this->OldY != y)
    {
    double dyf = this->MotionFactor * (double)(y - this->OldY) /
      (double)(this->Center[1]);
    double zoomFactor = pow((double)1.1, dyf);
          
    if (this->CurrentCamera->GetParallelProjection())
      {
      this->CurrentCamera->
        SetParallelScale(this->CurrentCamera->GetParallelScale()/zoomFactor);
      }
    else
      {
      this->CurrentCamera->Dolly(zoomFactor);
      this->CurrentRenderer->ResetCameraClippingRange();
      }
    
    vtkRenderWindowInteractor *rwi = this->Interactor;
    if (rwi->GetLightFollowCamera())
      {
      /* get the first light */
      this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
      this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
      }

    this->OldX = x;
    this->OldY = y;
    rwi->Render();
    }
}

// Description:
// rotate the actor in trackball (motion sensitive) style
void vtkInteractorStyleTrackball::TrackballRotateActor(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if ((this->OldX != x) || (this->OldY != y))
    {

    if (this->Preprocess)
      {
      float *center = this->InteractionProp->GetCenter();
      this->ObjCenter[0] = center[0];
      this->ObjCenter[1] = center[1];
      this->ObjCenter[2] = center[2];

      // GetLength gets the length of the diagonal of the bounding box
      double boundRadius = this->InteractionProp->GetLength() * 0.5;

      // get the view up and view right vectors
      this->CurrentCamera->OrthogonalizeViewUp();
      this->CurrentCamera->ComputeViewPlaneNormal();
      this->CurrentCamera->GetViewUp(this->ViewUp);
      vtkMath::Normalize(this->ViewUp);
      this->CurrentCamera->GetViewPlaneNormal(this->ViewLook);
      vtkMath::Cross(this->ViewUp, this->ViewLook, this->ViewRight);
      vtkMath::Normalize(this->ViewRight);

      // get the furtherest point from object position+origin
      double outsidept[3];
      outsidept[0] = this->ObjCenter[0] + this->ViewRight[0] * boundRadius;
      outsidept[1] = this->ObjCenter[1] + this->ViewRight[1] * boundRadius;
      outsidept[2] = this->ObjCenter[2] + this->ViewRight[2] * boundRadius;

      // convert them to display coord
      this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                  this->ObjCenter[2], this->DispObjCenter);
      this->ComputeWorldToDisplay(outsidept[0], outsidept[1],
                                  outsidept[2], outsidept);

      // get the radius in display coord
      double ftmp[3];
      ftmp[0] = this->DispObjCenter[0];
      ftmp[1] = this->DispObjCenter[1];
      ftmp[2] = this->DispObjCenter[2];

      this->Radius = sqrt(vtkMath::Distance2BetweenPoints(ftmp, outsidept));
      this->HighlightProp3D(NULL);
      this->Preprocess = 0;
      }

    double nxf = (double)(x - this->DispObjCenter[0]) / this->Radius;
    double nyf = (double)(y - this->DispObjCenter[1]) / this->Radius;
    double oxf = (double)(this->OldX - this->DispObjCenter[0]) / this->Radius;
    double oyf = (double)(this->OldY - this->DispObjCenter[1]) / this->Radius;

    if (((nxf * nxf + nyf * nyf) <= 1.0) &&
        ((oxf * oxf + oyf * oyf) <= 1.0))
      {
            
      double newXAngle = asin(nxf) * this->RadianToDegree;
      double newYAngle = asin(nyf) * this->RadianToDegree;
      double oldXAngle = asin(oxf) * this->RadianToDegree;
      double oldYAngle = asin(oyf) * this->RadianToDegree;

      double scale[3];
      scale[0] = scale[1] = scale[2] = 1.0;
      double **rotate = new double*[2];
      rotate[0] = new double[4];
      rotate[1] = new double[4];

      rotate[0][0] = newXAngle - oldXAngle;
      rotate[0][1] = this->ViewUp[0];
      rotate[0][2] = this->ViewUp[1];
      rotate[0][3] = this->ViewUp[2];
      
      rotate[1][0] = oldYAngle - newYAngle;
      rotate[1][1] = this->ViewRight[0];
      rotate[1][2] = this->ViewRight[1];
      rotate[1][3] = this->ViewRight[2];
      
      
      this->Prop3DTransform(this->InteractionProp,
                           this->ObjCenter,
                           2, rotate, scale);

      delete [] rotate[0];
      delete [] rotate[1];
      delete [] rotate;
      
      this->OldX = x;
      this->OldY = y;
      this->CurrentRenderer->ResetCameraClippingRange();
      rwi->Render();
      }
    }
}

// Description:
// spin the actor in trackball (motion sensitive) style
void vtkInteractorStyleTrackball::TrackballSpinActor(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      // get the position plus origin of the object
      float *center = this->InteractionProp->GetCenter();
      this->ObjCenter[0] = center[0];
      this->ObjCenter[1] = center[1];
      this->ObjCenter[2] = center[2];

      // get the axis to rotate around = vector from eye to origin
      if (this->CurrentCamera->GetParallelProjection())
        {
        this->CurrentCamera->ComputeViewPlaneNormal();
        this->CurrentCamera->GetViewPlaneNormal(this->MotionVector);
        }
      else
        {   
        this->CurrentCamera->GetPosition(this->ViewPoint);
        this->MotionVector[0] = this->ViewPoint[0] - this->ObjCenter[0];
        this->MotionVector[1] = this->ViewPoint[1] - this->ObjCenter[1];
        this->MotionVector[2] = this->ViewPoint[2] - this->ObjCenter[2];
        vtkMath::Normalize(this->MotionVector);
        }
      
      this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                  this->ObjCenter[2], this->DispObjCenter);

      this->HighlightProp3D(NULL);
      this->Preprocess = 0;
      }
    
    // this has to be in the loop
    double newAngle = atan2((double)(y - this->DispObjCenter[1]),
                           (double)(x - this->DispObjCenter[0]));
    double oldAngle = atan2((double)(this->OldY - this->DispObjCenter[1]),
                           (double)(this->OldX - this->DispObjCenter[0]));
    
    newAngle *= this->RadianToDegree;
    oldAngle *= this->RadianToDegree;

    double scale[3];
    scale[0] = scale[1] = scale[2] = 1.0;
    double **rotate = new double*[1];
    rotate[0] = new double[4];

    rotate[0][0] = newAngle - oldAngle;
    rotate[0][1] = this->MotionVector[0];
    rotate[0][2] = this->MotionVector[1];
    rotate[0][3] = this->MotionVector[2];
  
    this->Prop3DTransform(this->InteractionProp,
                         this->ObjCenter,
                         1, rotate, scale);

    delete [] rotate[0];
    delete [] rotate;
    
    this->OldX = x;
    this->OldY = y;
    this->CurrentRenderer->ResetCameraClippingRange();
    rwi->Render();
    }
}

// Description:
// pan the actor in trackball (motion sensitive) style
void vtkInteractorStyleTrackball::TrackballPanActor(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      // use initial center as the origin from which to pan
      float *center = this->InteractionProp->GetCenter();
      this->ObjCenter[0] = center[0];
      this->ObjCenter[1] = center[1];
      this->ObjCenter[2] = center[2];
      this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                  this->ObjCenter[2], this->DispObjCenter);
      this->FocalDepth = this->DispObjCenter[2];
      
      this->HighlightProp3D(NULL);
      this->Preprocess = 0;
      }
  
    this->ComputeDisplayToWorld(double(x), double(y),
                                this->FocalDepth,
                                this->NewPickPoint);

    this->ComputeDisplayToWorld(double(this->OldX), double(this->OldY),
                                this->FocalDepth, this->OldPickPoint);

    this->MotionVector[0] = this->NewPickPoint[0] - this->OldPickPoint[0];
    this->MotionVector[1] = this->NewPickPoint[1] - this->OldPickPoint[1];
    this->MotionVector[2] = this->NewPickPoint[2] - this->OldPickPoint[2];

    if (this->InteractionProp->GetUserMatrix() != NULL)
      {
      vtkTransform *t = vtkTransform::New();
      t->PostMultiply();
      t->SetMatrix(this->InteractionProp->GetUserMatrix());
      t->Translate(this->MotionVector[0], this->MotionVector[1], 
                   this->MotionVector[2]);
      this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
      t->Delete();
      }
    else
      {
      this->InteractionProp->AddPosition(this->MotionVector);
      }
      
    this->OldX = x;
    this->OldY = y;
    rwi->Render();
    }
}


// Description:
// Dolly the actor in trackball (motion sensitive) style
void vtkInteractorStyleTrackball::TrackballDollyActor(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->OldY != y)
    {
    if (this->Preprocess)
      {
      this->CurrentCamera->GetPosition(this->ViewPoint);
      this->CurrentCamera->GetFocalPoint(this->ViewFocus);

      this->HighlightProp3D(NULL);
      this->Preprocess = 0;
      }
    
    double yf = (double)(this->OldY - y) / (double)(this->Center[1]) *
      this->MotionFactor;
    double dollyFactor = pow((double)1.1, yf);

    dollyFactor -= 1.0;
    this->MotionVector[0] = (this->ViewPoint[0] -
                             this->ViewFocus[0]) * dollyFactor;
    this->MotionVector[1] = (this->ViewPoint[1] -
                             this->ViewFocus[1]) * dollyFactor;
    this->MotionVector[2] = (this->ViewPoint[2] -
                             this->ViewFocus[2]) * dollyFactor;
    
    if (this->InteractionProp->GetUserMatrix() != NULL)
      {
      vtkTransform *t = vtkTransform::New();
      t->PostMultiply();
      t->SetMatrix(this->InteractionProp->GetUserMatrix());
      t->Translate(this->MotionVector[0], this->MotionVector[1], 
                   this->MotionVector[2]);
      this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
      t->Delete();
      }
    else
      {
      this->InteractionProp->AddPosition(this->MotionVector);
      }
  
    this->OldX = x;
    this->OldY = y;
    rwi->Render();
    }
}

// Description:
// Scale the actor in trackball (motion sensitive) style
void vtkInteractorStyleTrackball::TrackballScaleActor(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if ((this->OldX != x) || (this->OldY != y))
    {
    if (this->Preprocess)
      {
      float *center = this->InteractionProp->GetCenter();
      this->ObjCenter[0] = center[0];
      this->ObjCenter[1] = center[1];
      this->ObjCenter[2] = center[2];

      this->HighlightProp3D(NULL);
      this->Preprocess = 0;
      }
    
    double yf = (double)(y - this->OldY) / (double)(this->Center[1]) *
      this->MotionFactor;
    double scaleFactor = pow((double)1.1, yf);

    double **rotate = NULL;

    double scale[3];
    scale[0] = scale[1] = scale[2] = scaleFactor;

    this->Prop3DTransform(this->InteractionProp,
                         this->ObjCenter,
                         0, rotate, scale);

    this->OldX = x;
    this->OldY = y;
    this->CurrentRenderer->ResetCameraClippingRange();
    rwi->Render();
    }
}

void vtkInteractorStyleTrackball::PrintSelf(ostream& os, vtkIndent indent) 
{
  this->vtkInteractorStyle::PrintSelf(os,indent);

  os << indent << "Interaction Picker: " 
     << this->InteractionPicker << endl;
  os << indent << "Actor Picked: " <<
    (this->PropPicked ? "Yes\n" : "No\n");
  if ( this->InteractionProp )
    {
    os << indent << "Interacting Actor: " << this->InteractionProp << "\n";
    }
  else
    {
    os << indent << "Interacting Actor: (none)\n";
    }
  os << indent << "Mode: " <<
    (this->ActorMode ? "Actor\n" : "Camera\n");
  os << indent << "Mode: " <<
    (this->TrackballMode ? "Trackball\n" : "Joystick\n");
  os << indent << "Control Key: " <<
    (this->ControlMode ? "On\n" : "Off\n");
  os << indent << "Preprocessing: " <<
    (this->Preprocess ? "Yes\n" : "No\n");

}

void vtkInteractorStyleTrackball::JoystickRotateActor(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->Preprocess)
    {
    // first get the origin of the assembly
    float *center = this->InteractionProp->GetCenter();
    this->ObjCenter[0] = center[0];
    this->ObjCenter[1] = center[1];
    this->ObjCenter[2] = center[2];

    // GetLength gets the length of the diagonal of the bounding box
    double boundRadius = this->InteractionProp->GetLength() * 0.5;

    // get the view up and view right vectors
    this->CurrentCamera->OrthogonalizeViewUp();
    this->CurrentCamera->ComputeViewPlaneNormal();
    this->CurrentCamera->GetViewUp(this->ViewUp);
    vtkMath::Normalize(this->ViewUp);
    this->CurrentCamera->GetViewPlaneNormal(this->ViewLook);
    vtkMath::Cross(this->ViewUp, this->ViewLook, this->ViewRight);
    vtkMath::Normalize(this->ViewRight);

    // get the furtherest point from object bounding box center
    float outsidept[3];
    outsidept[0] = this->ObjCenter[0] + this->ViewRight[0] * boundRadius;
    outsidept[1] = this->ObjCenter[1] + this->ViewRight[1] * boundRadius;
    outsidept[2] = this->ObjCenter[2] + this->ViewRight[2] * boundRadius;

    // convert to display coord
    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);
    this->ComputeWorldToDisplay(outsidept[0], outsidept[1],
                                outsidept[2], outsidept);

    this->Radius = sqrt(vtkMath::Distance2BetweenPoints(this->DispObjCenter,
                                                        outsidept));

    this->HighlightProp3D(NULL);
    this->Preprocess = 0;
    }


  double nxf = (double)(x - this->DispObjCenter[0]) / this->Radius;
  double nyf = (double)(y - this->DispObjCenter[1]) / this->Radius;

  if (nxf > 1.0)
    {
    nxf = 1.0;
    }
  else if (nxf < -1.0)
    {
    nxf = -1.0;
    }

  if (nyf > 1.0)
    {
    nyf = 1.0;
    }
  else if (nyf < -1.0)
    {
    nyf = -1.0;
    }

  double newXAngle = asin(nxf) * this->RadianToDegree / this->MotionFactor;
  double newYAngle = asin(nyf) * this->RadianToDegree / this->MotionFactor;

  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;
  double **rotate = new double*[2];
  rotate[0] = new double[4];
  rotate[1] = new double[4];

  rotate[0][0] = newXAngle;
  rotate[0][1] = this->ViewUp[0];
  rotate[0][2] = this->ViewUp[1];
  rotate[0][3] = this->ViewUp[2];

  rotate[1][0] = -newYAngle;
  rotate[1][1] = this->ViewRight[0];
  rotate[1][2] = this->ViewRight[1];
  rotate[1][3] = this->ViewRight[2];


  this->Prop3DTransform(this->InteractionProp,
                       this->ObjCenter,
                       2, rotate, scale);

  delete [] rotate[0];
  delete [] rotate[1];
  delete [] rotate;

  rwi->Render();
}

void vtkInteractorStyleTrackball::JoystickSpinActor(int vtkNotUsed(x), int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  // get the axis to rotate around = vector from eye to origin
  if (this->Preprocess)
    {

    float *center = this->InteractionProp->GetCenter();
    this->ObjCenter[0] = center[0];
    this->ObjCenter[1] = center[1];
    this->ObjCenter[2] = center[2];

    if (this->CurrentCamera->GetParallelProjection())
      {
      // if parallel projection, want to get the view plane normal...
      this->CurrentCamera->ComputeViewPlaneNormal();
      this->CurrentCamera->GetViewPlaneNormal(this->MotionVector);
      }
    else
      {
      // perspective projection, get vector from eye to center of actor
      this->CurrentCamera->GetPosition(this->ViewPoint);
      this->MotionVector[0] = this->ViewPoint[0] - this->ObjCenter[0];
      this->MotionVector[1] = this->ViewPoint[1] - this->ObjCenter[1];
      this->MotionVector[2] = this->ViewPoint[2] - this->ObjCenter[2];
      vtkMath::Normalize(this->MotionVector);
      }

    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);

    this->HighlightProp3D(NULL);
    this->Preprocess = 0;
    }

  double yf = (double)(y -this->DispObjCenter[1]) / (double)(this->Center[1]);
  if (yf > 1.0)
    {
    yf = 1.0;
    }
  else if (yf < -1.0)
    {
    yf = -1.0;
    }

  double newAngle = asin(yf) * this->RadianToDegree / this->MotionFactor;

  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;
  double **rotate = new double*[1];
  rotate[0] = new double[4];

  rotate[0][0] = newAngle;
  rotate[0][1] = this->MotionVector[0];
  rotate[0][2] = this->MotionVector[1];
  rotate[0][3] = this->MotionVector[2];

  this->Prop3DTransform(this->InteractionProp,
                       this->ObjCenter,
                       1, rotate, scale);

  delete [] rotate[0];
  delete [] rotate;

  rwi->Render();
}

void vtkInteractorStyleTrackball::JoystickPanActor(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->Preprocess)
    {
    // use initial center as the origin from which to pan
    float *center = this->InteractionProp->GetCenter();
    this->ObjCenter[0] = center[0];
    this->ObjCenter[1] = center[1];
    this->ObjCenter[2] = center[2];

    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);
    this->FocalDepth = this->DispObjCenter[2];

    this->HighlightProp3D(NULL);
    this->Preprocess = 0;
    }

  this->ComputeDisplayToWorld(double(x), double(y),
                              this->FocalDepth,
                              this->NewPickPoint);

  /*
   * Compute a translation vector, moving everything 1/10
   * the distance to the cursor. (Arbitrary scale factor)
   */
  this->MotionVector[0] = (this->NewPickPoint[0] -
                           this->ObjCenter[0]) / this->MotionFactor;
  this->MotionVector[1] = (this->NewPickPoint[1] -
                           this->ObjCenter[1]) / this->MotionFactor;
  this->MotionVector[2] = (this->NewPickPoint[2] -
                           this->ObjCenter[2]) / this->MotionFactor;

  if (this->InteractionProp->GetUserMatrix() != NULL)
    {
    vtkTransform *t = vtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(this->InteractionProp->GetUserMatrix());
    t->Translate(this->MotionVector[0], this->MotionVector[1],
                 this->MotionVector[2]);
    this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
    t->Delete();
    }
  else
    {
    this->InteractionProp->AddPosition(this->MotionVector);
    }

  rwi->Render();
}

void vtkInteractorStyleTrackball::JoystickDollyActor(int vtkNotUsed(x), int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  // dolly is based on distance from center of screen,
  // and the upper half is positive, lower half is negative

  if (this->Preprocess)
    {
    this->CurrentCamera->GetPosition(this->ViewPoint);
    this->CurrentCamera->GetFocalPoint(this->ViewFocus);

    // use initial center as the origin from which to pan
    float *center = this->InteractionProp->GetCenter();
    this->ObjCenter[0] = center[0];
    this->ObjCenter[1] = center[1];
    this->ObjCenter[2] = center[2];
    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);

    this->HighlightProp3D(NULL);
    this->Preprocess = 0;
    }

  double yf = (double)(y - this->DispObjCenter[1]) /
    (double)(this->Center[1]);
  double dollyFactor = pow((double)1.1, yf);

  dollyFactor -= 1.0;
  this->MotionVector[0] = (this->ViewPoint[0] -
                           this->ViewFocus[0]) * dollyFactor;
  this->MotionVector[1] = (this->ViewPoint[1] -
                           this->ViewFocus[1]) * dollyFactor;
  this->MotionVector[2] = (this->ViewPoint[2] -
                           this->ViewFocus[2]) * dollyFactor;

  if (this->InteractionProp->GetUserMatrix() != NULL)
    {
    vtkTransform *t = vtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(this->InteractionProp->GetUserMatrix());
    t->Translate(this->MotionVector[0], this->MotionVector[1],
                 this->MotionVector[2]);
    this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
    t->Delete();
    }
  else
    {
    this->InteractionProp->AddPosition(this->MotionVector);
    }

  rwi->Render();
}

void vtkInteractorStyleTrackball::JoystickScaleActor(int vtkNotUsed(x), int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  // Uniform scale is based on distance from center of screen,
  // and the upper half is positive, lower half is negative

  if (this->Preprocess)
    {
    // use bounding box center as the origin from which to pan
    float *center = this->InteractionProp->GetCenter();
    this->ObjCenter[0] = center[0];
    this->ObjCenter[1] = center[1];
    this->ObjCenter[2] = center[2];

    this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                                this->ObjCenter[2], this->DispObjCenter);

    this->HighlightProp3D(NULL);
    this->Preprocess = 0;
    }

  double yf = (double)(y - this->DispObjCenter[1]) /
    (double)(this->Center[1]);
  double scaleFactor = pow((double)1.1, yf);

  double **rotate = NULL;

  double scale[3];
  scale[0] = scale[1] = scale[2] = scaleFactor;

  this->Prop3DTransform(this->InteractionProp,
                       this->ObjCenter,
                       0, rotate, scale);

  rwi->Render();
}


//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::Prop3DTransform(vtkProp3D *prop3D,
                                                 double *boxCenter,
                                                 int numRotation,
                                                 double **rotate,
                                                 double *scale)
{
  vtkMatrix4x4 *oldMatrix = vtkMatrix4x4::New();
  prop3D->GetMatrix(oldMatrix);
  
  float orig[3];
  prop3D->GetOrigin(orig);
  
  vtkTransform *newTransform = vtkTransform::New();
  newTransform->PostMultiply();
  if (prop3D->GetUserMatrix() != NULL) 
    {
    newTransform->SetMatrix(prop3D->GetUserMatrix());
    }
  else 
    {
    newTransform->SetMatrix(oldMatrix);
    }
  
  newTransform->Translate(-(boxCenter[0]), -(boxCenter[1]), -(boxCenter[2]));
  
  for (int i = 0; i < numRotation; i++) 
    {
    newTransform->RotateWXYZ(rotate[i][0], rotate[i][1],
                             rotate[i][2], rotate[i][3]);
    }
  
  if ((scale[0] * scale[1] * scale[2]) != 0.0) 
    {
    newTransform->Scale(scale[0], scale[1], scale[2]);
    }
  
  newTransform->Translate(boxCenter[0], boxCenter[1], boxCenter[2]);
  
  // now try to get the composit of translate, rotate, and scale
  newTransform->Translate(-(orig[0]), -(orig[1]), -(orig[2]));
  newTransform->PreMultiply();
  newTransform->Translate(orig[0], orig[1], orig[2]);
  
  if (prop3D->GetUserMatrix() != NULL) 
    {
    newTransform->GetMatrix(prop3D->GetUserMatrix());
    }
  else 
    {
    prop3D->SetPosition(newTransform->GetPosition());
    prop3D->SetScale(newTransform->GetScale());
    prop3D->SetOrientation(newTransform->GetOrientation());
    }
  oldMatrix->Delete();
  newTransform->Delete();
}



//----------------------------------------------------------------------------
// Intercept any keypresses which are style independent here and do the rest in
// subclasses - none really required yet!
//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::OnChar(int ctrl, int shift, 
                                         char keycode, int repeatcount) 
{
  // first invoke superclass method
  this->vtkInteractorStyle::OnChar(ctrl,shift,keycode,repeatcount);

  // catch additional keycodes
  switch (keycode) 
    {
    case 'j':
    case 'J':
      if (this->State == VTKIS_START) 
        {
        this->TrackballMode = VTKIS_JOY;
        }
      break;
      
    case 't':
    case 'T':
      if (this->State == VTKIS_START) 
        {
        this->TrackballMode = VTKIS_TRACK;
        }
      break;
      
    case 'o':
    case 'O':
      if (this->State == VTKIS_START) 
        {
        if (this->ActorMode != VTKIS_ACTOR)
          {
          // reset the actor picking variables
          this->InteractionProp = NULL;
          this->PropPicked = 0;
          this->HighlightProp3D(NULL);          
          this->ActorMode = VTKIS_ACTOR;
          }
        }
      break;
      
    case 'c':
    case 'C':
      if (this->State == VTKIS_START) 
        {
        if (this->ActorMode != VTKIS_CAMERA)
          {
          // reset the actor picking variables
          this->InteractionProp = NULL;
          this->PropPicked = 0;
          this->HighlightProp3D(NULL);
          this->ActorMode = VTKIS_CAMERA;
          }
        }
      break;
    }
}

//----------------------------------------------------------------------------
// By overriding the RotateCamera, RotateActor members we can
// use this timer routine for Joystick or Trackball - quite tidy
//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::OnTimer(void) 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  switch (this->State) 
    {
    case VTKIS_START:
      if (this->AnimState == VTKIS_ANIM_ON)
        {
        rwi->DestroyTimer();
        rwi->Render();
        rwi->CreateTimer(VTKI_TIMER_FIRST);
        }
      break;
      
    case VTKIS_ROTATE:  // rotate with respect to an axis perp to look
      if (this->ActorMode && this->PropPicked)
        {
        if (this->TrackballMode)
          {
          this->TrackballRotateActor(this->LastPos[0],
                                   this->LastPos[1]);
          }
        else
          {
          this->JoystickRotateActor(this->LastPos[0],
                                  this->LastPos[1]);
          }
        }
      else if (!(this->ActorMode))
        {
        if (this->TrackballMode)
          {
          this->TrackballRotateCamera(this->LastPos[0],
                                    this->LastPos[1]);
          }
        else
          {
          this->RotateCamera(this->LastPos[0],this->LastPos[1]);
          }
        }
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      
    case VTKIS_PAN: // move perpendicular to camera's look vector
      if (this->ActorMode && this->PropPicked)
        {
        if (this->TrackballMode)
          { 
          this->TrackballPanActor(this->LastPos[0],
                                  this->LastPos[1]);
          }
        else
          {
          this->JoystickPanActor(this->LastPos[0],
                                 this->LastPos[1]);
          }
        }
      else if (!(this->ActorMode))
        {
        if (this->TrackballMode)
          {
          this->TrackballPanCamera(this->LastPos[0],
                                   this->LastPos[1]);
          }
        else
          {
          this->PanCamera(this->LastPos[0],this->LastPos[1]);
          }
        }
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      
    case VTKIS_ZOOM:
      if (!(this->ActorMode))
        {
        if (this->TrackballMode)
          { 
          this->TrackballDollyCamera(this->LastPos[0],
                                   this->LastPos[1]);
          }
        else
          {
          this->DollyCamera(this->LastPos[0],this->LastPos[1]);
          }
        }
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      
    case VTKIS_SPIN:
      if (this->ActorMode && this->PropPicked)
        {
        if (this->TrackballMode)
          { 
          this->TrackballSpinActor(this->LastPos[0],
                                 this->LastPos[1]);
          }
        else
          {
          this->JoystickSpinActor(this->LastPos[0],
                                this->LastPos[1]);
          }
        }
      else if (!(this->ActorMode))
        {
        if (this->TrackballMode)
          {
          this->TrackballSpinCamera(this->LastPos[0],
                                  this->LastPos[1]);
          }
        else
          {
          this->SpinCamera(this->LastPos[0],this->LastPos[1]);
          }
        }
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      
    case VTKIS_DOLLY:  // move along camera's view vector
      if (this->ActorMode && this->PropPicked)
        {
        if (this->TrackballMode)
          { 
          this->TrackballDollyActor(this->LastPos[0],
                                  this->LastPos[1]);
          }
        else
          {
          this->JoystickDollyActor(this->LastPos[0],
                                 this->LastPos[1]);
          }
        }
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      
    case VTKIS_USCALE:
      if (this->ActorMode && this->PropPicked)
        {
        if (this->TrackballMode)
          {
          this->TrackballScaleActor(this->LastPos[0],
                                    this->LastPos[1]);
          }
        else
          {
          this->JoystickScaleActor(this->LastPos[0],
                                   this->LastPos[1]);
          }
        }
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      
    case VTKIS_TIMER:
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;

    default :
      break;
    }
}

void vtkInteractorStyleTrackball::Prop3DTransform(vtkProp3D *prop3D,
                                                float *boxCenter,
                                                int numRotation,
                                                double **rotate,
                                                double *scale)
{
  double boxCenter2[3];
  boxCenter2[0] = boxCenter[0];
  boxCenter2[1] = boxCenter[1];
  boxCenter2[2] = boxCenter[2];
  this->Prop3DTransform(prop3D,boxCenter2,numRotation,rotate,scale);
}

void vtkInteractorStyleTrackball::FindPickedActor(int X, int Y)
{
  this->InteractionPicker->Pick(X,Y, 0.0, this->CurrentRenderer);
  vtkProp *prop = this->InteractionPicker->GetProp();
  if ( prop != NULL )
    {
    vtkProp3D *prop3D = vtkProp3D::SafeDownCast(prop);
    if ( prop3D != NULL )
      {
      this->InteractionProp = prop3D;
      }
    }

  // refine the answer to whether an actor was picked.  CellPicker()
  // returns true from Pick() if the bounding box was picked,
  // but we only want something to be picked if a cell was actually
  // selected
  this->PropPicked = (this->InteractionProp != NULL);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::OnLeftButtonDown(int ctrl, int shift, 
                                          int X, int Y) 
{
  //
  this->OldX = X;
  this->OldY = Y;
  this->UpdateInternalState(ctrl, shift, X, Y);

  this->FindPokedCamera(X, Y);
  this->Preprocess = 1;
  if (this->HasObserver(vtkCommand::LeftButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    }
  else 
    {
    if (this->ActorMode)
      {
      this->FindPickedActor(X,Y);    
      }
    if (this->ShiftKey) 
      { // I haven't got a Middle button !
      if (this->CtrlKey) 
        {
        this->StartDolly();
        }
      else      
        {
        this->StartPan();
        }
      } 
    else 
      {
      if (this->CtrlKey) 
        {
        this->StartSpin();
        }
      else         
        {
        this->StartRotate();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::OnLeftButtonUp(int ctrl, int shift, int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
  if (this->HasObserver(vtkCommand::LeftButtonReleaseEvent)) 
    {
    this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
    }
  else 
    {
    if (this->ShiftKey) 
      {
      if (this->CtrlKey) 
        {
        this->EndDolly();
        }
      else        
        {
        this->EndPan();
        }
      } 
    else 
      {
      if (this->CtrlKey) 
        {
        this->EndSpin();
        }
      else
        {
        this->EndRotate();
        }
      }
    }
  this->OldX = 0.0;
  this->OldY = 0.0;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::OnMiddleButtonDown(int ctrl, int shift, 
                                            int X, int Y) 
{
  this->OldX = X;
  this->OldY = Y;
  //
  this->UpdateInternalState(ctrl, shift, X, Y);
  //
  this->Preprocess = 1;
  this->FindPokedCamera(X, Y);
  //
  if (this->HasObserver(vtkCommand::MiddleButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);
    }
  else 
    {
    if (this->ActorMode)
      {
      this->FindPickedActor(X,Y);    
      }
    if (this->CtrlKey) 
      {
      this->StartDolly();
      }
    else
      {
      this->StartPan();
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::OnMiddleButtonUp(int ctrl, int shift, 
                                                   int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
  if (this->HasObserver(vtkCommand::MiddleButtonReleaseEvent)) 
    {
    this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent,NULL);
    }
  else 
    {
    if (this->CtrlKey) 
      {
      this->EndDolly();
      }
    else   
      {
      this->EndPan();
      }
    }
  this->OldX = 0.0;
  this->OldY = 0.0;
  if (this->ActorMode && this->PropPicked)
    {
    this->HighlightProp3D(this->InteractionProp);
    }
  else if (this->ActorMode)
    {
    this->HighlightProp3D(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::OnRightButtonDown(int ctrl, int shift, 
                                                    int X, int Y) 
{
  this->OldX = X;
  this->OldY = Y;
  //
  this->UpdateInternalState(ctrl, shift, X, Y);
  this->FindPokedCamera(X, Y);
  this->Preprocess = 1;
  if (this->HasObserver(vtkCommand::RightButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);
    }
  else 
    {
    if (this->ActorMode)
      {
      this->FindPickedActor(X,Y);    
      this->StartUniformScale();
      }
    else
      {
      this->StartZoom();
      }
    }
}
//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::OnRightButtonUp(int ctrl, int shift, 
                                                  int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
  if (this->HasObserver(vtkCommand::RightButtonReleaseEvent)) 
    {
    this->InvokeEvent(vtkCommand::RightButtonReleaseEvent,NULL);
    }
  else 
    {
    if (this->ActorMode)
      {
      this->EndUniformScale();
      }
    else
      {
      this->EndZoom();
      }
    }
  this->OldX = 0.0;
  this->OldY = 0.0;
  if (this->ActorMode && this->PropPicked)
    {
    this->HighlightProp3D(this->InteractionProp);
    }
  else if (this->ActorMode)
    {
    this->HighlightProp3D(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::SetTrackballModeToTrackball()
{
  if (this->TrackballMode == VTKIS_TRACK)
    {
    return;
    }
  this->TrackballMode = VTKIS_TRACK;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::SetTrackballModeToJoystick()
{
  if (this->TrackballMode == VTKIS_JOY)
    {
    return;
    }
  this->TrackballMode = VTKIS_JOY;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::SetActorModeToCamera()
{
  if (this->ActorMode == VTKIS_CAMERA)
    {
    return;
    }
  this->ActorMode = VTKIS_CAMERA;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::SetActorModeToActor()
{
  if (this->ActorMode == VTKIS_ACTOR)
    {
    return;
    }
  this->ActorMode = VTKIS_ACTOR;
  this->Modified();

}
