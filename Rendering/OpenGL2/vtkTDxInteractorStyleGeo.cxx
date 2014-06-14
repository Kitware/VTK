/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxInteractorStyleGeo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTDxInteractorStyleGeo.h"

#include "vtkTransform.h"
#include <cassert>
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTDxMotionEventInfo.h"
#include "vtkObjectFactory.h"
#include "vtkTDxInteractorStyleSettings.h"

vtkStandardNewMacro(vtkTDxInteractorStyleGeo);

// ----------------------------------------------------------------------------
vtkTDxInteractorStyleGeo::vtkTDxInteractorStyleGeo()
{
  this->Transform=vtkTransform::New();
//  this->DebugOn();
}

// ----------------------------------------------------------------------------
vtkTDxInteractorStyleGeo::~vtkTDxInteractorStyleGeo()
{
  this->Transform->Delete();
}

// ----------------------------------------------------------------------------
void vtkTDxInteractorStyleGeo::OnMotionEvent(
  vtkTDxMotionEventInfo *motionInfo)
{
  assert("pre: motionInfo_exist" && motionInfo!=0);

  const double TyCalibration=0.1; // the value works well.
  const double RxCalibration=0.1; // this value works well.

  vtkDebugMacro(<<"vtkTDxInteractorStyleGeo::OnMotionEvent()");

  if(this->Renderer==0 || this->Settings==0)
    {
    vtkDebugMacro(<<"vtkTDxInteractorStyleGeo::OnMotionEvent() no renderer or no settings");
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

  // Get the rotation axis in world coordinates.
  this->Transform->Identity();
  this->Transform->Concatenate(eyeToWorld);
  this->Transform->Inverse();

  double xAxisEye[3];
  double xAxisWorld[3];
  xAxisEye[0]=1.0;
  xAxisEye[1]=0.0;
  xAxisEye[2]=0.0;
  this->Transform->TransformVector(xAxisEye,xAxisWorld);

  double yAxisEye[3];
  double yAxisWorld[3];
  yAxisEye[0]=0.0;
  yAxisEye[1]=1.0;
  yAxisEye[2]=0.0;
  this->Transform->TransformVector(yAxisEye,yAxisWorld);

  double zAxisEye[3];
  double zAxisWorld[3];
  zAxisEye[0]=0.0;
  zAxisEye[1]=0.0;
  zAxisEye[2]=1.0;

  this->Transform->TransformVector(zAxisEye,zAxisWorld);

  // Get the translation vector in world coordinates. Used at the end.
  double translationEye[3];
  double translationWorld[3];
  translationEye[0]=0.0;
  translationEye[1]=0.0;
  translationEye[2]=TyCalibration*motionInfo->Y*
    this->Settings->GetTranslationYSensitivity();

  this->Transform->TransformVector(translationEye,translationWorld);

  this->Transform->Identity();

  // default multiplication is "pre" which means applied to the "right" of
  // the current matrix, which follows the OpenGL multiplication convention.

  double *p=c->GetFocalPoint();

  // 1. build the displacement (aka affine rotation) with the axes
  // passing through the focal point.
  this->Transform->Translate(p[0],p[1],p[2]);

  // Device X translation maps to camera Y rotation (west to east)
  this->Transform->RotateWXYZ(
    motionInfo->X*this->Settings->GetAngleSensitivity(),
    yAxisWorld[0],yAxisWorld[1],yAxisWorld[2]);

  // Device Z translation maps to camera X rotation (south to north)
  this->Transform->RotateWXYZ(
    motionInfo->Z*this->Settings->GetAngleSensitivity(),
    xAxisWorld[0],xAxisWorld[1],xAxisWorld[2]);

  // Device Y rotation maps to camera Z rotation (tilt)
  this->Transform->RotateWXYZ(motionInfo->Angle*motionInfo->AxisY*
                              this->Settings->GetAngleSensitivity(),
                              zAxisWorld[0],zAxisWorld[1],zAxisWorld[2]);
  this->Transform->Translate(-p[0],-p[1],-p[2]);

  //2. build the displacement (aka affine rotation) with the axes
  // passing through the camera position.

  double *pos=c->GetPosition();
  this->Transform->Translate(pos[0],pos[1],pos[2]);

  // Device X rotation maps to camera X rotation
  this->Transform->RotateWXYZ(
    RxCalibration*motionInfo->Angle*motionInfo->AxisX*
    this->Settings->GetAngleSensitivity(),
    xAxisWorld[0],xAxisWorld[1],xAxisWorld[2]);
  this->Transform->Translate(-pos[0],-pos[1],-pos[2]);


  // Apply the transform to the camera position
  double newPosition[3];
  this->Transform->TransformPoint(pos,newPosition);

  // 3. In addition position is translated (not the focal point)
  newPosition[0]=newPosition[0]+translationWorld[0];
  newPosition[1]=newPosition[1]+translationWorld[1];
  newPosition[2]=newPosition[2]+translationWorld[2];

  // Apply the vector part of the transform to the camera view up vector
  double *up=c->GetViewUp();
  double newUp[3];
  this->Transform->TransformVector(up,newUp);

  // Apply the transform to the camera focal point
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
void vtkTDxInteractorStyleGeo::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
