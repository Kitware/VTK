/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyle.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
authors and that existing copyright notices are retained in all copies.
Some
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
#include "vtkInteractorStyle.h"
#include "vtkPolyDataMapper.h"
#include "vtkOutlineSource.h"
#include "vtkMath.h" 
#include "vtkCellPicker.h"

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
  this->CurrentActor     = NULL;

  this->State    = VTKIS_START;
  this->CtrlKey  = 0;
  this->ShiftKey = 1;

  this->LeftButtonPressMethod = NULL;
  this->LeftButtonPressMethodArgDelete = NULL;
  this->LeftButtonPressMethodArg = NULL;
  this->LeftButtonReleaseMethod = NULL;
  this->LeftButtonReleaseMethodArgDelete = NULL;
  this->LeftButtonReleaseMethodArg = NULL;

  this->MiddleButtonPressMethod = NULL;
  this->MiddleButtonPressMethodArgDelete = NULL;
  this->MiddleButtonPressMethodArg = NULL;
  this->MiddleButtonReleaseMethod = NULL;
  this->MiddleButtonReleaseMethodArgDelete = NULL;
  this->MiddleButtonReleaseMethodArg = NULL;

  this->RightButtonPressMethod = NULL;
  this->RightButtonPressMethodArgDelete = NULL;
  this->RightButtonPressMethodArg = NULL;
  this->RightButtonReleaseMethod = NULL;
  this->RightButtonReleaseMethodArgDelete = NULL;
  this->RightButtonReleaseMethodArg = NULL;
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

  if ((this->LeftButtonPressMethodArg)&&(this->LeftButtonPressMethodArgDelete))
    {
    (*this->LeftButtonPressMethodArgDelete)(this->LeftButtonPressMethodArg);
    }
  if ((this->LeftButtonReleaseMethodArg)&&
      (this->LeftButtonReleaseMethodArgDelete))
    {
    (*this->LeftButtonReleaseMethodArgDelete)
      (this->LeftButtonReleaseMethodArg);
    }
  if ((this->MiddleButtonPressMethodArg)&&
      (this->MiddleButtonPressMethodArgDelete))
    {
    (*this->MiddleButtonPressMethodArgDelete)
      (this->MiddleButtonPressMethodArg);
    }
  if ((this->MiddleButtonReleaseMethodArg)&&
      (this->MiddleButtonReleaseMethodArgDelete))
    {
    (*this->MiddleButtonReleaseMethodArgDelete)
      (this->MiddleButtonReleaseMethodArg);
    }
  if ((this->RightButtonPressMethodArg)&&
      (this->RightButtonPressMethodArgDelete))
    {
    (*this->RightButtonPressMethodArgDelete)(this->RightButtonPressMethodArg);
    }
  if ((this->RightButtonReleaseMethodArg)&&
      (this->RightButtonReleaseMethodArgDelete))
    {
    (*this->RightButtonReleaseMethodArgDelete)
      (this->RightButtonReleaseMethodArg);
    }
}

// Set the left button pressed method. This method is invoked on a left mouse button press.
void vtkInteractorStyle::SetLeftButtonPressMethod(void (*f)(void *), void *arg)
{
  if ( f != this->LeftButtonPressMethod || 
       arg != this->LeftButtonPressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->LeftButtonPressMethodArg)&&
        (this->LeftButtonPressMethodArgDelete))
      {
      (*this->LeftButtonPressMethodArgDelete)(this->LeftButtonPressMethodArg);
      }
    this->LeftButtonPressMethod = f;
    this->LeftButtonPressMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetLeftButtonPressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->LeftButtonPressMethodArgDelete)
    {
    this->LeftButtonPressMethodArgDelete = f;
    this->Modified();
    }
}


void vtkInteractorStyle::SetLeftButtonReleaseMethod(void (*f)(void *), 
                                                    void *arg)
{
  if ( f != this->LeftButtonReleaseMethod || arg != 
       this->LeftButtonReleaseMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->LeftButtonReleaseMethodArg)&&
        (this->LeftButtonReleaseMethodArgDelete))
      {
      (*this->LeftButtonReleaseMethodArgDelete)(this->LeftButtonReleaseMethodArg);
      }
    this->LeftButtonReleaseMethod = f;
    this->LeftButtonReleaseMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetLeftButtonReleaseMethodArgDelete(void (*f)(void *))
{
  if ( f != this->LeftButtonReleaseMethodArgDelete)
    {
    this->LeftButtonReleaseMethodArgDelete = f;
    this->Modified();
    }
}


