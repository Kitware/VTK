/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32RenderWindowInteractor.cxx
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
authors and that existing copyright notices are retained in all copies. Some
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkWin32RenderWindowInteractor.h"
#include "vtkActor.h"
#include <math.h>

//status
#define VTKXI_ANIMATE 7


void (*vtkWin32RenderWindowInteractor::ClassExitMethod)(void *)
                                                  = (void (*)(void *))NULL;
void *vtkWin32RenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)(void *)
                                                  = (void (*)(void *))NULL;


// Construct object so that light follows camera motion.
vtkWin32RenderWindowInteractor::vtkWin32RenderWindowInteractor()
{
  static timerId = 1;
  
  this->State = VTKXI_START;  
  this->AnimationState = VTKXI_START;
  this->WindowId = 0;
  this->TimerId = timerId++;
}


vtkWin32RenderWindowInteractor::~vtkWin32RenderWindowInteractor()
{
  vtkWin32OpenGLRenderWindow *tmp;

  // we need to release any hold we have on a windows event loop
  if (this->WindowId)
    {
    vtkWin32OpenGLRenderWindow *ren;
    ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
    tmp = (vtkWin32OpenGLRenderWindow *)GetWindowLong(this->WindowId,GWL_USERDATA);
    // watch for odd conditions
    if ((tmp != ren) && (ren != NULL))
      {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      this->OldProc(this->WindowId,WM_USER+14,28,(LONG)this->OldProc);
      }
    else
      {
      SetWindowLong(this->WindowId,GWL_WNDPROC,(LONG)this->OldProc);
      }

    this->Enabled = 0;
    }
}

void  vtkWin32RenderWindowInteractor::Start()
{
  MSG msg;
  
  while (GetMessage(&msg, NULL, 0, 0))
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }
}

// Begin processing keyboard strokes.
void vtkWin32RenderWindowInteractor::Initialize()
{
  static int any_initialized = 0;
  vtkWin32OpenGLRenderWindow *ren;
  int depth;
  int *size;
  int *position;
  int argc = 0;

  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
    {
    vtkErrorMacro(<<"No renderer defined!");
    return;
    }

  if (this->Initialized) return;
  this->Initialized = 1;

  // get the info we need from the RenderingWindow
  ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
  ren->Render();
  size    = ren->GetSize();
  position= ren->GetPosition();
  this->WindowId = ren->GetWindowId();

  this->Enable();

  this->Size[0] = size[0];
  this->Size[1] = size[1];
}


void vtkWin32RenderWindowInteractor::Enable()
{
  vtkWin32OpenGLRenderWindow *ren;
  vtkWin32OpenGLRenderWindow *tmp;

  if (this->Enabled)
    {
    return;
    }
  
  // add our callback
  ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
  this->OldProc = (WNDPROC)GetWindowLong(this->WindowId,GWL_WNDPROC);
  tmp=(vtkWin32OpenGLRenderWindow *)GetWindowLong(this->WindowId,GWL_USERDATA);
  // watch for odd conditions
  if (tmp != ren)
    {
    // OK someone else has a hold on our event handler
    // so lets have them handle this stuff
    // well send a USER message to the other
    // event handler so that it can properly
    // call this event handler if required
    this->OldProc(this->WindowId,WM_USER+12,24,(LONG)vtkHandleMessage);
    }
  else
    {
    SetWindowLong(this->WindowId,GWL_WNDPROC,(LONG)vtkHandleMessage);
    }

  // in case the size of the window has changed while we were away
  int *size;
  size = ren->GetSize();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
  
  this->Enabled = 1;
  this->Modified();
}


void vtkWin32RenderWindowInteractor::Disable()
{
  vtkWin32OpenGLRenderWindow *tmp;

  if (!this->Enabled)
    {
    return;
    }
  
  // we need to release any hold we have on a windows event loop
  if (this->WindowId)
    {
    vtkWin32OpenGLRenderWindow *ren;
    ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
    tmp = (vtkWin32OpenGLRenderWindow *)GetWindowLong(this->WindowId,GWL_USERDATA);
    // watch for odd conditions
    if ((tmp != ren) && (ren != NULL))
      {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      this->OldProc(this->WindowId,WM_USER+14,28,(LONG)this->OldProc);
      }
    else
      {
      SetWindowLong(this->WindowId,GWL_WNDPROC,(LONG)this->OldProc);
      }

    this->Enabled = 0;
    this->Modified();
    }
}


