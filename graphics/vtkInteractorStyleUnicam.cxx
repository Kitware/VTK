/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleUnicam.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    contributed by A. Forsberg, Brown University, December 2000

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
#include "vtkInteractorStyleUnicam.h"
#include "vtkMath.h"
#include "vtkWorldPointPicker.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"

#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"

//--------------------------------------------------------------------------
vtkInteractorStyleUnicam* vtkInteractorStyleUnicam::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret=vtkObjectFactory::CreateInstance("vtkInteractorStyleUnicam");
  if(ret)
    {
    return (vtkInteractorStyleUnicam*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleUnicam;
}

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
    sphere->SetThetaResolution(6); sphere->SetPhiResolution(6);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInput(sphere->GetOutput());

  // XXX - would like to make the focus sphere not be affected by
  // XXX - the lights-- i.e., always be easily easily seen.  i'm not sure
  // XXX - how to do that.
  this->FocusSphere = vtkActor::New();
    this->FocusSphere->SetMapper(sphereMapper);
    this->FocusSphere->GetProperty()->SetColor(0.8900,0.6600,0.4100);
    this->FocusSphere->GetProperty()->SetRepresentationToWireframe();

  // set WorldUpVector to be z-axis by default
  WorldUpVector[0] = 0;
  WorldUpVector[1] = 0;
  WorldUpVector[2] = 1;
}

vtkInteractorStyleUnicam::~vtkInteractorStyleUnicam() 
{
  this->InteractionPicker->Delete();
}

void vtkInteractorStyleUnicam::PrintSelf(ostream& os, vtkIndent indent) 
{
  this->vtkInteractorStyle::PrintSelf(os,indent);

  os << indent << "Interaction Picker: " << this->InteractionPicker;
//   os << indent << "WorldUpVector: " << this->WorldUpVector;
}

void vtkInteractorStyleUnicam::OnTimer(void) 
{
  if (this->ButtonDown != VTK_UNICAM_NONE) 
    {
    // restart timer-- we want to keep getting 'OnMouseMove' events
    this->Interactor->CreateTimer(VTKI_TIMER_UPDATE);
    }
}

