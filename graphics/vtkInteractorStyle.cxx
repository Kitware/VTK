/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyle.cxx
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
#include "vtkInteractorStyle.h"
#include "vtkPolyDataMapper.h"
#include "vtkOutlineSource.h"
#include "vtkMath.h" 
#include "vtkCellPicker.h"
#include "vtkAssemblyNode.h"
#include "vtkCommand.h"

//----------------------------------------------------------------------------
vtkInteractorStyle *vtkInteractorStyle::New() 
{
  return new vtkInteractorStyle;
}

//----------------------------------------------------------------------------
vtkInteractorStyle::vtkInteractorStyle() 
{
  this->Interactor       = NULL;
  this->CurrentCamera    = NULL;
  this->CurrentLight     = NULL;
  this->CurrentRenderer  = NULL;
  this->Outline          = vtkOutlineSource::New();
  
  this->OutlineActor     = NULL;
  this->OutlineMapper    = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInput(this->Outline->GetOutput());
  this->PickedRenderer   = NULL;
  this->CurrentProp      = NULL;
  this->PropPicked      = 0;
  this->Center[0] = this->Center[1] = 0.0;
  this->PickColor[0] = 1.0; this->PickColor[1] = 0.0; this->PickColor[2] = 0.0;
  this->PickedActor2D = NULL;

  this->State    = VTKIS_START;
  this->AnimState = VTKIS_ANIM_OFF; 
  this->CtrlKey  = 0;
  this->ShiftKey = 1;

  this->LeftButtonPressTag = 0;
  this->LeftButtonReleaseTag = 0;
  this->MiddleButtonPressTag = 0;
  this->MiddleButtonReleaseTag = 0;
  this->RightButtonPressTag = 0;
  this->RightButtonReleaseTag = 0;
  
  this->LastPos[0] = this->LastPos[1] = 0;
}

//----------------------------------------------------------------------------
vtkInteractorStyle::~vtkInteractorStyle() 
{
  if ( this->OutlineActor ) 
    {
    // if we change style when an object is selected, we must remove the
    // actor from the renderer
    if (this->CurrentRenderer) 
      {
      this->CurrentRenderer->RemoveActor(this->OutlineActor);
      }
    this->OutlineActor->Delete();
    }
  if ( this->OutlineMapper ) 
    {
    this->OutlineMapper->Delete();
    }
  this->Outline->Delete();
  this->Outline = NULL;

  if ( this->CurrentRenderer)
    {
    this->CurrentRenderer->UnRegister(this);
    this->CurrentRenderer = NULL;
    }

}

// Set the left button pressed method. This method is invoked on a left mouse button press.
void vtkInteractorStyle::SetLeftButtonPressMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->LeftButtonPressTag);
  this->LeftButtonPressTag = this->AddObserver(vtkCommand::LeftButtonPressEvent,cbc);
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetLeftButtonPressMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->LeftButtonPressTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}


void vtkInteractorStyle::SetLeftButtonReleaseMethod(void (*f)(void *), 
                                                    void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->LeftButtonReleaseTag);
  this->LeftButtonReleaseTag = this->AddObserver(vtkCommand::LeftButtonReleaseEvent,cbc);
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetLeftButtonReleaseMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->LeftButtonReleaseTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}


