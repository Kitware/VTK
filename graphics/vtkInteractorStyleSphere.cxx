/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSphere.cxx
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
#include "vtkInteractorStyleSphere.h"
#include "vtkPolyDataMapper.h"
#include "vtkOutlineSource.h"
#include "vtkMath.h" 
#include "vtkCellPicker.h"

//----------------------------------------------------------------------------
vtkInteractorStyleSphere *vtkInteractorStyleSphere::New() 
{
  return new vtkInteractorStyleSphere;
}

//----------------------------------------------------------------------------
vtkInteractorStyleSphere::vtkInteractorStyleSphere() 
{
  this->SphereSource    = vtkSphereSource::New();
  this->SphereMapper    = vtkPolyDataMapper::New();
  this->SphereMapper->SetInput(this->SphereSource->GetOutput());
  this->SphereActor     = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

  this->CenterSource    = vtkAxes::New();
  this->CenterSource->SymmetricOn();
  this->CenterMapper    = vtkPolyDataMapper::New();
  this->CenterMapper->SetInput(this->CenterSource->GetOutput());
  this->CenterActor     = vtkActor::New();
  this->CenterActor->SetMapper(this->CenterMapper);

  this->Button = -1;
  this->State = VTK_INTERACTOR_STYLE_SPHERE_NONE;
  this->ChangeMethod = NULL;
  this->ChangeMethodArg = NULL;
  this->ChangeMethodArgDelete = NULL;
}