void vtkWin32RenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRenderWindowInteractor::PrintSelf(os,indent);
}


void  vtkWin32RenderWindowInteractor::UpdateSize(int x,int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0])||(y != this->Size[1]))
    {
    this->Size[0] = x;
    this->Size[1] = y;
    this->RenderWindow->SetSize(x,y);
    }
}
 
void  vtkWin32RenderWindowInteractor::StartRotate()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->State = VTKXI_ROTATE;
  if (this->AnimationState != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  if (!SetTimer(this->WindowId,this->TimerId,10,NULL))
    {
    vtkErrorMacro(<< "Not enough timers");
    }
}

void  vtkWin32RenderWindowInteractor::EndRotate()
{
  if (this->State != VTKXI_ROTATE)
    {
    return;
    }
  this->State = VTKXI_START;
  if (this->AnimationState == VTKXI_START)
    {
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    KillTimer(this->WindowId,this->TimerId);
    }
  this->RenderWindow->Render();
}

void  vtkWin32RenderWindowInteractor::StartZoom()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->State = VTKXI_ZOOM;
  if (this->AnimationState != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  if (!SetTimer(this->WindowId,this->TimerId,10,NULL))
    {
    vtkErrorMacro(<< "Not enough timers");
    }
}

void  vtkWin32RenderWindowInteractor::EndZoom()
{
  if (this->State != VTKXI_ZOOM)
    {
    return;
    }
  this->State = VTKXI_START;
  if (this->AnimationState == VTKXI_START)
    {
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    KillTimer(this->WindowId,this->TimerId);
    }
  this->RenderWindow->Render();
}

void  vtkWin32RenderWindowInteractor::StartPan()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->State = VTKXI_PAN;
  if (this->AnimationState != VTKXI_START)
    {
    return;
    }
  
  // calculation of focal depth has been moved to panning function.
       
  this->Preprocess = 1;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  if (!SetTimer(this->WindowId,this->TimerId,10,NULL))
    {
    vtkErrorMacro(<< "Not enough timers");
    }
}

void  vtkWin32RenderWindowInteractor::EndPan()
{
  if (this->State != VTKXI_PAN)
    {
    return;
    }
  this->State = VTKXI_START;
  if (this->AnimationState == VTKXI_START)
    {
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    KillTimer(this->WindowId,this->TimerId);
    }
  this->RenderWindow->Render();
}

void  vtkWin32RenderWindowInteractor::StartAnimation()
{
  if (this->AnimationState != VTKXI_START)
    {
    return;
    }
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  this->AnimationState = VTKXI_ANIMATE;
  if (this->State != VTKXI_START)
    {
    return;
    }
  if (!SetTimer(this->WindowId,this->TimerId,10,NULL))
    {
    vtkErrorMacro(<< "Not enough timers");
    }
}

void  vtkWin32RenderWindowInteractor::EndAnimation()
{
  this->AnimationState = VTKXI_START;
  if (this->State == VTKXI_START) 
    {
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    KillTimer(this->WindowId,this->TimerId);
    }
  this->RenderWindow->Render();
}

void  vtkWin32RenderWindowInteractor::StartSpin()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->State = VTKXI_SPIN;
  if (this->AnimationState != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  if (!SetTimer(this->WindowId,this->TimerId,10,NULL))
    {
    vtkErrorMacro(<< "Not enough timers");
    }
}

void  vtkWin32RenderWindowInteractor::EndSpin()
{
  if (this->State != VTKXI_SPIN)
    {
    return;
    }
  this->State = VTKXI_START;
  if (this->AnimationState == VTKXI_START)
    {
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    KillTimer(this->WindowId,this->TimerId);
    }
  this->RenderWindow->Render();
}

void  vtkWin32RenderWindowInteractor::StartDolly()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->State = VTKXI_DOLLY;
  if (this->AnimationState != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  if (!SetTimer(this->WindowId,this->TimerId,10,NULL))
    {
    vtkErrorMacro(<< "Not enough timers");
    }
}