void vtkInteractorStyle::SetMiddleButtonPressMethod(void (*f)(void *), 
                                                    void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->MiddleButtonPressTag);
  this->MiddleButtonPressTag = this->AddObserver(vtkCommand::MiddleButtonPressEvent,cbc);
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetMiddleButtonPressMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->MiddleButtonPressTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Set the exit method. This method is invoked on a <e> keyrelease.
void vtkInteractorStyle::SetMiddleButtonReleaseMethod(void (*f)(void *), 
                                                      void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->MiddleButtonReleaseTag);
  this->MiddleButtonPressTag = this->AddObserver(vtkCommand::MiddleButtonReleaseEvent,cbc);
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetMiddleButtonReleaseMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->MiddleButtonReleaseTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Set the exit method. This method is invoked on a <e> keypress.
void vtkInteractorStyle::SetRightButtonPressMethod(void (*f)(void *), 
                                                   void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->RightButtonPressTag);
  this->RightButtonPressTag = this->AddObserver(vtkCommand::RightButtonPressEvent,cbc);
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetRightButtonPressMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->RightButtonPressTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Set the exit method. This method is invoked on a <e> keyrelease.
void vtkInteractorStyle::SetRightButtonReleaseMethod(void (*f)(void *), 
                                                     void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->RightButtonReleaseTag);
  this->RightButtonReleaseTag = this->AddObserver(vtkCommand::RightButtonReleaseEvent,cbc);
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetRightButtonReleaseMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->RightButtonReleaseTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyle::FindPokedRenderer(int x,int y) 
{
  vtkRendererCollection *rc;
  vtkRenderer *aren;
  vtkRenderer *interactiveren = NULL, *viewportren = NULL;
  int numRens, i;
  if (this->CurrentRenderer)
    {
    this->CurrentRenderer->UnRegister(this);
    }
  this->CurrentRenderer = NULL;

  rc = this->Interactor->GetRenderWindow()->GetRenderers();
  numRens = rc->GetNumberOfItems();
  
  for (i = numRens -1; (i >= 0) && !this->CurrentRenderer; i--) 
    {
    aren = (vtkRenderer *)rc->GetItemAsObject(i);
    if (aren->IsInViewport(x,y) && aren->GetInteractive()) 
      {
      this->CurrentRenderer = aren;
      this->CurrentRenderer->Register(this);
      }

    if (interactiveren == NULL && aren->GetInteractive())
      {
      // Save this renderer in case we can't find one in the viewport that
      // is interactive.
      interactiveren = aren;
      }
    if (viewportren == NULL && aren->IsInViewport(x, y))
      {
      // Save this renderer in case we can't find one in the viewport that 
      // is interactive.
      viewportren = aren;
      }
    }
  
  // We must have a value.  If we found an interactive renderer before, that's
  // better than a non-interactive renderer.
  if (this->CurrentRenderer == NULL)
    {
    this->CurrentRenderer = interactiveren;
    this->CurrentRenderer->Register(this);
    }
  
  // We must have a value.  If we found a renderer that is in the viewport,
  // that is better than any old viewport (but not as good as an interactive
  // one).
  if (this->CurrentRenderer == NULL)
    {
    this->CurrentRenderer = viewportren;
    this->CurrentRenderer->Register(this);
    }

  // We must have a value - take anything.
  if (this->CurrentRenderer == NULL) 
    {
    rc->InitTraversal();
    aren = rc->GetNextItem();
    this->CurrentRenderer = aren;
    this->CurrentRenderer->Register(this);
    }
}

//----------------------------------------------------------------------------
void  vtkInteractorStyle::FindPokedCamera(int x,int y) 
{
  float *vp;
  vtkLightCollection *lc;
  int *Size = this->Interactor->GetSize();

  this->FindPokedRenderer(x,y);
  vp = this->CurrentRenderer->GetViewport();
  
  this->CurrentCamera = this->CurrentRenderer->GetActiveCamera();
  this->Center[0] = this->CurrentRenderer->GetCenter()[0];
  this->Center[1] = this->CurrentRenderer->GetCenter()[1];
  this->DeltaElevation = -20.0/((vp[3] - vp[1])*Size[1]);
  this->DeltaAzimuth = -20.0/((vp[2] - vp[0])*Size[0]);
  
  // as a side effect also set the light
  // in case they are using light follow camera
  lc = this->CurrentRenderer->GetLights();
  lc->InitTraversal();
  this->CurrentLight = lc->GetNextItem();
}

