/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStylePlane.cxx
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
#include "vtkInteractorStylePlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkOutlineSource.h"
#include "vtkMath.h" 
#include "vtkCellPicker.h"

//----------------------------------------------------------------------------
vtkInteractorStylePlane *vtkInteractorStylePlane::New() 
{
  return new vtkInteractorStylePlane;
}


//----------------------------------------------------------------------------
void vtkInteractorStylePlaneCallback(void *arg)
{
  vtkInteractorStylePlane *self = (vtkInteractorStylePlane *)arg;

  self->DefaultCallback(self->GetCallbackType());
}



//----------------------------------------------------------------------------
vtkInteractorStylePlane::vtkInteractorStylePlane() 
{
  this->SphereSource    = vtkSphereSource::New();
  this->SphereMapper    = vtkPolyDataMapper::New();
  this->SphereMapper->SetInput(this->SphereSource->GetOutput());
  this->SphereActor     = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);
  this->SphereActor->GetProperty()->SetColor(1.0, 0.7, 0.7);

  this->PlaneSource    = vtkPlaneSource::New();
  this->PlaneMapper    = vtkPolyDataMapper::New();
  this->PlaneMapper->SetInput(this->PlaneSource->GetOutput());
  this->PlaneActor     = vtkActor::New();
  this->PlaneActor->SetMapper(this->PlaneMapper);

  this->Button = -1;
  this->State = VTK_INTERACTOR_STYLE_PLANE_NONE;
  this->ActiveCornerId = -1;
  this->ActiveCorner[0] = this->ActiveCorner[1] = this->ActiveCorner[2] = 0.0;
  this->CallbackMethod = vtkInteractorStylePlaneCallback;
  this->CallbackMethodArg = (void *)this;
  this->CallbackMethodArgDelete = NULL;
  this->CallbackType = NULL;

  this->Transform = vtkTransform::New();
}