void vtkInteractorStyle::SetMiddleButtonPressMethod(void (*f)(void *), 
                                                    void *arg)
{
  if ( f != this->MiddleButtonPressMethod || 
       arg != this->MiddleButtonPressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->MiddleButtonPressMethodArg)&&
        (this->MiddleButtonPressMethodArgDelete))
      {
      (*this->MiddleButtonPressMethodArgDelete)(this->MiddleButtonPressMethodArg);
      }
    this->MiddleButtonPressMethod = f;
    this->MiddleButtonPressMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetMiddleButtonPressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->MiddleButtonPressMethodArgDelete)
    {
    this->MiddleButtonPressMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked on a <e> keyrelease.
void vtkInteractorStyle::SetMiddleButtonReleaseMethod(void (*f)(void *), 
                                                      void *arg)
{
  if ( f != this->MiddleButtonReleaseMethod || 
       arg != this->MiddleButtonReleaseMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->MiddleButtonReleaseMethodArg)&&
        (this->MiddleButtonReleaseMethodArgDelete))
      {
      (*this->MiddleButtonReleaseMethodArgDelete)(this->MiddleButtonReleaseMethodArg);
      }
    this->MiddleButtonReleaseMethod = f;
    this->MiddleButtonReleaseMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetMiddleButtonReleaseMethodArgDelete(void (*f)(void *))
{
  if ( f != this->MiddleButtonReleaseMethodArgDelete)
    {
    this->MiddleButtonReleaseMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked on a <e> keypress.
void vtkInteractorStyle::SetRightButtonPressMethod(void (*f)(void *), 
                                                   void *arg)
{
  if ( f != this->RightButtonPressMethod || 
       arg != this->RightButtonPressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->RightButtonPressMethodArg)&&
        (this->RightButtonPressMethodArgDelete))
      {
      (*this->RightButtonPressMethodArgDelete)(this->RightButtonPressMethodArg);
      }
    this->RightButtonPressMethod = f;
    this->RightButtonPressMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetRightButtonPressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->RightButtonPressMethodArgDelete)
    {
    this->RightButtonPressMethodArgDelete = f;
    this->Modified();
    }
}

// Set the exit method. This method is invoked on a <e> keyrelease.
void vtkInteractorStyle::SetRightButtonReleaseMethod(void (*f)(void *), 
                                                     void *arg)
{
  if ( f != this->RightButtonReleaseMethod || 
       arg != this->RightButtonReleaseMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->RightButtonReleaseMethodArg)&&
        (this->RightButtonReleaseMethodArgDelete))
      {
      (*this->RightButtonReleaseMethodArgDelete)(this->RightButtonReleaseMethodArg);
      }
    this->RightButtonReleaseMethod = f;
    this->RightButtonReleaseMethodArg = arg;
    this->Modified();
    }
}