void vtkInteractorStyle::HighlightProp(vtkProp *prop) 
{
  this->CurrentProp = prop;

  if ( prop != NULL )
    {
    vtkActor2D *actor2D;
    vtkProp3D *prop3D;
    if ( (prop3D=vtkProp3D::SafeDownCast(prop)) != NULL )
      {
      this->HighlightProp3D(prop3D);
      }
    else if ( (actor2D=vtkActor2D::SafeDownCast(prop)) != NULL )
      {
      this->HighlightActor2D(actor2D);
      }
    }
  else
    {//unhighlight everything, both 2D & 3D
    this->HighlightProp3D(NULL);
    this->HighlightActor2D(NULL);
    }
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
// When pick action successfully selects a vtkProp3Dactor, this method
// highlights the vtkProp3D appropriately. Currently this is done by placing a
// bounding box around the vtkProp3D.
void vtkInteractorStyle::HighlightProp3D(vtkProp3D *prop3D) 
{
  if ( ! this->OutlineActor ) 
    {
    // have to defer creation to get right type
    this->OutlineActor = vtkActor::New();
    this->OutlineActor->PickableOff();
    this->OutlineActor->DragableOff();
    this->OutlineActor->SetMapper(this->OutlineMapper);
    this->OutlineActor->GetProperty()->SetColor(1.0,1.0,1.0);
    this->OutlineActor->GetProperty()->SetAmbient(1.0);
    this->OutlineActor->GetProperty()->SetDiffuse(0.0);
    this->CurrentRenderer->AddActor(this->OutlineActor);
    }
  
  if ( ! prop3D ) 
    {
    this->PickedRenderer = NULL;
    this->OutlineActor->VisibilityOff();
    }
  else 
    {
    this->PickedRenderer = this->CurrentRenderer;
    this->Outline->SetBounds(prop3D->GetBounds());
    this->OutlineActor->VisibilityOn();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyle::HighlightActor2D(vtkActor2D *actor2D) 
{
  // If nothing has changed, just return
  if ( actor2D == this->PickedActor2D )
    {
    return;
    }

  if ( actor2D )
    {
    if ( this->PickedActor2D )
      {
      actor2D->GetProperty()->SetColor(
        this->PickedActor2D->GetProperty()->GetColor());
      this->PickedActor2D->GetProperty()->SetColor(this->PickColor);
      }
    else
      {
      float tmpColor[3];
      actor2D->GetProperty()->GetColor(tmpColor);
      actor2D->GetProperty()->SetColor(this->PickColor);
      this->PickColor[0] = tmpColor[0];
      this->PickColor[1] = tmpColor[1];
      this->PickColor[2] = tmpColor[2];
      }
    }
  else
    {
    if ( this->PickedActor2D )
      {
      float tmpColor[3];
      this->PickedActor2D->GetProperty()->GetColor(tmpColor);
      this->PickedActor2D->GetProperty()->SetColor(this->PickColor);
      this->PickColor[0] = tmpColor[0];
      this->PickColor[1] = tmpColor[1];
      this->PickColor[2] = tmpColor[2];
      }
    }
  
  this->PickedActor2D = actor2D;
}

//----------------------------------------------------------------------------
void vtkInteractorStyle::UpdateInternalState(int ctrl, int shift, 
                                             int X, int Y) 
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
  this->Interactor->SetEventPosition(X, Y);
}

//----------------------------------------------------------------------------
// Implementation of motion state control methods
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StartState(int newstate) 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  this->State = newstate;
  if (this->AnimState == VTKIS_ANIM_OFF) 
    {
    rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());
    if ( !rwi->CreateTimer(VTKI_TIMER_FIRST) ) 
      {
      vtkErrorMacro(<< "Timer start failed");
      this->State = VTKIS_START;
      }
    }
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StopState() 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  this->State = VTKIS_START;
  if (this->AnimState == VTKIS_ANIM_OFF) 
    {   
    rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
    rwi->Render();
    if ( !rwi->DestroyTimer() ) 
      {
      vtkErrorMacro(<< "Timer stop failed");
      }
    }   
}

//----------------------------------------------------------------------------
// JCP animation control 
void  vtkInteractorStyle::StartAnimate() 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
            vtkErrorMacro(<< "starting animation");
  this->AnimState = VTKIS_ANIM_ON;
  if (this->State == VTKIS_START) 
    {   
    vtkErrorMacro(<< "Start state found");
    rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());
    if ( !rwi->CreateTimer(VTKI_TIMER_FIRST) ) 
      {
      vtkErrorMacro(<< "Timer start failed");
      }
    }   
  rwi->Render();
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StopAnimate() 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  this->AnimState = VTKIS_ANIM_OFF;
  if (this->State == VTKIS_START) 
    {   
    rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
    if ( !rwi->DestroyTimer() ) 
      {
      vtkErrorMacro(<< "Timer stop failed");
      }
    }   
}