//----------------------------------------------------------------------------
vtkInteractorStylePlane::~vtkInteractorStylePlane() 
{
  if (this->CurrentRenderer) 
    { // just in case delete occurs while style is active.
    if (this->State != VTK_INTERACTOR_STYLE_PLANE_NONE)
      {
      this->CurrentRenderer->RemoveActor(this->SphereActor);
      }
    }
  this->SphereSource->Delete();
  this->SphereSource = NULL;
  this->SphereMapper->Delete();
  this->SphereMapper = NULL;
  this->SphereActor->Delete();
  this->SphereActor = NULL;

  this->PlaneSource->Delete();
  this->PlaneSource = NULL;
  this->PlaneMapper->Delete();
  this->PlaneMapper = NULL;
  this->PlaneActor->Delete();
  this->PlaneActor = NULL;

  if ((this->CallbackMethodArg)&&(this->CallbackMethodArgDelete))
    {
    (*this->CallbackMethodArgDelete)(this->CallbackMethodArg);
    }
  this->SetCallbackType(NULL);

  this->Transform->Delete();
  this->Transform = NULL;
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::SetCallbackMethod(void (*f)(void *), void *arg)
{
  if ( f != this->CallbackMethod || arg != this->CallbackMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->CallbackMethodArg)&&(this->CallbackMethodArgDelete))
      {
      (*this->CallbackMethodArgDelete)(this->CallbackMethodArg);
      }
    this->CallbackMethod = f;
    this->CallbackMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::SetCallbackMethodArgDelete(void (*f)(void *))
{
  if ( f != this->CallbackMethodArgDelete)
    {
    this->CallbackMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::DefaultCallback(char *type)
{
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::OnMouseMove(int vtkNotUsed(ctrl), 
					   int vtkNotUsed(shift),
					   int x, int y) 
{
  if (this->Button == -1)
    {
    this->HandleIndicator(x, y);
    }

  if (this->Button == 0 && this->State == VTK_INTERACTOR_STYLE_PLANE_CENTER)
    {
    this->RotateXY(x - this->LastPos[0], y - this->LastPos[1]);
    this->CurrentRenderer->ResetCameraClippingRange();
    this->CurrentRenderer->GetRenderWindow()->Render();
    }
  if (this->Button == 1 && this->State == VTK_INTERACTOR_STYLE_PLANE_CENTER)
    {
    this->TranslateXY(x - this->LastPos[0], y - this->LastPos[1]);
    this->CurrentRenderer->ResetCameraClippingRange();
    this->CurrentRenderer->GetRenderWindow()->Render();
    }
  if (this->Button == 2 && this->State == VTK_INTERACTOR_STYLE_PLANE_CENTER)
    {
    this->TranslateZ(x - this->LastPos[0], y - this->LastPos[1]);
    this->CurrentRenderer->ResetCameraClippingRange();
    this->CurrentRenderer->GetRenderWindow()->Render();
    }

  if (this->Button == 0 && this->State == VTK_INTERACTOR_STYLE_PLANE_CORNER)
    {
    this->ResizeCorner(x - this->LastPos[0], y - this->LastPos[1]);
    this->CurrentRenderer->ResetCameraClippingRange();
    this->CurrentRenderer->GetRenderWindow()->Render();
    }
  if (this->Button == 1 && this->State == VTK_INTERACTOR_STYLE_PLANE_CORNER)
    {
    this->DiamondCorner(x - this->LastPos[0], y - this->LastPos[1]);
    this->CurrentRenderer->ResetCameraClippingRange();
    this->CurrentRenderer->GetRenderWindow()->Render();
    }


  this->LastPos[0] = x;
  this->LastPos[1] = y;
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::ResizeCorner(int dx, int dy)
{
  float target[4], display[3];
  float point0[3], point1[3], point2[3], point3[3];

  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  this->CurrentRenderer->SetWorldPoint(this->ActiveCorner[0],
				       this->ActiveCorner[1],
				       this->ActiveCorner[2], 1.0);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);

  display[0] += dx;
  display[1] += dy;

  this->CurrentRenderer->SetDisplayPoint(display);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(target);
  target[0] /= target[3];
  target[1] /= target[3];
  target[2] /= target[3];

  // We have the new corner point, now what do we do with it?
  this->PlaneSource->GetOrigin(point0);
  this->PlaneSource->GetPoint1(point1);
  this->PlaneSource->GetPoint2(point2);
  point3[0] = point1[0] + point2[0] - point0[0];
  point3[1] = point1[1] + point2[1] - point0[1];
  point3[2] = point1[2] + point2[2] - point0[2];

  if (this->ActiveCornerId == 3)
    {
    this->ComputeParameters(point0, point1, point2, point3, target);
    this->SetActiveCorner(point3);
    this->PlaneSource->SetPoint1(point1);
    this->PlaneSource->SetPoint2(point2);
    }
  if (this->ActiveCornerId == 0)
    {
    this->ComputeParameters(point3, point1, point2, point0, target);
    this->SetActiveCorner(point0);
    this->PlaneSource->SetOrigin(point0);
    this->PlaneSource->SetPoint1(point1);
    this->PlaneSource->SetPoint2(point2);
    }
  if (this->ActiveCornerId == 1)
    {
    this->ComputeParameters(point2, point0, point3, point1, target);
    this->SetActiveCorner(point1);
    this->PlaneSource->SetOrigin(point0);
    this->PlaneSource->SetPoint1(point1);
    this->PlaneSource->SetPoint2(point2);
    }
  if (this->ActiveCornerId == 2)
    {
    this->ComputeParameters(point1, point0, point3, point2, target);
    this->SetActiveCorner(point2);
    this->PlaneSource->SetOrigin(point0);
    this->PlaneSource->SetPoint1(point1);
    this->PlaneSource->SetPoint2(point2);
    }

  this->SphereActor->SetPosition(this->ActiveCorner);
}


//----------------------------------------------------------------------------
void vtkInteractorStylePlane::ComputeParameters(float *p0, float *p1, 
						float *p2, float *p3,
						float *target3)
{
  double k1, k2;
  double v1[3], v2[3], vt[3];
  double m0[2], m1[2];
  double mi0[2], mi1[2];
  double *M[2], *MI[2];
  double c[2];
  int i;

  for (i = 0; i < 3; ++i)
    {
    v1[i] = p1[i] - p0[i];
    v2[i] = p2[i] - p0[i];
    vt[i] = target3[i] - p0[i];
    }
  M[0] = m0;
  M[1] = m1;
  MI[0] = mi0;
  MI[1] = mi1;

  m0[0] =         vtkMath::Dot(v1,v1);
  m0[1] = m1[0] = vtkMath::Dot(v1,v2);
  m1[1] =         vtkMath::Dot(v2,v2);
  c[0] =          vtkMath::Dot(vt, v1);
  c[1] =          vtkMath::Dot(vt, v2);

  //vtkErrorMacro("v1: " << v1[0] << ", " << v1[1] << ", " << v1[2]);
  //vtkErrorMacro("v2: " << v2[0] << ", " << v2[1] << ", " << v2[2]);
  //vtkErrorMacro("vt: " << vt[0] << ", " << vt[1] << ", " << vt[2]);

  vtkMath::InvertMatrix(M, MI, 2);

  k1 = c[0] * mi0[0] + c[1] * mi1[0];
  k2 = c[0] * mi0[1] + c[1] * mi1[1];
  for (i = 0; i < 3; ++i)
    {
    p3[i] = p0[i] + k1 * v1[i] + k2 * v2[i];
    p1[i] = p0[i] + k1 * v1[i];
    p2[i] = p0[i] + k2 * v2[i];
    }
}


//----------------------------------------------------------------------------
void vtkInteractorStylePlane::DiamondCorner(int dx, int dy)
{
  float newPoint[4], display[3];
  float origin[3], point1[3], point2[3];

  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  this->CurrentRenderer->SetWorldPoint(this->ActiveCorner[0],
				       this->ActiveCorner[1],
				       this->ActiveCorner[2], 1.0);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  //vtkErrorMacro("d1 : " << display[0] << ", " << display[1] << "," << display[2]);
  display[0] += dx;
  display[1] += dy;
  //vtkErrorMacro("d2 : " << display[0] << ", " << display[1] << "," << display[2]);
  this->CurrentRenderer->SetDisplayPoint(display);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(newPoint);
  newPoint[0] /= newPoint[3];
  newPoint[1] /= newPoint[3];
  newPoint[2] /= newPoint[3];

  this->SphereActor->SetPosition(newPoint);

  //vtkErrorMacro("Point: " << this->ActiveCorner[0] 
  //		<< ", " << this->ActiveCorner[1] 
  //		<< ", " << this->ActiveCorner[2] << ", to new: "
  //		<< newPoint[0] << ", " << newPoint[1] << ", " 
  //		<< newPoint[2]); 

  // We have the new corner point, now what do we do with it?
  this->PlaneSource->GetOrigin(origin);
  this->PlaneSource->GetPoint1(point1);
  this->PlaneSource->GetPoint2(point2);

  if (this->ActiveCornerId == 0)
    {
    origin[0] = newPoint[0];
    origin[1] = newPoint[1];
    origin[2] = newPoint[2];
    }

  if (this->ActiveCornerId == 3)
    {
    origin[0] += this->ActiveCorner[0] - newPoint[0];
    origin[1] += this->ActiveCorner[1] - newPoint[1];
    origin[2] += this->ActiveCorner[2] - newPoint[2];
    }

  if (this->ActiveCornerId == 1)
    {
    point2[0] += this->ActiveCorner[0] - newPoint[0];
    point2[1] += this->ActiveCorner[1] - newPoint[1];
    point2[2] += this->ActiveCorner[2] - newPoint[2];
    point1[0] = newPoint[0];
    point1[1] = newPoint[1];
    point1[2] = newPoint[2];
    }

  if (this->ActiveCornerId == 2)
    {
    point1[0] += this->ActiveCorner[0] - newPoint[0];
    point1[1] += this->ActiveCorner[1] - newPoint[1];
    point1[2] += this->ActiveCorner[2] - newPoint[2];
    point2[0] = newPoint[0];
    point2[1] = newPoint[1];
    point2[2] = newPoint[2];
    }

  //vtkErrorMacro("Active CornerID: " << this->ActiveCornerId);
  //vtkErrorMacro("origin: " << origin[0] << ", " << origin[1] << ", " 
  //		<< origin[2]);
  //vtkErrorMacro("pt1: " << point1[0] << ", " << point1[1] << ", " 
  //		<< point1[2]);
  //vtkErrorMacro("pt2: " << point2[0] << ", " << point2[1] << ", " 
  //		<< point2[2]);

  this->PlaneSource->SetOrigin(origin);
  this->PlaneSource->SetPoint1(point1);
  this->PlaneSource->SetPoint2(point2);

  this->ActiveCorner[0] = newPoint[0];
  this->ActiveCorner[1] = newPoint[1];
  this->ActiveCorner[2] = newPoint[2];

}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::RotateXY(int dx, int dy)
{
  vtkCamera *cam;
  double *vu, v2[3];
  double center[3];
  float n1[4], n2[4];
  int *size;
  
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  cam = this->CurrentRenderer->GetActiveCamera();
  size = this->CurrentRenderer->GetSize();
    
  // We do not have to translate to center of rotation because we will
  // apply this transform to the normal.
  this->Transform->Identity();

  // azimuth
  vu = cam->GetViewUp();	
  this->Transform->RotateWXYZ(360.0 * dx / size[0], vu[0], vu[1], vu[2]);
  // Elevation
  vtkMath::Cross(cam->GetDirectionOfProjection(), cam->GetViewUp(), v2);
  this->Transform->RotateWXYZ(-360.0 * dy / size[1], v2[0], v2[1], v2[2]);
    
  this->PlaneSource->GetNormal(n1);
  n1[3] = 1.0;
  this->Transform->MultiplyPoint(n1, n2);
  this->PlaneSource->SetNormal(n2);
}


//----------------------------------------------------------------------------
void vtkInteractorStylePlane::TranslateXY(int dx, int dy)
{
  float world[4], display[3];

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->PlaneSource->GetCenter(world);

  world[3] = 1.0;
  this->CurrentRenderer->SetWorldPoint(world);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  display[0] += dx;
  display[1] += dy;
  this->CurrentRenderer->SetDisplayPoint(display);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(world);
  world[0] /= world[3];
  world[1] /= world[3];
  world[2] /= world[3];
  this->PlaneSource->SetCenter(world[0], world[1], world[2]);
  this->SphereActor->SetPosition(world[0], world[1], world[2]);

  if ( this->CallbackMethod )
    {
    (*this->CallbackMethod)(this->CallbackMethodArg);
    }  
}



//----------------------------------------------------------------------------
void vtkInteractorStylePlane::TranslateZ(int vtkNotUsed(dx), int dy)
{
  vtkCamera *cam;
  float v1[4], d[4];
  float *center, dist;
  int *size;

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // compute distance from camera position to parts
  cam = this->CurrentRenderer->GetActiveCamera();
  cam->GetViewPlaneNormal(v1);
  cam->GetPosition(d);
  center = this->PlaneSource->GetCenter();
  d[0] = d[0] - center[0];
  d[1] = d[1] - center[1];
  d[2] = d[2] - center[2];

  dist = 2.0 * sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);

  size = this->CurrentRenderer->GetSize();
  v1[0] = v1[0] * dist / (float)(size[1]);
  v1[1] = v1[1] * dist / (float)(size[1]);
  v1[2] = v1[2] * dist / (float)(size[1]);

  this->PlaneSource->SetCenter(center[0]+dy*v1[0], center[1]+dy*v1[1], center[2]+dy*v1[2]);
  this->SphereActor->SetPosition(this->PlaneSource->GetCenter());
    
  if ( this->CallbackMethod )
    {
    (*this->CallbackMethod)(this->CallbackMethodArg);
    }  
}


//----------------------------------------------------------------------------
// This method handles display of active parameters.
// When the mouse is passively being moved over objects, this will
// highlight an object to indicate that it can be manipulated with the mouse.
void vtkInteractorStylePlane::HandleIndicator(int x, int y) 
{
  float display[3], point[4], corner[4], *origin;
  float sphereCx, sphereCy, sphereCz, sphereRad;
  float temp, centerDistDisplay, centerDistWorld, rad;
  int renderFlag = 0;
  float dist;
  int closestCornerId;
  float closestCorner[3];
  float closestCornerDist;

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // Are we over the center.
  this->PlaneSource->GetCenter(point);
  sphereCx = point[0];
  sphereCy = point[1];
  sphereCz = point[2];
  point[3] = 1.0;
  this->CurrentRenderer->SetWorldPoint(point);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  temp = (float)x - display[0];
  centerDistDisplay = temp * temp;
  temp = (float)y - display[1];
  centerDistDisplay += temp * temp;
  centerDistDisplay = sqrt(centerDistDisplay);

  // Compute the size of the sphere 
  // as a fraction of the plane radius.
  origin = this->PlaneSource->GetOrigin();
  temp = origin[0] - point[0];
  rad = temp * temp;
  temp = origin[1] - point[1];
  rad += temp * temp;
  temp = origin[2] - point[2];
  rad += temp * temp;
  rad = sqrt(rad);
  this->SphereActor->SetScale(rad * 0.1);

  // Find the closest corner and its distance.
  corner[3] = 1.0;
  this->PlaneSource->GetOrigin(corner);
  this->CurrentRenderer->SetWorldPoint(corner);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  temp = (float)x - display[0];
  dist = temp * temp;
  temp = (float)y - display[1];
  dist += temp * temp;
  dist = sqrt(dist);
  closestCornerId = 0;
  closestCorner[0] = corner[0];
  closestCorner[1] = corner[1];
  closestCorner[2] = corner[2];
  closestCornerDist = dist;

  this->PlaneSource->GetPoint1(corner);
  this->CurrentRenderer->SetWorldPoint(corner);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  temp = (float)x - display[0];
  dist = temp * temp;
  temp = (float)y - display[1];
  dist += temp * temp;
  dist = sqrt(dist);
  if (dist < closestCornerDist)
    {
    closestCornerId = 1;
    closestCorner[0] = corner[0];
    closestCorner[1] = corner[1];
    closestCorner[2] = corner[2];
    closestCornerDist = dist;
    }

  this->PlaneSource->GetPoint2(corner);
  this->CurrentRenderer->SetWorldPoint(corner);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  temp = (float)x - display[0];
  dist = temp * temp;
  temp = (float)y - display[1];
  dist += temp * temp;
  dist = sqrt(dist);
  if (dist < closestCornerDist)
    {
    closestCornerId = 2;
    closestCorner[0] = corner[0];
    closestCorner[1] = corner[1];
    closestCorner[2] = corner[2];
    closestCornerDist = dist;
    }

  // The forth point we have to compute.
  this->PlaneSource->GetPoint1(point);
  corner[0] = corner[0] + point[0] - origin[0];
  corner[1] = corner[1] + point[1] - origin[1];
  corner[2] = corner[2] + point[2] - origin[2];
  this->CurrentRenderer->SetWorldPoint(corner);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  temp = (float)x - display[0];
  dist = temp * temp;
  temp = (float)y - display[1];
  dist += temp * temp;
  dist = sqrt(dist);
  if (dist < closestCornerDist)
    {
    closestCornerId = 3;
    closestCorner[0] = corner[0];
    closestCorner[1] = corner[1];
    closestCorner[2] = corner[2];
    closestCornerDist = dist;
    }

  // If we are within a couple pixles of the center, turn it on.
  if (centerDistDisplay < 16.0 && centerDistDisplay < closestCornerDist)
    { // The center should become active.
    if (this->State == VTK_INTERACTOR_STYLE_PLANE_NONE)
      { // Center was previously off. Turn it on.
      this->CurrentRenderer->AddActor(this->SphereActor);
      }
    if (this->State != VTK_INTERACTOR_STYLE_PLANE_CENTER)
      {
      this->SphereActor->SetPosition(this->PlaneActor->GetCenter());
      this->CurrentRenderer->ResetCameraClippingRange();
      this->CurrentRenderer->GetRenderWindow()->Render();
      }
    this->State = VTK_INTERACTOR_STYLE_PLANE_CENTER;
    this->ActiveCornerId = -1;
    return;
    }

  if (closestCornerDist < 16.0)
    { // The closest corner should become active.
    if (this->State == VTK_INTERACTOR_STYLE_PLANE_NONE)
      { // Center was previously off. Turn it on.
      this->CurrentRenderer->AddActor(this->SphereActor);
      }
    if (this->ActiveCornerId != closestCornerId)
      {
      this->SphereActor->SetPosition(closestCorner);
      this->CurrentRenderer->ResetCameraClippingRange();
      this->CurrentRenderer->GetRenderWindow()->Render();
      }
    this->State = VTK_INTERACTOR_STYLE_PLANE_CORNER;
    this->ActiveCornerId = closestCornerId;
    this->ActiveCorner[0] = closestCorner[0];
    this->ActiveCorner[1] = closestCorner[1];
    this->ActiveCorner[2] = closestCorner[2];
    return;
    }

  // Neither is on.
  if (this->State != VTK_INTERACTOR_STYLE_PLANE_NONE)
    { // Center was previously on. Turn it off.
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    this->CurrentRenderer->GetRenderWindow()->Render();
    this->State = VTK_INTERACTOR_STYLE_PLANE_NONE;
    this->ActiveCornerId = -1;
    }

  return;

}



//----------------------------------------------------------------------------
void vtkInteractorStylePlane::OnLeftButtonDown(int ctrl, int shift, 
						int x, int y) 
{
  
  this->UpdateInternalState(ctrl, shift, x, y);

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (this->State == VTK_INTERACTOR_STYLE_PLANE_NONE)
    { // deactivate the interactor.
    this->Button = -2;
    return;
    }

  this->Button = 0;
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::OnLeftButtonUp(int ctrl, int shift, 
					      int x, int y) 
{
  this->UpdateInternalState(ctrl, shift, x, y);
  this->Button = -1;
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::OnMiddleButtonDown(int ctrl, int shift, 
						 int x, int y) 
{
  this->UpdateInternalState(ctrl, shift, x, y);

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (this->State == VTK_INTERACTOR_STYLE_PLANE_NONE)
    { // deactivate the interactor.
    this->Button = -2;
    return;
    }

  this->Button = 1;
}
//----------------------------------------------------------------------------
void vtkInteractorStylePlane::OnMiddleButtonUp(int ctrl, int shift, 
					       int x, int y) 
{
  this->UpdateInternalState(ctrl, shift, x, y);
  this->Button = -1;
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::OnRightButtonDown(int ctrl, int shift, 
						int x, int y) 
{
  this->UpdateInternalState(ctrl, shift, x, y);

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (this->State == VTK_INTERACTOR_STYLE_PLANE_NONE)
    { // deactivate the interactor.
    this->Button = -2;
    return;
    }

  this->Button = 2;
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::OnRightButtonUp(int ctrl, int shift, 
					      int x, int y) 
{
  this->UpdateInternalState(ctrl, shift, x, y);
  this->Button = -1;
}

//----------------------------------------------------------------------------
void vtkInteractorStylePlane::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyle::PrintSelf(os,indent);

  os << indent << "CallbackType: " << this->CallbackType << endl;
  os << indent << "PlaneSource: (" << this->PlaneSource << ")\n";
  os << indent << "PlaneActor: (" << this->PlaneActor << ")\n";

  if ( this->CallbackMethod )
    {
    os << indent << "Callback Method defined\n";
    }
  else
    {
    os << indent <<"No Callback Method\n";
    }
}












