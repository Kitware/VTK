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

  this->Plane          = vtkPlane::New();

  this->Button = -1;
  this->State = VTK_INTERACTOR_STYLE_PLANE_NONE;
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

  this->Plane->Delete();
  this->Plane       = NULL;

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
  type = type;
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

  this->LastPos[0] = x;
  this->LastPos[1] = y;
}


//----------------------------------------------------------------------------
void vtkInteractorStylePlane::RotateXY(int dx, int dy)
{
  vtkCamera *cam;
  double *vu, v2[3];
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
  vtkMath::Cross(cam->GetViewPlaneNormal(), cam->GetViewUp(), v2);
  this->Transform->RotateWXYZ(360.0 * dy / size[1], v2[0], v2[1], v2[2]);
    
  this->Plane->GetNormal(n1);
  n1[3] = 1.0;
  this->Transform->MultiplyPoint(n1, n2);
  this->Plane->SetNormal(n2);
}


//----------------------------------------------------------------------------
void vtkInteractorStylePlane::TranslateXY(int dx, int dy)
{
  float world[4], display[3];

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->Plane->GetOrigin(world);

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
  this->Plane->SetOrigin(world[0], world[1], world[2]);
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
  center = this->Plane->GetOrigin();
  d[0] = d[0] - center[0];
  d[1] = d[1] - center[1];
  d[2] = d[2] - center[2];

  dist = 2.0 * sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);

  size = this->CurrentRenderer->GetSize();
  v1[0] = v1[0] * dist / (float)(size[1]);
  v1[1] = v1[1] * dist / (float)(size[1]);
  v1[2] = v1[2] * dist / (float)(size[1]);

  this->Plane->SetOrigin(center[0]+dy*v1[0], center[1]+dy*v1[1], center[2]+dy*v1[2]);
  this->SphereActor->SetPosition(this->Plane->GetOrigin());
    
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
  int *size;
  float display[3], point[4];
  float sphereCx, sphereCy, sphereCz;
  float temp, centerDistDisplay, rad;
  int renderFlag = 0;

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // Are we over the center.
  this->Plane->GetOrigin(point);
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
  // as a fraction of the renderers size.
  // (Display is set to the origin in display coordinates.)
  size = this->CurrentRenderer->GetSize();
  display[0] += (float)(size[0] + size[1]) / 50.0;
  this->CurrentRenderer->SetDisplayPoint(display);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(point);
  temp = sphereCx - point[0];
  rad = temp * temp;
  temp = sphereCy - point[1];
  rad += temp * temp;
  temp = sphereCz - point[2];
  rad += temp * temp;
  rad = sqrt(rad);
  this->SphereActor->SetScale(rad);

  // If we are within a couple pixles of the center, turn it on.
  if (centerDistDisplay < 16.0)
    { // The center should become active.
    if (this->State == VTK_INTERACTOR_STYLE_PLANE_NONE)
      { // Center was previously off. Turn it on.
      this->CurrentRenderer->AddActor(this->SphereActor);
      }
    if (this->State != VTK_INTERACTOR_STYLE_PLANE_CENTER)
      {
      this->SphereActor->SetPosition(sphereCx, sphereCy, sphereCz);
      this->CurrentRenderer->ResetCameraClippingRange();
      this->CurrentRenderer->GetRenderWindow()->Render();
      }
    this->State = VTK_INTERACTOR_STYLE_PLANE_CENTER;
    return;
    }

  // Neither is on.
  if (this->State != VTK_INTERACTOR_STYLE_PLANE_NONE)
    { // Center was previously on. Turn it off.
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    this->CurrentRenderer->GetRenderWindow()->Render();
    this->State = VTK_INTERACTOR_STYLE_PLANE_NONE;
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
  os << indent << "Plane: (" << this->Plane << ")\n";

  if ( this->CallbackMethod )
    {
    os << indent << "Callback Method defined\n";
    }
  else
    {
    os << indent <<"No Callback Method\n";
    }
}