void  vtkWin32RenderWindowInteractor::EndDolly()
{
  if (this->State != VTKXI_DOLLY)
    {
    return;
    }
  this->State = VTKXI_START;
  if (this->AnimationState == VTKXI_START)
    {
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    KillTimer(this->WindowId,this->TimerId);
    }
  this->RenderWindow->Render();
}


void  vtkWin32RenderWindowInteractor::StartUniformScale()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->State = VTKXI_USCALE;
  if (this->AnimationState != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  if (!SetTimer(this->WindowId,this->TimerId,10,NULL))
    {
    vtkErrorMacro(<< "Not enough timers");
    }
}

void  vtkWin32RenderWindowInteractor::EndUniformScale()
{
  if (this->State != VTKXI_USCALE)
    {
    return;
    }
  this->State = VTKXI_START;
  if (this->AnimationState == VTKXI_START)
    {
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    KillTimer(this->WindowId,this->TimerId);
    }
  this->RenderWindow->Render();
}


LRESULT CALLBACK vtkHandleMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  float xf,yf;
  vtkWin32OpenGLRenderWindow *ren;
  vtkWin32RenderWindowInteractor *me;

  ren = (vtkWin32OpenGLRenderWindow *)GetWindowLong(hWnd,GWL_USERDATA);
  me = (vtkWin32RenderWindowInteractor *)ren->GetInteractor();

  if (me == NULL)
    {
    return 0;
    }
  
  if ((uMsg == WM_USER+13)&&(wParam == 26))
    {
    // someone is telling us to set our OldProc
    me->OldProc = (WNDPROC)lParam;
    return 1;
    }

  switch (uMsg)
    {
    case WM_PAINT: 
      me->GetRenderWindow()->Render();
      return me->OldProc(hWnd,uMsg,wParam,lParam);
      //return DefWindowProc(hWnd,uMsg,wParam,lParam);
      break;
	    
    case WM_SIZE:
      me->UpdateSize(LOWORD(lParam),HIWORD(lParam)); 
      return me->OldProc(hWnd,uMsg,wParam,lParam);
      break;
	    
    case WM_LBUTTONDOWN: 
      me->SetEventPosition(LOWORD(lParam),
                           me->Size[1] - HIWORD(lParam) - 1);

      me->OldX = LOWORD(lParam);
      me->OldY = HIWORD(lParam);

      if (wParam & MK_CONTROL)
        {
	me->ControlMode = VTKXI_CONTROL_ON;
        }
      else
        {
	me->ControlMode = VTKXI_CONTROL_OFF;
        }

      me->FindPokedCamera(LOWORD(lParam),
                          me->Size[1] - HIWORD(lParam) - 1);
      
      if (me->ActorMode)
        {
        if (me->StartInteractionPickMethod)
          {
	  (*me->StartInteractionPickMethod)(me->StartInteractionPickMethodArg);
          }
        
	me->InteractionPicker->Pick(LOWORD(lParam), 
                                    me->Size[1]-HIWORD(lParam)-1,
                                    0.0, me->CurrentRenderer);

        // now go through the actor collection and decide which is closest
        vtkActor *closestActor = NULL, *actor;
        vtkActorCollection *actors = me->InteractionPicker->GetActors();
        vtkPoints *pickPositions = me->InteractionPicker->GetPickedPositions();
        int i = 0;
        float *pickPoint, d;
        float distToCamera = VTK_LARGE_FLOAT;
        if (actors && actors->GetNumberOfItems() > 0)
          {
          actors->InitTraversal();
          me->CurrentCamera->GetPosition(me->ViewPoint);
          while (i < pickPositions->GetNumberOfPoints())
            {
            actor = actors->GetNextItem();
            if (actor != NULL)
              {
              pickPoint = pickPositions->GetPoint(i);
              d = vtkMath::Distance2BetweenPoints(pickPoint, me->ViewPoint);
              if (distToCamera > d)
                {
                distToCamera = d;
                closestActor = actor;
                }
              }
            i++;
            }
          }

        me->InteractionActor = closestActor;
        // refine the answer to whether an actor was picked.  CellPicker()
        // returns true from Pick() if the bounding box was picked,
        // but we only want something to be picked if a cell was actually
        // selected
        me->ActorPicked = (me->InteractionActor != NULL);
        // highlight actor at the end of interaction

	if (me->EndInteractionPickMethod)
	  {
	  (*me->EndInteractionPickMethod)(me->EndInteractionPickMethodArg);
	  }

        }

      //      if ((wParam & MK_SHIFT) || (wParam & MK_RBUTTON))
      if (wParam & MK_SHIFT)
	{
	if (me->MiddleButtonPressMethod) 
	  {
	  (*me->MiddleButtonPressMethod)(me->MiddleButtonPressMethodArg);
	  }
	else
	  {
          if (me->ControlMode)
            {
            me->StartDolly();
            }
          else
            {me->StartPan();
            }
	  }
	}
      else
	{
	if (me->LeftButtonPressMethod) 
	  {
	  (*me->LeftButtonPressMethod)(me->LeftButtonPressMethodArg);
	  }
	else
	  {
          if (me->ControlMode)
            {
            me->StartSpin();
            }
          else
            {
            me->StartRotate();
            }
	  }
	}
      break;
	    
    case WM_LBUTTONUP: 
      me->SetEventPosition(LOWORD(lParam),
                           me->Size[1] - HIWORD(lParam) - 1);

      // don't change actor or trackball modes in the middle of motion
      // don't change control mode in the middle of mouse movement

      //      if ((wParam & MK_SHIFT) || (wParam & MK_RBUTTON))
      if (wParam & MK_SHIFT)
	{
	if (me->MiddleButtonReleaseMethod) 
	  {
	  (*me->MiddleButtonReleaseMethod)(me->MiddleButtonReleaseMethodArg);
	  }
	else
	  {
          if (me->ControlMode)
            {
            me->EndDolly();
            }
          else
            {
            me->EndPan();
            }
	  }
	}
      else
	{
	if (me->LeftButtonReleaseMethod) 
	  {
	  (*me->LeftButtonReleaseMethod)(me->LeftButtonReleaseMethodArg);
	  }
	else
	  {
          if (me->ControlMode)
            {
            me->EndSpin();
            }
          else
            {
            me->EndRotate();
            }
	  }
	}
      me->OldX = 0.0;
      me->OldY = 0.0;
      if (me->ActorMode && me->ActorPicked)
        {
        me->HighlightActor(me->InteractionActor);
        }
      else
        {
        me->HighlightActor(NULL);
        }
      break;
	    
    case WM_MBUTTONDOWN: 
      me->SetEventPosition(LOWORD(lParam),
                           me->Size[1] - HIWORD(lParam) - 1);

      me->OldX = LOWORD(lParam);
      me->OldY = HIWORD(lParam);
            
      if (wParam & MK_CONTROL)
        {
	me->ControlMode = VTKXI_CONTROL_ON;
        }
      else
        {
	me->ControlMode = VTKXI_CONTROL_OFF;
        }

      me->FindPokedCamera(LOWORD(lParam),
                          me->Size[1] - HIWORD(lParam) - 1);

      if (me->ActorMode)
        {
	if (me->StartInteractionPickMethod)
	  {
	  (*me->StartInteractionPickMethod)(me->StartInteractionPickMethodArg);
	  }

        (me->InteractionPicker)->Pick(LOWORD(lParam), 
				      me->Size[1]-HIWORD(lParam)-1,
				      0.0, me->CurrentRenderer);

        // now go through the actor collection and decide which is closest
        vtkActor *closestActor = NULL, *actor;
        vtkActorCollection *actors = me->InteractionPicker->GetActors();
        vtkPoints *pickPositions = me->InteractionPicker->GetPickedPositions();
        int i = 0;
        float *pickPoint, d;
        float distToCamera = VTK_LARGE_FLOAT;
        if (actors && actors->GetNumberOfItems() > 0)
          {
          actors->InitTraversal();
          me->CurrentCamera->GetPosition(me->ViewPoint);
          while (i < pickPositions->GetNumberOfPoints())
            {
            actor = actors->GetNextItem();
            if (actor != NULL)
              {
              pickPoint = pickPositions->GetPoint(i);
              d = vtkMath::Distance2BetweenPoints(pickPoint, me->ViewPoint);
              if (distToCamera > d)
                {
                distToCamera = d;
                closestActor = actor;
                }
              }
            i++;
            }
          }

        me->InteractionActor = closestActor;
        // refine the answer to whether an actor was picked.  CellPicker()
        // returns true from Pick() if the bounding box was picked,
        // but we only want something to be picked if a cell was actually
        // selected
        me->ActorPicked = (me->InteractionActor != NULL);
        // highlight actor at the end of interaction

	if (me->EndInteractionPickMethod)
	  {
	  (*me->EndInteractionPickMethod)(me->EndInteractionPickMethodArg);
	  }
        }

      if (me->MiddleButtonPressMethod) 
	{
	(*me->MiddleButtonPressMethod)(me->MiddleButtonPressMethodArg);
	}
      else
	{
        if (me->ControlMode)
          {
          me->StartDolly();
          }
        else
          {
          me->StartPan();
          }
	}
      break;
      
    case WM_MBUTTONUP: 
      me->SetEventPosition(LOWORD(lParam),
                           me->Size[1] - HIWORD(lParam) - 1);

      // don't change actor or trackball mode in the middle of motion
      // don't change control mode in the middle of mouse movement

      if (me->MiddleButtonReleaseMethod) 
	{
	(*me->MiddleButtonReleaseMethod)(me->MiddleButtonReleaseMethodArg);
	}
      else
	{
        if (me->ControlMode)
          {
          me->EndDolly();
          }
        else
          {
          me->EndPan();
          }
	}
      me->OldX = 0.0;
      me->OldY = 0.0;
      if (me->ActorMode && me->ActorPicked)
        {
        me->HighlightActor(me->InteractionActor);
        }
      else
        {
        me->HighlightActor(NULL);
        }
      break;
      
    case WM_RBUTTONDOWN: 
      me->SetEventPosition(LOWORD(lParam),
                           me->Size[1] - HIWORD(lParam) - 1);

      me->OldX = LOWORD(lParam);
      me->OldY = HIWORD(lParam);
            
      if (wParam & MK_CONTROL)
        {
	me->ControlMode = VTKXI_CONTROL_ON;
        }
      else
        {
	me->ControlMode = VTKXI_CONTROL_OFF;
        }


      me->FindPokedCamera(LOWORD(lParam),
                          me->Size[1] - HIWORD(lParam) - 1);
      
      if (me->ActorMode)
        { 
        if (me->StartInteractionPickMethod)
	  {
	  (*me->StartInteractionPickMethod)(me->StartInteractionPickMethodArg);
	  }

        (me->InteractionPicker)->Pick(LOWORD(lParam), 
				      me->Size[1]-HIWORD(lParam)-1,
				      0.0, me->CurrentRenderer);

        // now go through the actor collection and decide which is closest
        vtkActor *closestActor = NULL, *actor;
        vtkActorCollection *actors = me->InteractionPicker->GetActors();
        vtkPoints *pickPositions = me->InteractionPicker->GetPickedPositions();
        int i = 0;
        float *pickPoint, d;
        float distToCamera = VTK_LARGE_FLOAT;
        if (actors && actors->GetNumberOfItems() > 0)
          {
          actors->InitTraversal();
          me->CurrentCamera->GetPosition(me->ViewPoint);
          while (i < pickPositions->GetNumberOfPoints())
            {
            actor = actors->GetNextItem();
            if (actor != NULL)
              {
              pickPoint = pickPositions->GetPoint(i);
              d = vtkMath::Distance2BetweenPoints(pickPoint, me->ViewPoint);
              if (distToCamera > d)
                {
                distToCamera = d;
                closestActor = actor;
                }
              }
            i++;
            }
          }

        me->InteractionActor = closestActor;
        // refine the answer to whether an actor was picked.  CellPicker()
        // returns true from Pick() if the bounding box was picked,
        // but we only want something to be picked if a cell was actually
        // selected
        me->ActorPicked = (me->InteractionActor != NULL);
        // highlight actor at the end of interaction

	if (me->EndInteractionPickMethod)
	  {
	  (*me->EndInteractionPickMethod)(me->EndInteractionPickMethodArg);
	  }
        }

      if (me->RightButtonPressMethod) 
	{
	(*me->RightButtonPressMethod)(me->RightButtonPressMethodArg);
	}
      else
	{
        if (me->ActorMode)
          {
          me->StartUniformScale();
          }
        else
          {
          me->StartZoom();
          }
	}
      break;
	    
    case WM_RBUTTONUP: 
      me->SetEventPosition(LOWORD(lParam),
                           me->Size[1] - HIWORD(lParam) - 1);

      // don't change actor or trackball modes in the middle of motion
      // don't change control mode in the middle of mouse movement

      if (me->RightButtonReleaseMethod) 
	{
	(*me->RightButtonReleaseMethod)(me->RightButtonReleaseMethodArg);
	}
      else
	{
        if (me->ActorMode)
          {
          me->EndUniformScale();
          }
        else
          {
          me->EndZoom();
          }
	}
      me->OldX = 0.0;
      me->OldY = 0.0;
      if (me->ActorMode && me->ActorPicked)
        {
        me->HighlightActor(me->InteractionActor);
        }
      else
        {
        me->HighlightActor(NULL);
        }
      break;
	    
    case WM_MOUSEMOVE:
      me->LastPosition = lParam;
      break;

    case WM_CLOSE:
      if (me->ExitMethod)
	{
        (*me->ExitMethod)(me->ExitMethodArg);
        }
      else if (me->ClassExitMethod)
	{
        (*me->ClassExitMethod)(me->ClassExitMethodArg);
        }
      else
        {
        PostQuitMessage(0);
        }
      break;
	    
    case WM_CHAR:
      switch (wParam)
	{
	case 'a':
	case 'A':
	  if (me->AnimationState != VTKXI_ANIMATE) 
            {
            me->StartAnimation();
            }
	  else
            {
            me->EndAnimation();
            }
	  break;

        case 'Q':
        case 'q':
	case 'e':
	case 'E': 
	  if (me->ExitMethod)
	    {
            (*me->ExitMethod)(me->ExitMethodArg);
            }
	  else if (me->ClassExitMethod)
	    {
            (*me->ClassExitMethod)(me->ClassExitMethodArg);
            }
	  else
            {
            PostQuitMessage(0);
            }
	  break;
          
	case 'u':
	case 'U':
	  if (me->UserMethod)
            {
            (*me->UserMethod)(me->UserMethodArg);
            }
	  break;
          
	case 'r':
	case 'R':
	  if (me->ActorMode)
            {
            //vtkDebugMacro(<<"Please switch to Camera mode when resetting");
            }
	  else
            {
            me->FindPokedRenderer(LOWORD(me->LastPosition),
				  me->Size[1]-HIWORD(me->LastPosition)-1);
	    me->CurrentRenderer->ResetCamera();
	    me->RenderWindow->Render();
            }
	  break;
          
	case 'w':
	case 'W':
	  {
          vtkActorCollection *ac;
	  vtkActor *anActor, *aPart;
	  
	  me->FindPokedRenderer(LOWORD(me->LastPosition),
                                me->Size[1]-HIWORD(me->LastPosition)-1);
	  ac = me->CurrentRenderer->GetActors();
	  for (ac->InitTraversal(); anActor = ac->GetNextItem(); )
	    {
	    for (anActor->InitPartTraversal(); aPart=anActor->GetNextPart(); )
	      {
	      aPart->GetProperty()->SetRepresentationToWireframe();
	      }
	    }
	  
	  me->RenderWindow->Render();
	  }
          break;
          
	case 's':
	case 'S':
	  {
          vtkActorCollection *ac;
	  vtkActor *anActor, *aPart;
	  
	  me->FindPokedRenderer(LOWORD(me->LastPosition),
                                me->Size[1]-HIWORD(me->LastPosition)-1);
	  ac = me->CurrentRenderer->GetActors();
	  for (ac->InitTraversal(); anActor = ac->GetNextItem(); )
	    {
	    for (anActor->InitPartTraversal(); aPart=anActor->GetNextPart(); )
	      {
	      aPart->GetProperty()->SetRepresentationToSurface();
	      }
	    }
	  
	  me->RenderWindow->Render();
	  }
          break;
          
	case '3':
	  if (me->RenderWindow->GetStereoRender())
	    {
	    me->RenderWindow->StereoRenderOff();
	    }
	  else
	    {
	    me->RenderWindow->StereoRenderOn();
	    }
	  me->RenderWindow->Render();
	  break;
          
	case 'p':
	case 'P':
          if (me->State == VTKXI_START)
            {
            me->FindPokedRenderer(LOWORD(me->LastPosition),
                                  me->Size[1]-HIWORD(me->LastPosition)-1);
            if (me->StartPickMethod)
              {
              (*me->StartPickMethod)(me->StartPickMethodArg);
              }
            me->Picker->Pick(LOWORD(me->LastPosition), 
                             me->Size[1]-HIWORD(me->LastPosition)-1,
                             0.0, me->CurrentRenderer);
            
            me->InteractionActor = NULL;
            me->ActorPicked = 0;
            me->HighlightActor(me->Picker->GetAssembly());

            if (me->EndPickMethod)
              {
              (*me->EndPickMethod)(me->EndPickMethodArg);
              }
            }
          break;
          
	case 'j':
	case 'J':
	  if (me->State == VTKXI_START) 
	    {
            me->TrackballMode = VTKXI_JOY;
	    //vtkDebugMacro(<<"Swtich to Joystick style interaction.");
	    if (me->JoystickModeMethod) 
	      {
	      (*me->JoystickModeMethod)(me->JoystickModeMethodArg);
	      }
	    }
	  break;
          
	case 't':
	case 'T':
	  if (me->State == VTKXI_START) 
	    {
            me->TrackballMode = VTKXI_TRACK;
            //vtkDebugMacro(<<"Swtich to Trackball style interaction.");
	    if (me->TrackballModeMethod) 
	      {
	      (*me->TrackballModeMethod)(me->TrackballModeMethodArg);
	      }
	    }
	  break;
          
        case 'o':
        case 'O':
          if (me->State == VTKXI_START) 
	    {
            if (me->ActorMode != VTKXI_ACTOR)
              {
              // reset the actor picking variables
              me->InteractionActor = NULL;
              me->ActorPicked = 0;
              me->HighlightActor(NULL);

              me->ActorMode = VTKXI_ACTOR;
              //vtkDebugMacro(<<"switch to Actor mode.");
              if (me->ActorModeMethod) 
                {
                (*me->ActorModeMethod)(me->ActorModeMethodArg);
                }
              }
            }
          break;
          
        case 'c':
        case 'C':
          if (me->State == VTKXI_START) 
	    {
            if (me->ActorMode != VTKXI_CAMERA)
              {
              // reset the actor picking variables
              me->InteractionActor = NULL;
              me->ActorPicked = 0;
              me->HighlightActor(NULL);

              me->ActorMode = VTKXI_CAMERA;
              //vtkDebugMacro(<<"switch to Camera mode.");
              if (me->CameraModeMethod) 
                {
                (*me->CameraModeMethod)(me->CameraModeMethodArg);
                }
              }
            }
          break;
          
	}
      break;
      
    case WM_TIMER:
      if (me->TimerMethod)
        {
	me->SetEventPosition(LOWORD(me->LastPosition),
                             me->Size[1] - HIWORD(me->LastPosition)-1);
	(*me->TimerMethod)(me->TimerMethodArg);
        }
      
      switch (me->State)
        {
        case VTKXI_START:
          //?? me->RenderWindow->Render();
          break;
          
        case VTKXI_ROTATE:  // rotate with respect to an axis perp to look
          if (me->ActorMode && me->ActorPicked)
            {
            if (me->TrackballMode)
              {
              me->TrackballRotateActor(LOWORD(me->LastPosition),
                                       HIWORD(me->LastPosition));
              }
  	    else
              {
              me->JoystickRotateActor(LOWORD(me->LastPosition),
                                      HIWORD(me->LastPosition));
              }
            }
          else if (!(me->ActorMode))
            {
            if (me->TrackballMode)
              {
              me->TrackballRotateCamera(LOWORD(me->LastPosition),
                                        HIWORD(me->LastPosition));
              }
            else
              {
              me->JoystickRotateCamera(LOWORD(me->LastPosition),
                                       HIWORD(me->LastPosition));
              }
            }
          break;
          
        case VTKXI_PAN: // move perpendicular to camera's look vector
          if (me->ActorMode && me->ActorPicked)
            {
            if (me->TrackballMode)
              { 
              me->TrackballPanActor(LOWORD(me->LastPosition),
                                    HIWORD(me->LastPosition));
              }
            else
              {
              me->JoystickPanActor(LOWORD(me->LastPosition),
                                   HIWORD(me->LastPosition));
              }
            }
          else if (!(me->ActorMode))
            {
            if (me->TrackballMode)
              {
              me->TrackballPanCamera(LOWORD(me->LastPosition),
                                     HIWORD(me->LastPosition));
              }
            else
              {
              me->JoystickPanCamera(LOWORD(me->LastPosition),
                                    HIWORD(me->LastPosition));
              }
            }
          break;
          
        case VTKXI_ZOOM:
          if (!(me->ActorMode))
            {
            if (me->TrackballMode)
              { 
              me->TrackballDollyCamera(LOWORD(me->LastPosition),
                                       HIWORD(me->LastPosition));
              }
            else
              {
              me->JoystickDollyCamera(LOWORD(me->LastPosition),
                                      HIWORD(me->LastPosition));
              }
            }
          break;
          
        case VTKXI_SPIN:
          if (me->ActorMode && me->ActorPicked)
            {
            if (me->TrackballMode)
              { 
              me->TrackballSpinActor(LOWORD(me->LastPosition),
                                     HIWORD(me->LastPosition));
              }
            else
              {
              me->JoystickSpinActor(LOWORD(me->LastPosition),
                                    HIWORD(me->LastPosition));
              }
            }
          else if (!(me->ActorMode))
            {
            if (me->TrackballMode)
              {
              me->TrackballSpinCamera(LOWORD(me->LastPosition),
                                      HIWORD(me->LastPosition));
              }
            else
              {
              me->JoystickSpinCamera(LOWORD(me->LastPosition),
                                     HIWORD(me->LastPosition));
              }
            }
          break;
          
        case VTKXI_DOLLY:  // move along camera's view vector
          if (me->ActorMode && me->ActorPicked)
            {
            if (me->TrackballMode)
              { 
              me->TrackballDollyActor(LOWORD(me->LastPosition),
                                      HIWORD(me->LastPosition));
              }
            else
              {
              me->JoystickDollyActor(LOWORD(me->LastPosition),
                                     HIWORD(me->LastPosition));
              }
            }
          break;

        case VTKXI_USCALE:
          if (me->ActorMode && me->ActorPicked)
            {
            if (me->TrackballMode)
              {
              me->TrackballScaleActor(LOWORD(me->LastPosition),
                                      HIWORD(me->LastPosition));
              }
            else
              {
              me->JoystickScaleActor(LOWORD(me->LastPosition),
                                     HIWORD(me->LastPosition));
              }
            }
          break;
          
        }
      break;
    default:
      if (me)
	{
	return me->OldProc(hWnd,uMsg,wParam,lParam);
	}
    };
  return 0;
}



// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void
vtkWin32RenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkWin32RenderWindowInteractor::ClassExitMethod
       || arg != vtkWin32RenderWindowInteractor::ClassExitMethodArg)
    {
    // delete the current arg if there is a delete method
    if ((vtkWin32RenderWindowInteractor::ClassExitMethodArg)
	&& (vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete))
      {
      (*vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)
	(vtkWin32RenderWindowInteractor::ClassExitMethodArg);
      }
    vtkWin32RenderWindowInteractor::ClassExitMethod = f;
    vtkWin32RenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
    }
}


// Set the arg delete method.  This is used to free user memory.
void
vtkWin32RenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)
    {
    vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
    }
}