// JCP Animation control
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StartRotate() 
{
  if (this->State != VTKIS_START) 
    {
    return;
    }
  this->StartState(VTKIS_ROTATE);
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::EndRotate() 
{
  if (this->State != VTKIS_ROTATE) 
    {
    return;
    }
  this->StopState();
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StartZoom() 
{
  if (this->State != VTKIS_START) 
    {
    return;
    }
  this->StartState(VTKIS_ZOOM);
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::EndZoom() 
{
  if (this->State != VTKIS_ZOOM) 
    {
    return;
    }
  this->StopState();
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StartPan() 
{
  if (this->State != VTKIS_START) 
    {
    return;
    }
  this->StartState(VTKIS_PAN);
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::EndPan() 
{
  if (this->State != VTKIS_PAN) 
    {
    return;
    }
  this->StopState();
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StartSpin() 
{
  if (this->State != VTKIS_START) 
    {
    return;
    }
  this->StartState(VTKIS_SPIN);
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::EndSpin() 
{
  if (this->State != VTKIS_SPIN) 
    {
    return;
    }
  this->StopState();
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StartDolly() 
{
  if (this->State != VTKIS_START) 
    {
    return;
    }
  this->StartState(VTKIS_DOLLY);
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::EndDolly() 
{
    if (this->State != VTKIS_DOLLY) 
      {
      return;
      }
    this->StopState();
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StartUniformScale() 
{
  if (this->State != VTKIS_START) 
    {
    return;
    }
  this->StartState(VTKIS_USCALE);
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::EndUniformScale() 
{
  if (this->State != VTKIS_USCALE) 
    {
    return;
    }
  this->StopState();
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StartTimer() 
{
  if (this->State != VTKIS_START) 
    {
    return;
    }
  this->StartState(VTKIS_TIMER);
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::EndTimer() 
{
  if (this->State != VTKIS_TIMER) 
    {
    return;
    }
  this->StopState();
}
//----------------------------------------------------------------------------
// Intercept any keypresses which are style independent here and do the rest in
// subclasses - none really required yet!
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnKeyDown(int ctrl, int shift, char vtkNotUsed(keycode), int vtkNotUsed(repeatcount))
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
}
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnKeyUp  (int ctrl, int shift, char vtkNotUsed(keycode), int vtkNotUsed(repeatcount)) 
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
}
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnKeyPress(int ctrl, int shift, char vtkNotUsed(keycode), char *vtkNotUsed(keysym), int vtkNotUsed(repeatcount))
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
}
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnKeyRelease(int ctrl, int shift, char vtkNotUsed(keycode), char *vtkNotUsed(keysym), int vtkNotUsed(repeatcount)) 
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
}
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnChar(int ctrl, int shift, 
                                char keycode, int vtkNotUsed(repeatcount)) 
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
  
  vtkRenderWindowInteractor *rwi = this->Interactor;
  switch (keycode) 
    {
    // JCP Animation control
    case 'a' :
    case 'A' :
      if (this->AnimState == VTKIS_ANIM_OFF) 
        {
        this->StartAnimate();
        }
      else 
        {
        this->StopAnimate();
        }
      break;
      // JCP Animation control 
      //-----
    case 'Q' :
    case 'q' :
    case 'e' :
    case 'E' :
      rwi->ExitCallback();
      break;
      //-----
    case 'u' :
    case 'U' :
      rwi->UserCallback();
      break;
      //-----
    case 'r' :
    case 'R' :
      this->FindPokedRenderer(this->LastPos[0], this->LastPos[1]);
      this->CurrentRenderer->ResetCamera();
      rwi->Render();
      break;
      //-----
    case 'w' :
    case 'W' :
    {
    vtkActorCollection *ac;
    vtkActor *anActor, *aPart;
    vtkAssemblyPath *path;
    this->FindPokedRenderer(this->LastPos[0],this->LastPos[1]);
    ac = this->CurrentRenderer->GetActors();
    for (ac->InitTraversal(); (anActor = ac->GetNextItem()); ) 
      {
      for (anActor->InitPathTraversal(); (path=anActor->GetNextPath()); ) 
        {
        aPart=(vtkActor *)path->GetLastNode()->GetProp();
        aPart->GetProperty()->SetRepresentationToWireframe();
        }
      }
    rwi->Render();
    }
    break;
    //-----
    case 's' :
    case 'S' :
    {
    vtkActorCollection *ac;
    vtkActor *anActor, *aPart;
    vtkAssemblyPath *path;
    this->FindPokedRenderer(this->LastPos[0],this->LastPos[1]);
    ac = this->CurrentRenderer->GetActors();
    for (ac->InitTraversal(); (anActor = ac->GetNextItem()); ) 
      {
      for (anActor->InitPathTraversal(); (path=anActor->GetNextPath()); ) 
        {
        aPart=(vtkActor *)path->GetLastNode()->GetProp();
        aPart->GetProperty()->SetRepresentationToSurface();
        }
      }
    rwi->Render();
    }
    break;
    //-----
    case '3' :
      if (rwi->GetRenderWindow()->GetStereoRender()) 
        {
        rwi->GetRenderWindow()->StereoRenderOff();
        }
      else 
        {
        rwi->GetRenderWindow()->StereoRenderOn();
        }
      rwi->Render();
      break;
      //-----
    case 'p' :
    case 'P' :
      if (this->State == VTKIS_START) 
        {
        vtkAssemblyPath *path=NULL;
        this->FindPokedRenderer(this->LastPos[0],this->LastPos[1]);
        rwi->StartPickCallback();
        rwi->GetPicker()->Pick(this->LastPos[0],this->LastPos[1], 0.0, 
                               this->CurrentRenderer);
        vtkAbstractPropPicker *picker;
        if ( (picker=vtkAbstractPropPicker::SafeDownCast(rwi->GetPicker())) )
          {
          path = picker->GetPath();
          }
        if ( path == NULL )
          {
          this->HighlightProp(NULL);
          this->PropPicked = 0;
          }
        else
          {
          this->HighlightProp(path->GetFirstNode()->GetProp());
          this->PropPicked = 1;
          }
        rwi->EndPickCallback();
        }
      break;
    }
}

//----------------------------------------------------------------------------
// Overriding these method allows you to specify some special action to
// be taken when the window size is modified, or when the mouse
// enters or leaves the window.
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnConfigure(int vtkNotUsed(width), 
				     int vtkNotUsed(height))
{
}
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnEnter(int ctrl, int shift, int vtkNotUsed(y), int vtkNotUsed(x))
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
}
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnLeave(int ctrl, int shift, int vtkNotUsed(y), int vtkNotUsed(x))
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
}


//----------------------------------------------------------------------------
// By overriding the RotateCamera, RotateActor members we can
// use this timer routine for Joystick or Trackball - quite tidy
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnTimer(void) 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  switch (this->State) 
    {
    //-----
    case VTKIS_START:
      // JCP Animation control
      if (this->AnimState == VTKIS_ANIM_ON)
        {
                rwi->DestroyTimer();
                rwi->Render();
                rwi->CreateTimer(VTKI_TIMER_FIRST);
         }
      // JCP Animation control 
      break;
      //-----
    case VTKIS_ROTATE:  // rotate with respect to an axis perp to look
      this->RotateCamera(this->LastPos[0], this->LastPos[1]);
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    case VTKIS_PAN: // move perpendicular to camera's look vector
      this->PanCamera(this->LastPos[0], this->LastPos[1]);
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    case VTKIS_ZOOM:
      this->DollyCamera(this->LastPos[0], this->LastPos[1]);
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    case VTKIS_SPIN:
      this->SpinCamera(this->LastPos[0], this->LastPos[1]);
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    case VTKIS_DOLLY:  // move along camera's view vector
      break;
      //-----
    case VTKIS_USCALE:
      break;
      //-----
    case VTKIS_TIMER:
                rwi->Render();
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    default :

      break;
    }
}

//----------------------------------------------------------------------------
// Mouse events are identical for trackball and joystick mode
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnMouseMove(int vtkNotUsed(ctrl), int vtkNotUsed(shift),
                                    int X, int Y) 
{
  this->LastPos[0] = X;
  this->LastPos[1] = Y;
  if (this->HasObserver(vtkCommand::MouseMoveEvent)) 
    {
    this->InvokeEvent(vtkCommand::MouseMoveEvent,NULL);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyle::OnLeftButtonDown(int ctrl, int shift, 
                                          int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
  this->FindPokedCamera(X, Y);
  if (this->HasObserver(vtkCommand::LeftButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    }
  else 
    {
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
void vtkInteractorStyle::OnLeftButtonUp(int ctrl, int shift, int X, int Y) 
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
}

//----------------------------------------------------------------------------
void vtkInteractorStyle::OnMiddleButtonDown(int ctrl, int shift, 
                                            int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
  this->FindPokedCamera(X, Y);
  //
  if (this->HasObserver(vtkCommand::MiddleButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);
    }
  else 
    {
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
void vtkInteractorStyle::OnMiddleButtonUp(int ctrl, int shift, 
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
}

//----------------------------------------------------------------------------
void vtkInteractorStyle::OnRightButtonDown(int ctrl, int shift, int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
 this->FindPokedCamera(X, Y);
  if (this->HasObserver(vtkCommand::RightButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);
    }
  else 
    {
    this->StartZoom();
    }
}
//----------------------------------------------------------------------------
void vtkInteractorStyle::OnRightButtonUp(int ctrl, int shift, int X, int Y) 
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
    this->EndZoom();
    }
}

// NOTE!!! This does not do any reference counting!!!
// This is to avoid some ugly reference counting loops 
// and the benefit of being able to hold only an entire
// renderwindow from an interactor style doesn't seem worth the
// mess. 
void vtkInteractorStyle::SetInteractor(vtkRenderWindowInteractor *i)
{
  this->Interactor = i;
}

// Description:
// transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void vtkInteractorStyle::ComputeDisplayToWorld(double x, double y,
                                                      double z,
                                                      float *worldPt)
{
  this->CurrentRenderer->SetDisplayPoint(x, y, z);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(worldPt);
  if (worldPt[3])
    {
    worldPt[0] /= worldPt[3];
    worldPt[1] /= worldPt[3];
    worldPt[2] /= worldPt[3];
    worldPt[3] = 1.0;
    }
}

// Description:
// transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void vtkInteractorStyle::ComputeDisplayToWorld(double x, double y,
                                                      double z,
                                                      double *worldPt)
{
  this->CurrentRenderer->SetDisplayPoint(x, y, z);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(worldPt);
  if (worldPt[3])
    {
    worldPt[0] /= worldPt[3];
    worldPt[1] /= worldPt[3];
    worldPt[2] /= worldPt[3];
    worldPt[3] = 1.0;
    }
}


// Description:
// transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void vtkInteractorStyle::ComputeWorldToDisplay(double x, double y,
                                                      double z,
                                                      double *displayPt)
{
  this->CurrentRenderer->SetWorldPoint(x, y, z, 1.0);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(displayPt);
}

// Description:
// transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void vtkInteractorStyle::ComputeWorldToDisplay(double x, double y,
                                                      double z,
                                                      float *displayPt)
{
  this->CurrentRenderer->SetWorldPoint(x, y, z, 1.0);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(displayPt);
}

//----------------------------------------------------------------------------
// Implementations of Joystick Camera/Actor motions follow
//----------------------------------------------------------------------------

// This functionality of RotateCamera, SpinCamera, PanCamera, and DollyCamera
// is now handled by vtkInteractorStyleJoystickCamera.
void vtkInteractorStyle::RotateCamera(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  double rxf = (double)(x - this->Center[0]) * this->DeltaAzimuth;
  double ryf = (double)(y - this->Center[1]) * this->DeltaElevation;

  this->CurrentCamera->Azimuth(rxf);
  this->CurrentCamera->Elevation(ryf);
  this->CurrentCamera->OrthogonalizeViewUp();
  this->CurrentRenderer->ResetCameraClippingRange();
  if (rwi->GetLightFollowCamera())
    {
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }
  rwi->Render();
}

void vtkInteractorStyle::SpinCamera(int vtkNotUsed(x), int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  // spin is based on y value
  double yf = (double)(y - this->Center[1]) / (double)(this->Center[1]);
  if (yf > 1)
    {
    yf = 1;
    }
  else if (yf < -1)
    {
    yf = -1;
    }

  double newAngle = asin(yf) * 180.0 / 3.1415926;

  this->CurrentCamera->Roll(newAngle);
  this->CurrentCamera->OrthogonalizeViewUp();
  rwi->Render();
}

void vtkInteractorStyle::PanCamera(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  double ViewFocus[4];
  
  // calculate the focal depth since we'll be using it a lot
  this->CurrentCamera->GetFocalPoint(ViewFocus);
  this->ComputeWorldToDisplay(ViewFocus[0], ViewFocus[1],
                              ViewFocus[2], ViewFocus);
  double focalDepth = ViewFocus[2];

  double NewPickPoint[4];
  this->ComputeDisplayToWorld((float)x, (float)y,
                              focalDepth, NewPickPoint);

  // get the current focal point and position
  this->CurrentCamera->GetFocalPoint(ViewFocus);
  double *ViewPoint = this->CurrentCamera->GetPosition();

  /*
   * Compute a translation vector, moving everything 1/10
   * the distance to the cursor. (Arbitrary scale factor)
   */
  double MotionVector[3];
  MotionVector[0] = 0.1*(ViewFocus[0] - NewPickPoint[0]);
  MotionVector[1] = 0.1*(ViewFocus[1] - NewPickPoint[1]);
  MotionVector[2] = 0.1*(ViewFocus[2] - NewPickPoint[2]);

  this->CurrentCamera->SetFocalPoint(MotionVector[0] + ViewFocus[0],
                                     MotionVector[1] + ViewFocus[1],
                                     MotionVector[2] + ViewFocus[2]);
  this->CurrentCamera->SetPosition(MotionVector[0] + ViewPoint[0],
                                   MotionVector[1] + ViewPoint[1],
                                   MotionVector[2] + ViewPoint[2]);

  if (rwi->GetLightFollowCamera())
    {
    /* get the first light */
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }
  rwi->Render();
}

void vtkInteractorStyle::DollyCamera(int vtkNotUsed(x), int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  double dyf = 0.5 * (double)(y - this->Center[1]) /
    (double)(this->Center[1]);
  double zoomFactor = pow((double)1.1, dyf);
  if (zoomFactor < 0.5 || zoomFactor > 1.5)
    {
    vtkErrorMacro("Bad zoom factor encountered");
    }
  
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

  if (rwi->GetLightFollowCamera())
    {
    /* get the first light */
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }
  rwi->Render();
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void vtkInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Pick Color: (" << this->PickColor[0] << ", "
     << this->PickColor[1] << ", "
     << this->PickColor[2] << ")\n";

  os << indent << "CurrentCamera:   " << this->CurrentCamera << "\n";
  os << indent << "CurrentLight:    " << this->CurrentLight << "\n";
  os << indent << "CurrentRenderer: " << this->CurrentRenderer << "\n";
  os << indent << "Viewport Center: " << "( " << this->Center[0] <<
    ", " << this->Center[1] << " )\n";
  if ( this->PickedRenderer )
    {
    os << indent << "Picked Renderer: " << this->PickedRenderer << "\n";
    }
  else
    {
    os << indent << "Picked Renderer: (none)\n";
    }
  if ( this->CurrentProp )
    {
    os << indent << "Current Prop: " << this->CurrentProp << "\n";
    }
  else
    {
    os << indent << "Current Actor: (none)\n";
    }

  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Prop Picked: " <<
    (this->PropPicked ? "Yes\n" : "No\n");
  
  if (this->LeftButtonPressTag)
    {
    os << indent << "LeftButtonPressMethod: Defined\n";
    }
  if (this->LeftButtonReleaseTag)
    {
    os << indent << "LeftButtonReleaseMethod: Defined\n";
    }
  
  if (this->MiddleButtonPressTag)
    {
    os << indent << "MiddleButtonPressMethod: Defined\n";
    }
  if (this->MiddleButtonReleaseTag)
    {
    os << indent << "MiddleButtonReleaseMethod: Defined\n";
    }
  
  if (this->RightButtonPressTag)
    {
    os << indent << "RightButtonPressMethod: Defined\n";
    }
  if (this->RightButtonReleaseTag)
    {
    os << indent << "RightButtonReleaseMethod: Defined\n";
    }
}