// Called when a void* argument is being discarded.  Lets the user free it.
void vtkInteractorStyle::SetRightButtonReleaseMethodArgDelete(void (*f)(void *))
{
  if ( f != this->RightButtonReleaseMethodArgDelete)
    {
    this->RightButtonReleaseMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyle::FindPokedRenderer(int x,int y) 
{
  vtkRendererCollection *rc;
  vtkRenderer *aren;
  int numRens, i;
  this->CurrentRenderer = NULL;
  
  rc = this->Interactor->GetRenderWindow()->GetRenderers();
  numRens = rc->GetNumberOfItems();
  for (i = numRens -1; (i >= 0) && !this->CurrentRenderer; i--) 
    {
    aren = (vtkRenderer *)rc->GetItemAsObject(i);
    if (aren->IsInViewport(x,y)) 
      {
      this->CurrentRenderer = aren;
      }
    }
  // we must have a value
  if (this->CurrentRenderer == NULL) 
    {
    rc->InitTraversal();
    aren = rc->GetNextItem();
    this->CurrentRenderer = aren;
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

//----------------------------------------------------------------------------
// When pick action successfully selects actor, this method highlights the
// actor appropriately. Currently this is done by placing a bounding box
// around the actor.
void vtkInteractorStyle::HighlightActor(vtkActor *actor) 
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
    }
  if ( this->PickedRenderer ) 
    {
    this->PickedRenderer->RemoveActor(this->OutlineActor);
    }
  
  if ( ! actor ) 
    {
    this->PickedRenderer = NULL;
    }
  else 
    {
    this->PickedRenderer = this->CurrentRenderer;
    this->CurrentRenderer->AddActor(this->OutlineActor);
    this->Outline->SetBounds(actor->GetBounds());
    this->CurrentActor = actor;
    }
  this->Interactor->Render();
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
  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());
  if ( !rwi->CreateTimer(VTKI_TIMER_FIRST) ) 
    {
    vtkErrorMacro(<< "Timer start failed");
    this->State = VTKIS_START;
    }
}
//----------------------------------------------------------------------------
void  vtkInteractorStyle::StopState() 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  this->State = VTKIS_START;
  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
  if ( !rwi->DestroyTimer() ) 
    {
    vtkErrorMacro(<< "Timer stop failed");
    }
  rwi->Render();
}

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
void vtkInteractorStyle::OnChar(int ctrl, int shift, 
                                char keycode, int repeatcount) 
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
  
  vtkRenderWindowInteractor *rwi = this->Interactor;
  switch (keycode) 
    {
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
    this->FindPokedRenderer(this->LastPos[0],this->LastPos[1]);
    ac = this->CurrentRenderer->GetActors();
    for (ac->InitTraversal(); anActor = ac->GetNextItem(); ) 
      {
      for (anActor->InitPartTraversal(); aPart=anActor->GetNextPart(); ) 
        {
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
    this->FindPokedRenderer(this->LastPos[0],this->LastPos[1]);
    ac = this->CurrentRenderer->GetActors();
    for (ac->InitTraversal(); anActor = ac->GetNextItem(); ) 
      {
      for (anActor->InitPartTraversal(); aPart=anActor->GetNextPart(); ) 
        {
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
        this->FindPokedRenderer(this->LastPos[0],this->LastPos[1]);
        rwi->StartPickCallback();
        rwi->GetPicker()->Pick(this->LastPos[0],this->LastPos[1], 0.0, 
                               this->CurrentRenderer);
        this->ActorPicked = 0;
        this->HighlightActor(rwi->GetPicker()->GetAssembly());
        rwi->EndPickCallback();
        }
      break;
    }
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
      //?? rwi->Render();
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
void vtkInteractorStyle::OnMouseMove(int ctrl, int shift, int X, int Y) 
{
  this->LastPos[0] = X;
  this->LastPos[1] = Y;
}

//----------------------------------------------------------------------------
void vtkInteractorStyle::OnLeftButtonDown(int ctrl, int shift, 
                                          int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
  this->FindPokedCamera(X, Y);
  if (this->LeftButtonPressMethod) 
    {
    (*this->LeftButtonPressMethod)(this->LeftButtonPressMethodArg);
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
  if (this->LeftButtonReleaseMethod) 
    {
    (*this->LeftButtonReleaseMethod)(this->LeftButtonReleaseMethodArg);
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
  if (this->MiddleButtonPressMethod) 
    {
    (*this->MiddleButtonPressMethod)(this->MiddleButtonPressMethodArg);
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
  if (this->MiddleButtonReleaseMethod) 
    {
    (*this->MiddleButtonReleaseMethod)(this->MiddleButtonReleaseMethodArg);
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
  if (this->RightButtonPressMethod) 
    {
    (*this->RightButtonPressMethod)(this->RightButtonPressMethodArg);
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
  if (this->RightButtonReleaseMethod) 
    {
    (*this->RightButtonReleaseMethod)(this->RightButtonReleaseMethodArg);
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
void vtkInteractorStyle::RotateCamera(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  double rxf = (double)(x - this->Center[0]) * this->DeltaAzimuth;
  double ryf = (double)(y - this->Center[1]) * this->DeltaElevation;

  this->CurrentCamera->Azimuth(rxf);
  this->CurrentCamera->Elevation(ryf);
  this->CurrentCamera->OrthogonalizeViewUp();
  if (rwi->GetLightFollowCamera())
    {
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }
  rwi->Render();
}

void vtkInteractorStyle::SpinCamera(int x, int y)
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
  double FocalDepth = ViewFocus[2];

  double NewPickPoint[4];
  this->ComputeDisplayToWorld((float)x, (float)y,
                              FocalDepth, NewPickPoint);

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

void vtkInteractorStyle::DollyCamera(int x, int y)
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
  if ( this->CurrentActor )
    {
    os << indent << "Current Actor: " << this->CurrentActor << "\n";
    }
  else
    {
    os << indent << "Current Actor: (none)\n";
    }

  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Actor Picked: " <<
    (this->ActorPicked ? "Yes\n" : "No\n");
}


