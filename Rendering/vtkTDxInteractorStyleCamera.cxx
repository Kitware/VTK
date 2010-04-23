/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxInteractorStyleCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTDxInteractorStyleCamera.h"

#include "vtkTransform.h"
#include <assert.h>
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTDxMotionEventInfo.h"
#include "vtkObjectFactory.h"
#include "vtkTDxInteractorStyleSettings.h"

vtkStandardNewMacro(vtkTDxInteractorStyleCamera);

// ----------------------------------------------------------------------------
vtkTDxInteractorStyleCamera::vtkTDxInteractorStyleCamera()
{
  this->Transform=vtkTransform::New();
//  this->DebugOn();
}

// ----------------------------------------------------------------------------
vtkTDxInteractorStyleCamera::~vtkTDxInteractorStyleCamera()
{
  this->Transform->Delete();
}

// ----------------------------------------------------------------------------
void vtkTDxInteractorStyleCamera::OnMotionEvent(
  vtkTDxMotionEventInfo *motionInfo)
{
  assert("pre: motionInfo_exist" && motionInfo!=0);
  
  vtkDebugMacro(<<"vtkTDxInteractorStyleCamera::OnMotionEvent()");
  
  if(this->Renderer==0 || this->Settings==0)
    {
    vtkDebugMacro(<<"vtkTDxInteractorStyleCamera::OnMotionEvent() no renderer or no settings");
    return;
    }
  
  vtkCamera *c=this->Renderer->GetActiveCamera();
  vtkRenderWindow *w=this->Renderer->GetRenderWindow();
  vtkRenderWindowInteractor *i=w->GetInteractor();
  
  vtkDebugMacro(<< "x=" << motionInfo->X << " y=" << motionInfo->Y << " z="
                << motionInfo->Z
                << " angle=" << motionInfo->Angle << " rx=" 
                << motionInfo->AxisX << " ry="
                << motionInfo->AxisY << " rz="<< motionInfo->AxisZ);
  
  vtkTransform *eyeToWorld=c->GetViewTransformObject();
  double axisEye[3];
  double axisWorld[3];
  
 
  // As we want to rotate the camera, the incoming data are expressed in
  // eye coordinates.
  if(this->Settings->GetUseRotationX())
    {
    axisEye[0]=motionInfo->AxisX;
    }
  else
    {
    axisEye[0]=0.0;
    }
  if(this->Settings->GetUseRotationY())
    {
    axisEye[1]=motionInfo->AxisY;
    }
  else
    {
    axisEye[1]=0.0;
    }
  if(this->Settings->GetUseRotationZ())
    {
    axisEye[2]=motionInfo->AxisZ;
    }
  else
    {
    axisEye[2]=0.0;
    }
  
  // Get the rotation axis in world coordinates.
  this->Transform->Identity();
  this->Transform->Concatenate(eyeToWorld);
  this->Transform->Inverse();
  this->Transform->TransformVector(axisEye,axisWorld);
  
  // Get the translation vector in world coordinates. Used at the end.
  double translationEye[3];
  double translationWorld[3];
  translationEye[0]=motionInfo->X*this->Settings->GetTranslationXSensitivity();
  translationEye[1]=motionInfo->Y*this->Settings->GetTranslationYSensitivity();
  translationEye[2]=motionInfo->Z*this->Settings->GetTranslationZSensitivity();
  this->Transform->TransformVector(translationEye,translationWorld);
  
  
  this->Transform->Identity();
  
  // default multiplication is "pre" which means applied to the "right" of
  // the current matrix, which follows the OpenGL multiplication convention.
  
  // 2. translate (affect position and focalPoint)
  this->Transform->Translate(translationWorld);
 
  // 1. build the displacement (aka affine rotation) with the axis
  // passing through the focal point.
  
  double *p=c->GetFocalPoint();
  this->Transform->Translate(p[0],p[1],p[2]);
  this->Transform->RotateWXYZ(motionInfo->Angle*
                              this->Settings->GetAngleSensitivity(),
                              axisWorld[0],axisWorld[1],axisWorld[2]);
  this->Transform->Translate(-p[0],-p[1],-p[2]);
 
  // Apply the transform to the camera position
  double *pos=c->GetPosition();
  double newPosition[3];
  this->Transform->TransformPoint(pos,newPosition);
          
  // Apply the vector part of the transform to the camera view up vector
  double *up=c->GetViewUp();
  double newUp[3];
  this->Transform->TransformVector(up,newUp);
  
  // Apply the transform to the camera position
  double newFocalPoint[3];
  this->Transform->TransformPoint(p,newFocalPoint);
  
  // Set the new view up vector and position of the camera.
  c->SetViewUp(newUp);
  c->SetPosition(newPosition);
  c->SetFocalPoint(newFocalPoint);
  
  this->Renderer->ResetCameraClippingRange();
  
  // Display the result.
  i->Render(); 
}
 
//----------------------------------------------------------------------------
void vtkTDxInteractorStyleCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