void vtkInteractorStyleUnicam::SetWorldUpVector(float x, float y, float z) 
{
  WorldUpVector[0] = x;
  WorldUpVector[1] = y;
  WorldUpVector[2] = z;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::OnLeftButtonDown(int ctrl, int shift, 
                                                    int X, int Y) 
{
  this->ButtonDown = VTK_UNICAM_BUTTON_LEFT;
  this->Interactor->CreateTimer(VTKI_TIMER_UPDATE);

  this->DTime    = TheTime();
  this->Dist     = 0;

  // cam manip init
  float curpt[2];
  NormalizeMouseXY(X, Y, &curpt[0], &curpt[1]);
  this->LastPos[0] = curpt[0];
  this->LastPos[1] = curpt[1];

  this->StartPix[0] = this->LastPix[0] = X;
  this->StartPix[1] = this->LastPix[1] = Y;

  // find 'this->DownPt'  (point in world space under the cursor tip)
  this->FindPokedRenderer(X, Y);
  this->InteractionPicker->Pick(X, Y, 0.0, this->CurrentRenderer);
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
float vtkInteractorStyleUnicam::WindowAspect()
{
  float w = Interactor->GetRenderWindow()->GetSize()[0];
  float h = Interactor->GetRenderWindow()->GetSize()[1];

  return w/h;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::NormalizeMouseXY(int X, int Y,
						float *NX, float *NY)
{
  float w = Interactor->GetRenderWindow()->GetSize()[0];
  float h = Interactor->GetRenderWindow()->GetSize()[1];

  *NX = -1.0 + 2.0 * float(X) / w;
  *NY = -1.0 + 2.0 * float(Y) / h;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::OnMouseMove(int ctrl, int shift, int X, int Y)
{
  // filter out any repeated events
  static int last_X = 0;
  static int last_Y = 0;
  if (X == last_X && Y == last_Y)
    {
    return;
    }

  // channel event to right method handler.
  switch (this->ButtonDown) 
    {
    case VTK_UNICAM_BUTTON_LEFT:
      OnLeftButtonMove  (ctrl, shift, X, Y);
      break;
    case VTK_UNICAM_BUTTON_MIDDLE:
      OnMiddleButtonMove(ctrl, shift, X, Y);
      break;
    case VTK_UNICAM_BUTTON_RIGHT:
      OnRightButtonMove (ctrl, shift, X, Y);
      break;
    }

  last_X = X;
  last_Y = Y;

  this->Interactor->Render();  // re-draw scene.. it should have changed
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::OnLeftButtonUp(int ctrl, int shift, 
					       int X, int Y) 
{
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
      this->FocusSphere->SetPosition(this->DownPt);

      float from[3];
      this->FindPokedCamera(X, Y);
      this->CurrentCamera->GetPosition(from);

      float vec[3];
      vec[0] = this->DownPt[0] - from[0];
      vec[1] = this->DownPt[1] - from[1];
      vec[2] = this->DownPt[2] - from[2];

      float at_v[4];
      at_v[0] = 0;
      at_v[1] = 0;
      at_v[2] = 1;
      at_v[3] = 0;
      this->CameraToWorld(at_v, at_v);
      vtkMath::Normalize(at_v);

      // calculate scale so focus sphere always is the same size on the screen
      double s = 0.02 * vtkMath::Dot(at_v, vec);

      this->FocusSphere->SetScale   (s, s, s);

      this->FindPokedRenderer(X, Y);
      this->FocusSphereRenderer = this->CurrentRenderer;
      this->FocusSphereRenderer->AddActor(this->FocusSphere);

      this->IsDot = 1;
      }
    this->Interactor->Render();
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
  rwi->Render();
  rwi->DestroyTimer();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::OnLeftButtonMove(int ctrl, int shift,
						 int X, int Y)
{
  switch (state) 
    {
    case VTK_UNICAM_CAM_INT_CHOOSE:   this->Choose(X, Y); break;
    case VTK_UNICAM_CAM_INT_ROT:      this->Rotate(X, Y); break;
    case VTK_UNICAM_CAM_INT_PAN:      this->Pan(X, Y); break;
    case VTK_UNICAM_CAM_INT_ZOOM:     this->Zoom(X, Y); break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::Choose( int X, int Y )
{
  int   te[2];  // pixel location
  te[0] = X;
  te[1] = Y;

  float curpt[2];
  NormalizeMouseXY(X, Y, &curpt[0], &curpt[1]);
  
  float delta[2];
  delta[0] = curpt[0] - this->LastPos[0];
  delta[1] = curpt[1] - this->LastPos[1];
  this->LastPos[0] = te[0];
  this->LastPos[1] = te[1];

  double tdelt(TheTime() - this->DTime);

  this->Dist += sqrt(delta[0] * delta[0] + delta[1] * delta[1]);

  float sdelt[2];
  sdelt[0] = te[0] - this->StartPix[0];
  sdelt[1] = te[1] - this->StartPix[1];

  int xa=0,ya=1;
  if (getenv("FLIP_CAM_MANIP")) 
    {
    int tmp = xa;
    xa = ya;
    ya = xa;
    }
     
  float len = sqrt(sdelt[0] * sdelt[0] + sdelt[1] * sdelt[1]);
  if (fabs(sdelt[ya])/len > 0.9 && tdelt > 0.05) 
    {
    state = VTK_UNICAM_CAM_INT_ZOOM;
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
      state = VTK_UNICAM_CAM_INT_ZOOM;
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
void vtkInteractorStyleUnicam::Rotate( int X, int Y )
{
  float cpt[3];
  float center[3];
  this->FocusSphere->GetPosition(center);
  this->ComputeWorldToDisplay(center[0], center[1], center[2], cpt);
  NormalizeMouseXY(cpt[0], cpt[1], &cpt[0], &cpt[1]);

  double      radsq = pow(1+fabs(cpt[0]),2); // squared rad of virtual cylinder

  float tp[2], te[2];
  NormalizeMouseXY(this->LastPix[0], this->LastPix[1], &tp[0], &tp[1]);
  NormalizeMouseXY(X, Y, &te[0], &te[1]);
  this->LastPix[0] = X;
  this->LastPix[1] = Y;

  float op[3], oe[3];
  op[0] = tp[0];
  op[1] = 0;
  op[2] = 0;
  oe[0] = te[0];
  oe[1] = 0;
  oe[2] = 0;

  double opsq = op[0] * op[0], oesq = oe[0] * oe[0];

  double lop  = opsq > radsq ? 0 : sqrt(radsq - opsq);
  double loe  = oesq > radsq ? 0 : sqrt(radsq - oesq);

  float nop[3], noe[3];
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
    this->FindPokedCamera(X, Y);

    double angle = -2*acos(clamp(dot,-1.,1.)) * Sign(te[0]-tp[0]);

    float UPvec[3];
    UPvec[0] = WorldUpVector[0];
    UPvec[1] = WorldUpVector[1];
    UPvec[2] = WorldUpVector[2];
    vtkMath::Normalize(UPvec);

    MyRotateCamera(center[0], center[1], center[2], 
                   UPvec[0], UPvec[1], UPvec[2],
                   angle);

    float dvec[3];
    float from[3];
    this->CurrentCamera->GetPosition(from);
    for(int i=0; i<3; i++)
      {
      dvec[i] = from[i] - center[i];
      }
     
    double rdist = te[1]-tp[1];
    vtkMath::Normalize(dvec);
    double tdist = acos(clamp((float)vtkMath::Dot(UPvec, dvec), (float)-1., (float)1.));

    float rightV[4];
    rightV[0] = 1.0;
    rightV[1] = 0.0;
    rightV[2] = 0.0;
    rightV[3] = 0.0;
    this->CameraToWorld(rightV, rightV);
    vtkMath::Normalize(rightV);

    MyRotateCamera(center [0], center [1], center [2],
                   rightV[0], rightV[1], rightV[2],
                   rdist);

    this->CurrentCamera->SetViewUp(UPvec[0], UPvec[1], UPvec[2]);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::Zoom( int X, int Y )
{
  int i;
  float cn[2], ln[2];
  NormalizeMouseXY(X, Y, &cn[0], &cn[1]);
  NormalizeMouseXY(this->LastPix[0], this->LastPix[1], &ln[0], &ln[1]);

  float delta[2];
  delta[0] = cn[0] - ln[0];
  delta[1] = cn[1] - ln[1];
  this->LastPix[0] = X;
  this->LastPix[1] = Y;

  double     ratio;

  // 1. handle zooming
  // XXX - assume perspective projection for now.
  float from[3];
  this->FindPokedCamera(X, Y);
  this->CurrentCamera->GetPosition(from);
  
  float movec[3];
  for(i=0; i<3; i++)
    {
    movec[i] = this->DownPt[i] - from[i];
    }

  float movec_n[3];  // normalized movec
  for(i=0; i<3; i++)
    {
    movec_n[i] = movec[i];
    }
  vtkMath::Normalize(movec_n);

  for(i=0; i<3; i++)
    {
    this->Center[i] = this->DownPt[i];
    }

  float trans1[3];
  for(i=0; i<3; i++)
    {
    trans1[i] = movec_n[i] * (vtkMath::Norm(movec) * delta[1] * -4);
    }

  this->MyTranslateCamera(trans1);

  // 2. now handle side-to-side panning
  float at_v[4];
  at_v[0] = 0.0;
  at_v[1] = 0.0;
  at_v[2] = 1.0;
  at_v[3] = 0.0;
  this->CameraToWorld(at_v, at_v);

  float vec[3];
  for(i=0; i<3; i++)
    {
    vec[i] = movec[i] * at_v[i];
    }
  double l = vtkMath::Norm(vec);

  double view_angle = this->CurrentCamera->GetViewAngle() * vtkMath::Pi() / 180.0;
  ratio = WindowAspect() * (2.0 * (tan(view_angle) * l)) / 2.0;

  float rightV[4];
  rightV[0] = 1.0;
  rightV[1] = 0.0;
  rightV[2] = 0.0;
  rightV[3] = 0.0;
  this->CameraToWorld(rightV, rightV);
  vtkMath::Normalize(rightV);

  float trans2[3];
  for(i=0; i<3; i++)
    {
    trans2[i] = -delta[0]/2 * rightV[i] * ratio;
    }

  this->MyTranslateCamera(trans2);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleUnicam::Pan( int X, int Y )
{
  float delta[2];
  float cn[2], ln[2];
  int i;

  NormalizeMouseXY(X, Y, &cn[0], &cn[1]);
  NormalizeMouseXY(this->LastPix[0], this->LastPix[1], &ln[0], &ln[1]);
  delta[0] = cn[0] - ln[0];
  delta[1] = cn[1] - ln[1];
  this->LastPix[0] = X;
  this->LastPix[1] = Y;

  double     ratio;
  // XXX - assume perspective projection for now
  float from[3];
  this->FindPokedCamera(X, Y);
  this->CurrentCamera->GetPosition(from);

  float movec[3];
  for(i=0; i<3; i++)
    {
    movec[i] = this->DownPt[i] - from[i];
    }

  float movec_n[3];  // normalized movec
  movec_n[0] = movec[0];
  movec_n[1] = movec[1];
  movec_n[2] = movec[2];
  vtkMath::Normalize(movec_n);

  float at_v[4];
  at_v[0] = 0.0;
  at_v[1] = 0.0;
  at_v[2] = 1.0;
  at_v[3] = 0.0;
  this->CameraToWorld(at_v, at_v);

  float vec[3];
  for(i=0; i<3; i++)
    {
    vec[i] = movec[i] * at_v[i];
    }
  double l = vtkMath::Norm(vec);

  double view_angle = this->CurrentCamera->GetViewAngle() * vtkMath::Pi() / 180.0;
  ratio = WindowAspect() * (2.0 * l * tan(view_angle)) / 2.0;

  float rightV[4], upV[4];
  rightV[0] = 1.0;
  rightV[1] = 0.0;
  rightV[2] = 0.0;
  rightV[3] = 0.0;
  upV[0] = 0.0;
  upV[1] = 1.0;
  upV[2] = 0.0;
  upV[3] = 0.0;
  this->CameraToWorld(rightV, rightV);
  this->CameraToWorld(upV   , upV   );
  vtkMath::Normalize(rightV);
  vtkMath::Normalize(upV);

  float trans[3];
  for(i=0; i<3; i++)
    {
    trans[i] = (-delta[0]/2 * rightV[i] * ratio +
		-delta[1]/2 * upV[i]    * ratio);
    }

  this->MyTranslateCamera(trans);
}

//
// transform the point <in[0], in[1], in[2]> from camera space to 
// world space.
// 
void vtkInteractorStyleUnicam::CameraToWorld(float *in, float *out)
{
  float *o = this->CurrentCamera->GetOrientationWXYZ();

  vtkTransform *t = vtkTransform::New();
  t->PostMultiply();
  t->Identity();
  t->RotateWXYZ(o[0], o[1], o[2], o[3]);

  t->MultiplyPoint(in, out);

  t->Delete();
}

// 
// Rotate the camera by 'angle' degrees about the point <cx, cy, cz>
// and around the vector/axis <ax, ay, az>.
// 
void vtkInteractorStyleUnicam::MyRotateCamera(float cx, float cy, float cz,
					      float ax, float ay, float az,
					      float angle)
{
  angle *= 180.0 / vtkMath::Pi();   // vtk uses degrees, not radians

  float p[4], f[4], u[4];
  this->CurrentCamera->GetPosition  (p);
  this->CurrentCamera->GetFocalPoint(f);
  this->CurrentCamera->GetViewUp    (u);
  p[3] = f[3] = 1.0; // (points)
  u[3] = 0.0;        // (a vector)

  vtkTransform *t = vtkTransform::New();
  t->PostMultiply();
  t->Identity();
  t->Translate(-cx, -cy, -cz);
  t->RotateWXYZ(angle, ax, ay, az);
  t->Translate( cx,  cy,  cz);

  float new_p[4], new_f[4];
  t->MultiplyPoint(p, new_p);
  t->MultiplyPoint(f, new_f);

  float new_u[4];
  t->Identity();
  t->RotateWXYZ(angle, ax, ay, az);
  t->MultiplyPoint(u, new_u);

  this->CurrentCamera->SetPosition  (new_p[0], new_p[1], new_p[2]);
  this->CurrentCamera->SetFocalPoint(new_f[0], new_f[1], new_f[2]);
  this->CurrentCamera->SetViewUp    (new_u[0], new_u[1], new_u[2]);

  // IMPORTANT!  If you don't re-compute view plane normal, the camera
  // view gets all messed up.
  this->CurrentCamera->ComputeViewPlaneNormal();

  t->Delete();
}

// Translate the camera by the offset <v[0], v[1], v[2]>.  Update
// the camera clipping range.
// 
void vtkInteractorStyleUnicam::MyTranslateCamera(float v[3])
{
  float p[3], f[3];
  this->CurrentCamera->GetPosition  (p);
  this->CurrentCamera->GetFocalPoint(f);

  float newP[3], newF[3];
  for(int i=0;i<3;i++) 
    {
    newP[i] = p[i] + v[i];
    newF[i] = f[i] + v[i];
    }

  this->CurrentCamera->SetPosition  (newP);
  this->CurrentCamera->SetFocalPoint(newF);

  this->CurrentRenderer->ResetCameraClippingRange();
}
