/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleUnicam.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * This work was produced under a grant from the Department of Energy to Brown 
 * University.  Neither Brown University nor the authors assert any copyright 
 * with respect to this work and it may be used, reproduced, and distributed 
 * without permission.
 */

#include "vtkInteractorStyleUnicam.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkWorldPointPicker.h"
#include "vtkCallbackCommand.h"

vtkStandardNewMacro(vtkInteractorStyleUnicam);

// define 'TheTime()' function-- returns time in elapsed seconds
#if defined(_WIN32) || defined(WIN32)
# include "vtkWindows.h"

static double TheTime() 
  {
    return GetTickCount()/1000.0;
  }
#else
#include <sys/time.h>

static double TheTime() 
{
  struct timeval ts; struct timezone tz;
  gettimeofday(&ts, &tz);
  return static_cast<double>(ts.tv_sec + ts.tv_usec/1e6);
}
#endif

vtkInteractorStyleUnicam::vtkInteractorStyleUnicam()
{
  // use z-buffer picking
  this->InteractionPicker = vtkWorldPointPicker::New();

  // set to default modes
  this->IsDot = 0;
  this->ButtonDown = VTK_UNICAM_NONE;
  state = 0;            // which camera mode is being used?

  // create focus sphere actor
  vtkSphereSource *sphere = vtkSphereSource::New();
  sphere->SetThetaResolution(6); 
  sphere->SetPhiResolution(6);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInput(sphere->GetOutput());
  sphere->Delete();
  
  // XXX - would like to make the focus sphere not be affected by
  // XXX - the lights-- i.e., always be easily easily seen.  i'm not sure
  // XXX - how to do that.
  this->FocusSphere = vtkActor::New();
  this->FocusSphere->SetMapper(sphereMapper);
  this->FocusSphere->GetProperty()->SetColor(0.8900,0.6600,0.4100);
  this->FocusSphere->GetProperty()->SetRepresentationToWireframe();
  sphereMapper->Delete();
  
  // set WorldUpVector to be z-axis by default
  WorldUpVector[0] = 0;
  WorldUpVector[1] = 0;
  WorldUpVector[2] = 1;
}

vtkInteractorStyleUnicam::~vtkInteractorStyleUnicam() 
{
  this->InteractionPicker->Delete();
  this->FocusSphere->Delete();
}

void vtkInteractorStyleUnicam::PrintSelf(ostream& os, vtkIndent indent) 
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Interaction Picker: " << this->InteractionPicker;
//   os << indent << "WorldUpVector: " << this->WorldUpVector;
}

void vtkInteractorStyleUnicam::OnTimer() 
{
  ; //timer just keeps ticking since we are using repeating timers
}