//----------------------------------------------------------------------------
vtkInteractorStyleSphere::~vtkInteractorStyleSphere() 
{
  if (this->CurrentRenderer) 
    { // just in case delete occurs while style is active.
    if (this->State == VTK_INTERACTOR_STYLE_SPHERE_RADIUS)
      {
      this->CurrentRenderer->RemoveActor(this->SphereActor);
      }
    if (this->State == VTK_INTERACTOR_STYLE_SPHERE_CENTER)
      {
      this->CurrentRenderer->RemoveActor(this->CenterActor);
      }
    }
  this->SphereSource->Delete();
  this->SphereSource = NULL;
  this->SphereMapper->Delete();
  this->SphereMapper = NULL;
  this->SphereActor->Delete();
  this->SphereActor = NULL;

  this->CenterSource->Delete();
  this->CenterSource = NULL;
  this->CenterMapper->Delete();
  this->CenterMapper = NULL;
  this->CenterActor->Delete();
  this->CenterActor = NULL;

  if ((this->ChangeMethodArg)&&(this->ChangeMethodArgDelete))
    {
    (*this->ChangeMethodArgDelete)(this->ChangeMethodArg);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::SetChangeMethod(void (*f)(void *), void *arg)
{
  if ( f != this->ChangeMethod || arg != this->ChangeMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->ChangeMethodArg)&&(this->ChangeMethodArgDelete))
      {
      (*this->ChangeMethodArgDelete)(this->ChangeMethodArg);
      }
    this->ChangeMethod = f;
    this->ChangeMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::SetChangeMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ChangeMethodArgDelete)
    {
    this->ChangeMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::SetCenter(float x, float y, float z)
{
  this->SphereSource->SetCenter(x, y, z);
  this->CenterActor->SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
float *vtkInteractorStyleSphere::GetCenter()
{
  return this->SphereSource->GetCenter();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::SetRadius(float rad)
{
  this->SphereSource->SetRadius(rad);
}

//----------------------------------------------------------------------------
float vtkInteractorStyleSphere::GetRadius()
{
  return this->SphereSource->GetRadius();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::OnMouseMove(int vtkNotUsed(ctrl), 
					   int vtkNotUsed(shift),
					   int x, int y) 
{
  if (this->Button == -1)
    {
    this->HandleIndicator(x, y);
    }

  if (this->Button == 0 && this->State == VTK_INTERACTOR_STYLE_SPHERE_CENTER)
    {
    this->MoveCenterXY(x - this->LastPos[0], y - this->LastPos[1]);
    }
  if (this->Button == 2 && this->State == VTK_INTERACTOR_STYLE_SPHERE_CENTER)
    {
    this->MoveCenterZ(x - this->LastPos[0], y - this->LastPos[1]);
    }
  if (this->Button >= 0 && this->State == VTK_INTERACTOR_STYLE_SPHERE_RADIUS)
    {
    this->MoveRadius(x, y);
    }

  this->LastPos[0] = x;
  this->LastPos[1] = y;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::MoveCenterXY(int dx, int dy)
{
  float world[4], display[3];

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->SphereSource->GetCenter(world);
  world[3] = 1.0;
  this->CurrentRenderer->SetWorldPoint(world);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  display[0] += dx;
  display[1] += dy;
  this->CurrentRenderer->SetDisplayPoint(display);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(world);
  this->SetCenter(world[0]/world[3], world[1]/world[3], world[2]/world[3]);
  
  if ( this->ChangeMethod )
    {
    (*this->ChangeMethod)(this->ChangeMethodArg);
    }  

  if (this->CurrentRenderer)
    {
    this->CurrentRenderer->GetRenderWindow()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::MoveRadius(int x, int y)
{
  float center[4], world[4], display[3], dist;

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->SphereSource->GetCenter(center);
  center[3] = 1.0;
  this->CurrentRenderer->SetWorldPoint(center);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  display[0] = x;
  display[1] = y;
  this->CurrentRenderer->SetDisplayPoint(display);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(world);

  world[0] = world[0]/world[3] - center[0];
  world[1] = world[1]/world[3] - center[1];
  world[2] = world[2]/world[3] - center[2];
  dist = world[0]*world[0] + world[1]*world[1] + world[2]*world[2];
  dist = sqrt(dist);
  this->SetRadius(dist);
  
  if ( this->ChangeMethod )
    {
    (*this->ChangeMethod)(this->ChangeMethodArg);
    }  

  this->CurrentRenderer->ResetCameraClippingRange();
  this->CurrentRenderer->GetRenderWindow()->Render();
}


//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::MoveCenterZ(int dx, int dy)
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
  cam->ComputeViewPlaneNormal();
  cam->GetViewPlaneNormal(v1);
  cam->GetPosition(d);
  center = this->GetCenter();
  d[0] = d[0] - center[0];
  d[1] = d[1] - center[1];
  d[2] = d[2] - center[2];

  dist = 2.0 * sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);

  size = this->CurrentRenderer->GetSize();
  v1[0] = v1[0] * dist / (float)(size[1]);
  v1[1] = v1[1] * dist / (float)(size[1]);
  v1[2] = v1[2] * dist / (float)(size[1]);

  this->SetCenter(center[0]+dy*v1[0], center[1]+dy*v1[1], center[2]+dy*v1[2]);
    
  if ( this->ChangeMethod )
    {
    (*this->ChangeMethod)(this->ChangeMethodArg);
    }  
  this->CurrentRenderer->ResetCameraClippingRange();
  this->CurrentRenderer->GetRenderWindow()->Render();
}


//----------------------------------------------------------------------------
// This method handles display of active parameters.
// When the mouse is passively being moved over objects, this will
// highlight an object to indicate that it can be manipulated with the mouse.
void vtkInteractorStyleSphere::HandleIndicator(int x, int y) 
{
  float display[3], world[4];
  float sphereCx, sphereCy, sphereCz, sphereRad;
  float temp, centerDistDisplay, centerDistWorld;
  int renderFlag = 0;

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // Are we over the center.
  this->SphereSource->GetCenter(world);
  sphereCx = world[0];
  sphereCy = world[1];
  sphereCz = world[2];
  world[3] = 1.0;
  this->CurrentRenderer->SetWorldPoint(world);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(display);
  temp = (float)x - display[0];
  centerDistDisplay = temp * temp;
  temp = (float)y - display[1];
  centerDistDisplay += temp * temp;
  centerDistDisplay = sqrt(centerDistDisplay);

  // Now handle the Radius (the z value of the display is already set).
  sphereRad = this->SphereSource->GetRadius();
  display[0] = (float)x;
  display[1] = (float)y;
  this->CurrentRenderer->SetDisplayPoint(display);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(world);
  temp = sphereCx - world[0];
  centerDistWorld = temp * temp;
  temp = sphereCy - world[1];
  centerDistWorld += temp * temp;
  temp = sphereCz - world[2];
  centerDistWorld += temp * temp;
  centerDistWorld = sqrt(centerDistWorld);


  // In order for the shell to be active, the cursor must be outside the shell
  // and within four pixles.
  if (centerDistWorld > sphereRad && centerDistWorld > 0.0 &&
      (centerDistWorld-sphereRad)*centerDistDisplay/centerDistWorld < 8.0)
    { // The shell should become active.
    if (this->State == VTK_INTERACTOR_STYLE_SPHERE_CENTER)
      { // Center was previously on. Turn it off.
      this->CurrentRenderer->RemoveActor(this->CenterActor);
      renderFlag = 1;
      }
    if (this->State != VTK_INTERACTOR_STYLE_SPHERE_RADIUS)
      { // The shell actor needs to be turned on.
      this->CurrentRenderer->AddActor(this->SphereActor);
      renderFlag = 1;
      }
    if (renderFlag)
      {
      this->CurrentRenderer->GetRenderWindow()->Render();
      }
    this->State = VTK_INTERACTOR_STYLE_SPHERE_RADIUS;
    return;
    }

  // If we are within a couple pixles of the center, turn it on.
  if (centerDistDisplay < 8.0)
    { // The center should become active.
    if (this->State != VTK_INTERACTOR_STYLE_SPHERE_CENTER)
      { // Center was previously off. Turn it on.
      this->CurrentRenderer->AddActor(this->CenterActor);
      renderFlag = 1;
      }
    if (this->State == VTK_INTERACTOR_STYLE_SPHERE_RADIUS)
      { // The shell actor needs to be turned off.
      this->CurrentRenderer->RemoveActor(this->SphereActor);
      renderFlag = 1;
      }
    if (renderFlag)
      {
      this->CurrentRenderer->GetRenderWindow()->Render();
      }
    this->State = VTK_INTERACTOR_STYLE_SPHERE_CENTER;
    return;
    }

  // Neither is on.
  if (this->State == VTK_INTERACTOR_STYLE_SPHERE_CENTER)
    { // Center was previously on. Turn it off.
    this->CurrentRenderer->RemoveActor(this->CenterActor);
    renderFlag = 1;
    }
  if (this->State == VTK_INTERACTOR_STYLE_SPHERE_RADIUS)
    { // The shell actor needs to be turned off.
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    renderFlag = 1;
    }
  if (renderFlag)
    {
    this->CurrentRenderer->GetRenderWindow()->Render();
    }

  this->State = VTK_INTERACTOR_STYLE_SPHERE_NONE;

  return;

}



//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::OnLeftButtonDown(int ctrl, int shift, 
						int x, int y) 
{
  
  this->UpdateInternalState(ctrl, shift, x, y);

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (this->State == VTK_INTERACTOR_STYLE_SPHERE_NONE)
    { // deactivate the interactor.
    this->Button = -2;
    return;
    }

  this->Button = 0;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::OnLeftButtonUp(int ctrl, int shift, 
					      int x, int y) 
{
  this->Button = -1;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::OnMiddleButtonDown(int ctrl, int shift, 
						  int x, int y) 
{
}
//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::OnMiddleButtonUp(int ctrl, int shift, 
						int x, int y) 
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::OnRightButtonDown(int ctrl, int shift, 
						 int x, int y) 
{
  this->UpdateInternalState(ctrl, shift, x, y);

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (this->State == VTK_INTERACTOR_STYLE_SPHERE_NONE)
    { // deactivate the interactor.
    this->Button = -2;
    return;
    }

  this->Button = 2;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::OnRightButtonUp(int ctrl, int shift, 
					       int X, int Y) 
{
  this->Button = -1;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSphere::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyle::PrintSelf(os,indent);

  float *center = this->SphereSource->GetCenter();
  os << indent << "Sphere Center: " << center[0] << ", " << center[1] 
     << ", " << center[2] << endl;
  os << indent << "Sphere Radius: " << this->SphereSource->GetRadius() <<endl;
  if ( this->ChangeMethod )
    {
    os << indent << "Change Method defined\n";
    }
  else
    {
    os << indent <<"No Change Method\n";
    }
}