void vtkInteractorStyleUnicam::SetWorldUpVector(double x, double y, double z) 
{
  WorldUpVector[0] = x;
  WorldUpVector[1] = y;
  WorldUpVector[2] = z;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::OnLeftButtonDown() 
{
  this->GrabFocus(this->EventCallbackCommand);

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->ButtonDown = VTK_UNICAM_BUTTON_LEFT;

  this->DTime    = TheTime();
  this->Dist     = 0;

  // cam manip init
  double curpt[2];
  this->NormalizeMouseXY(x, y, &curpt[0], &curpt[1]);
  this->LastPos[0] = curpt[0];
  this->LastPos[1] = curpt[1];

  this->StartPix[0] = this->LastPix[0] = x;
  this->StartPix[1] = this->LastPix[1] = y;

  // Find 'this->DownPt'  (point in world space under the cursor tip)
  // 
  // Note: If no object has been rendered to the pixel (X, Y), then
  // vtkWorldPointPicker will return a z-value with depth equal
  // to the distance from the camera's position to the focal point.
  // This seems like an arbitrary, but perhaps reasonable, default value.
  // 
  this->FindPokedRenderer(x, y);
  this->InteractionPicker->Pick(x, y, 0.0, this->CurrentRenderer);
  this->InteractionPicker->GetPickPosition(this->DownPt);
  
  // if someone has already clicked to make a dot and they're not clicking 
  // on it now, OR if the user is clicking on the perimeter of the screen, 
  // then we want to go into rotation mode.
  if ((fabs(curpt[0]) > .85 || fabs(curpt[1]) > .9) || this->IsDot) 
    {
    if (this->IsDot)
      {
      this->FocusSphere->GetPosition(this->Center);
      }
    state = VTK_UNICAM_CAM_INT_ROT;
    }
  else 
    {
    state = VTK_UNICAM_CAM_INT_CHOOSE;
    }
}

//----------------------------------------------------------------------------
double vtkInteractorStyleUnicam::WindowAspect()
{
  double w = Interactor->GetRenderWindow()->GetSize()[0];
  double h = Interactor->GetRenderWindow()->GetSize()[1];

  return w/h;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::NormalizeMouseXY(int X, int Y,
                                                double *NX, double *NY)
{
  double w = Interactor->GetRenderWindow()->GetSize()[0];
  double h = Interactor->GetRenderWindow()->GetSize()[1];

  *NX = -1.0 + 2.0 * double(X) / w;
  *NY = -1.0 + 2.0 * double(Y) / h;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

 // filter out any repeated events
  static int last_X = 0;
  static int last_Y = 0;
  if (x == last_X && y == last_Y)
    {
    return;
    }

  // channel event to right method handler.
  switch (this->ButtonDown) 
    {
    case VTK_UNICAM_BUTTON_LEFT:
      OnLeftButtonMove();
      break;
    }

  last_X = x;
  last_Y = y;

  this->Interactor->Render();  // re-draw scene.. it should have changed
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::OnLeftButtonUp() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->ButtonDown = VTK_UNICAM_NONE;

  if (state == VTK_UNICAM_CAM_INT_ROT && this->IsDot ) 
    {
    this->FocusSphereRenderer->RemoveActor(this->FocusSphere);
    this->IsDot = 0;
    } 
  else if (state == VTK_UNICAM_CAM_INT_CHOOSE) 
    {
    if (this->IsDot) 
      {
      this->FocusSphereRenderer->RemoveActor(this->FocusSphere);
      this->IsDot = 0;
      } 
    else 
      {
      this->FocusSphere->SetPosition(this->DownPt[0],
                                     this->DownPt[1],
                                     this->DownPt[2]);

      double from[3];
      this->FindPokedRenderer(x, y);
      vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
      camera->GetPosition(from);

      double vec[3];
      vec[0] = this->DownPt[0] - from[0];
      vec[1] = this->DownPt[1] - from[1];
      vec[2] = this->DownPt[2] - from[2];

      double at_v[4];
      camera->GetDirectionOfProjection(at_v);
      vtkMath::Normalize(at_v);

      // calculate scale so focus sphere always is the same size on the screen
      double s = 0.02 * vtkMath::Dot(at_v, vec);

      this->FocusSphere->SetScale(s, s, s);

      this->FindPokedRenderer(x, y);
      this->FocusSphereRenderer = this->CurrentRenderer;
      this->FocusSphereRenderer->AddActor(this->FocusSphere);

      this->IsDot = 1;
      }
    this->Interactor->Render();
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
  rwi->Render();
  if (this->UseTimers)
    {
    rwi->DestroyTimer(this->TimerId);
    }

  this->ReleaseFocus();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::OnLeftButtonMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (state) 
    {
    case VTK_UNICAM_CAM_INT_CHOOSE:   this->ChooseXY(x, y); break;
    case VTK_UNICAM_CAM_INT_ROT:      this->RotateXY(x, y); break;
    case VTK_UNICAM_CAM_INT_PAN:      this->PanXY(x, y); break;
    case VTK_UNICAM_CAM_INT_DOLLY:    this->DollyXY(x, y); break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::ChooseXY( int X, int Y )
{
  int   te[2];  // pixel location
  te[0] = X;
  te[1] = Y;

  double curpt[2];
  this->NormalizeMouseXY(X, Y, &curpt[0], &curpt[1]);
  
  double delta[2];
  delta[0] = curpt[0] - this->LastPos[0];
  delta[1] = curpt[1] - this->LastPos[1];
  this->LastPos[0] = te[0];
  this->LastPos[1] = te[1];

  double tdelt(TheTime() - this->DTime);

  this->Dist += sqrt(delta[0] * delta[0] + delta[1] * delta[1]);

  double sdelt[2];
  sdelt[0] = te[0] - this->StartPix[0];
  sdelt[1] = te[1] - this->StartPix[1];

  int xa=0,ya=1;
  if (getenv("FLIP_CAM_MANIP")) 
    {
    int tmp = xa;
    xa = ya;
    ya = tmp;
    }
     
  double len = sqrt(sdelt[0] * sdelt[0] + sdelt[1] * sdelt[1]);
  if (fabs(sdelt[ya])/len > 0.9 && tdelt > 0.05) 
    {
    state = VTK_UNICAM_CAM_INT_DOLLY;
    } 
  else if (tdelt < 0.1 && this->Dist < 0.03)
    {
    return;
    }
  else 
    {
    if (fabs(sdelt[xa])/len > 0.6 )
      {
      state = VTK_UNICAM_CAM_INT_PAN;
      }
    else 
      {
      state = VTK_UNICAM_CAM_INT_DOLLY;
      }
    }
}

// define some utilty functions
template <class Type>
inline Type clamp(const Type a,
                  const Type b,
                  const Type c) { return a > b ? (a < c ? a : c) : b ; }
inline int  Sign (double a)     { return a > 0 ? 1 : a < 0 ? -1 : 0; }

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::RotateXY( int X, int Y )
{
  double cpt[3];
  double center[3];
  this->FocusSphere->GetPosition(center);
  this->ComputeWorldToDisplay(center[0], center[1], center[2], cpt);
  this->NormalizeMouseXY(static_cast<int>(cpt[0]), static_cast<int>(cpt[1]), 
                         &cpt[0], &cpt[1]);

  double radsq = pow(1.0+fabs(cpt[0]),2.0); // squared rad of virtual cylinder

  double tp[2], te[2];
  this->NormalizeMouseXY(static_cast<int>(this->LastPix[0]), 
                         static_cast<int>(this->LastPix[1]), &tp[0], &tp[1]);
  this->NormalizeMouseXY(X, Y, &te[0], &te[1]);
  this->LastPix[0] = X;
  this->LastPix[1] = Y;

  double op[3], oe[3];
  op[0] = tp[0];
  op[1] = 0;
  op[2] = 0;
  oe[0] = te[0];
  oe[1] = 0;
  oe[2] = 0;

  double opsq = op[0] * op[0], oesq = oe[0] * oe[0];

  double lop  = opsq > radsq ? 0 : sqrt(radsq - opsq);
  double loe  = oesq > radsq ? 0 : sqrt(radsq - oesq);

  double nop[3], noe[3];
  nop[0] = op[0];
  nop[1] = 0;
  nop[2] = lop;
  vtkMath::Normalize(nop);
  noe[0] = oe[0];
  noe[1] = 0;
  noe[2] = loe;
  vtkMath::Normalize(noe);

  double dot = vtkMath::Dot(nop, noe);

  if (fabs(dot) > 0.0001) 
    {
    this->FindPokedRenderer(X, Y);

    double angle = -2*acos(clamp(dot,-1.0,1.0)) * Sign(te[0]-tp[0]);

    double UPvec[3];
    UPvec[0] = WorldUpVector[0];
    UPvec[1] = WorldUpVector[1];
    UPvec[2] = WorldUpVector[2];
    vtkMath::Normalize(UPvec);

    MyRotateCamera(center[0], center[1], center[2], 
                   UPvec[0], UPvec[1], UPvec[2],
                   angle);

    double dvec[3];
    double from[3];
    vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    camera->GetPosition(from);
    for(int i=0; i<3; i++)
      {
      dvec[i] = from[i] - center[i];
      }
     
    double rdist = te[1]-tp[1];
    vtkMath::Normalize(dvec);

    double atV[4], upV[4], rightV[4];
    camera->GetViewPlaneNormal(atV);
    camera->GetViewUp(upV);
    vtkMath::Cross(upV, atV, rightV);
    vtkMath::Normalize(rightV);

    // 
    // The following two tests try to prevent chaotic camera movement
    // that results from rotating over the poles defined by the
    // "WorldUpVector".  The problem is the constraint to keep the
    // camera's up vector in line w/ the WorldUpVector is at odds with
    // the action of rotating tover the top of the virtual sphere used
    // for rotation.  The solution here is to prevent the user from
    // rotating the last bit required to "go over the top"-- as a
    // consequence, you can never look directly down on the poles.
    // 
    // The "0.99" value is somewhat arbitrary, but seems to produce
    // reasonable results.  (Theoretically, some sort of clamping
    // function could probably be used rather than a hard cutoff, but
    // time constraints prevent figuring that out right now.)
    // 
    const double OVER_THE_TOP_THRESHOLD = 0.99;
    if (vtkMath::Dot(UPvec, atV) >  OVER_THE_TOP_THRESHOLD && rdist < 0)
      {
      rdist = 0;
      }
    if (vtkMath::Dot(UPvec, atV) < -OVER_THE_TOP_THRESHOLD && rdist > 0)
      {
      rdist = 0;
      }

    MyRotateCamera(center[0], center[1], center[2],
                   rightV[0], rightV[1], rightV[2],
                   rdist);

    camera->SetViewUp(UPvec[0], UPvec[1], UPvec[2]);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::DollyXY( int X, int Y )
{
  int i;
  double cn[2], ln[2];
  this->NormalizeMouseXY(X, Y, &cn[0], &cn[1]);
  this->NormalizeMouseXY(static_cast<int>(this->LastPix[0]), 
                         static_cast<int>(this->LastPix[1]), &ln[0], &ln[1]);

  double delta[2];
  delta[0] = cn[0] - ln[0];
  delta[1] = cn[1] - ln[1];
  this->LastPix[0] = X;
  this->LastPix[1] = Y;

  // 1. handle dollying
  // XXX - assume perspective projection for now.
  double from[3];
  this->FindPokedRenderer(X, Y);
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetPosition(from);
  
  double movec[3];
  for(i=0; i<3; i++)
    {
    movec[i] = this->DownPt[i] - from[i];
    }

  double offset1[3];
  for(i=0; i<3; i++)
    {
    offset1[i] = movec[i] * delta[1] * -4;
    }

  this->MyTranslateCamera(offset1);


  // 2. now handle side-to-side panning
  double rightV[3], upV[3];
  this->GetRightVandUpV(this->DownPt, camera,
                        rightV, upV);

  double offset2[3];
  for(i=0; i<3; i++)
    {
    offset2[i] = (-delta[0] * rightV[i]);
    }

  this->MyTranslateCamera(offset2);
}

//----------------------------------------------------------------------------
// 
// Transform mouse horizontal & vertical movements to a world
// space offset for the camera that maintains pick correlation.
// 
void vtkInteractorStyleUnicam::PanXY( int X, int Y )
{
  double delta[2];
  double cn[2], ln[2];
  int i;

  this->NormalizeMouseXY(X, Y, &cn[0], &cn[1]);
  this->NormalizeMouseXY(static_cast<int>(this->LastPix[0]), 
                         static_cast<int>(this->LastPix[1]), &ln[0], &ln[1]);
  delta[0] = cn[0] - ln[0];
  delta[1] = cn[1] - ln[1];
  this->LastPix[0] = X;
  this->LastPix[1] = Y;

  // XXX - assume perspective projection for now

  this->FindPokedRenderer(X, Y);

  double rightV[3], upV[3];
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  this->GetRightVandUpV(this->DownPt, camera,
                        rightV, upV);

  double offset[3];
  for(i=0; i<3; i++)
    {
    offset[i] = (-delta[0] * rightV[i] +
                 -delta[1] * upV   [i]);
    }

  this->MyTranslateCamera(offset);
}

// 
// Given a 3D point & a vtkCamera, compute the vectors that extend
// from the projection of the center of projection to the center of
// the right-edge and the center of the top-edge onto the plane
// containing the 3D point & with normal parallel to the camera's
// projection plane.
// 
void vtkInteractorStyleUnicam::GetRightVandUpV(double *p, vtkCamera *cam,
                                               double *rightV, double *upV)
{
  int i;

  // Compute the horizontal & vertical scaling ('scalex' and 'scaley')
  // factors as function of the down point & camera params.
  double from[3];
  cam->GetPosition(from);

  // construct a vector from the viewing position to the picked point
  double vec[3];
  for(i=0; i<3; i++)
    {
    vec[i] = p[i] - from[i];
    }

  // Get shortest distance 'l' between the viewing position and
  // plane parallel to the projection plane that contains the 'DownPt'.
  double atV[4];
  cam->GetViewPlaneNormal(atV);
  vtkMath::Normalize(atV);
  double l = -vtkMath::Dot(vec, atV);

  double view_angle = cam->GetViewAngle() * vtkMath::Pi() / 180.0;
  double w = Interactor->GetRenderWindow()->GetSize()[0];
  double h = Interactor->GetRenderWindow()->GetSize()[1];
  double scalex = w/h*((2*l*tan(view_angle/2))/2);
  double scaley =     ((2*l*tan(view_angle/2))/2);

  // construct the camera offset vector as function of delta mouse X & Y.
  cam->GetViewUp(upV);
  vtkMath::Cross(upV, atV, rightV);
  vtkMath::Cross(atV, rightV, upV); // (make sure 'upV' is orthogonal
                                    //  to 'atV' & 'rightV')
  vtkMath::Normalize(rightV);
  vtkMath::Normalize(upV);
  
  for(i=0; i<3; i++)
    {
    rightV[i] = rightV[i] * scalex;
    upV   [i] = upV   [i] * scaley;
    }
}

// 
// Rotate the camera by 'angle' degrees about the point <cx, cy, cz>
// and around the vector/axis <ax, ay, az>.
// 
void vtkInteractorStyleUnicam::MyRotateCamera(double cx, double cy, double cz,
                                              double ax, double ay, double az,
                                              double angle)
{
  angle *= 180.0 / vtkMath::Pi();   // vtk uses degrees, not radians

  double p[4], f[4], u[4];
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetPosition  (p);
  camera->GetFocalPoint(f);
  camera->GetViewUp    (u);
  p[3] = f[3] = 1.0; // (points)
  u[3] = 0.0;        // (a vector)

  vtkTransform *t = vtkTransform::New();
  t->PostMultiply();
  t->Identity();
  t->Translate(-cx, -cy, -cz);
  t->RotateWXYZ(angle, ax, ay, az);
  t->Translate( cx,  cy,  cz);

  double new_p[4], new_f[4];
  t->MultiplyPoint(p, new_p);
  t->MultiplyPoint(f, new_f);

  double new_u[4];
  t->Identity();
  t->RotateWXYZ(angle, ax, ay, az);
  t->MultiplyPoint(u, new_u);

  camera->SetPosition  (new_p[0], new_p[1], new_p[2]);
  camera->SetFocalPoint(new_f[0], new_f[1], new_f[2]);
  camera->SetViewUp    (new_u[0], new_u[1], new_u[2]);

  // IMPORTANT!  If you don't re-compute view plane normal, the camera
  // view gets all messed up.
  camera->ComputeViewPlaneNormal();
  t->Delete();
}

// Translate the camera by the offset <v[0], v[1], v[2]>.  Update
// the camera clipping range.
// 
void vtkInteractorStyleUnicam::MyTranslateCamera(double v[3])
{
  double p[3], f[3];
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetPosition  (p);
  camera->GetFocalPoint(f);

  double newP[3], newF[3];
  for(int i=0;i<3;i++) 
    {
    newP[i] = p[i] + v[i];
    newF[i] = f[i] + v[i];
    }

  camera->SetPosition  (newP);
  camera->SetFocalPoint(newF);

  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }
}

