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

// states
#define VTKXI_START   0
#define VTKXI_ROTATE  1
#define VTKXI_ZOOM    2
#define VTKXI_PAN     3
#define VTKXI_ANIMATE 4

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
    if (tmp != ren)
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
  vtkWin32OpenGLRenderWindow *tmp;

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
  this->OldProc = (WNDPROC)GetWindowLong(this->WindowId,GWL_WNDPROC);
  tmp = (vtkWin32OpenGLRenderWindow *)GetWindowLong(this->WindowId,GWL_USERDATA);
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
  /* add callback */

  this->Size[0] = size[0];
  this->Size[1] = size[1];
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
  if (this->State != VTKXI_START) return;
  this->State = VTKXI_ROTATE;
  if (this->AnimationState != VTKXI_START) return;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  if (!SetTimer(this->WindowId,this->TimerId,10,NULL))
    {
    vtkErrorMacro(<< "Not enough timers");
    }
}
void  vtkWin32RenderWindowInteractor::EndRotate()
{
  if (this->State != VTKXI_ROTATE) return;
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
  if (this->State != VTKXI_START) return;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  this->State = VTKXI_ZOOM;
  if (this->AnimationState != VTKXI_START) return;
  SetTimer(this->WindowId,this->TimerId,10,NULL);
}
void  vtkWin32RenderWindowInteractor::EndZoom()
{
  if (this->State != VTKXI_ZOOM) return;
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
  float *FocalPoint;
  float *Result;

  if (this->State != VTKXI_START) return;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  this->State = VTKXI_PAN;

  // calculate the focal depth since we'll be using it a lot
  FocalPoint = this->CurrentCamera->GetFocalPoint();
      
  this->CurrentRenderer->SetWorldPoint(FocalPoint[0],FocalPoint[1],
				       FocalPoint[2],1.0);
  this->CurrentRenderer->WorldToDisplay();
  Result = this->CurrentRenderer->GetDisplayPoint();
  this->FocalDepth = Result[2];

  if (this->AnimationState != VTKXI_START) return;
  SetTimer(this->WindowId,this->TimerId,10,NULL);
}
void  vtkWin32RenderWindowInteractor::EndPan()
{
  if (this->State != VTKXI_PAN) return;
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
  if (this->AnimationState != VTKXI_START) return;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  this->AnimationState = VTKXI_ANIMATE;
  if (this->State != VTKXI_START) return;

  SetTimer(this->WindowId,this->TimerId,10,NULL);
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

LRESULT CALLBACK vtkHandleMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  float xf,yf;
  vtkWin32OpenGLRenderWindow *ren;
  vtkWin32RenderWindowInteractor *me;

  ren = (vtkWin32OpenGLRenderWindow *)GetWindowLong(hWnd,GWL_USERDATA);
  me = (vtkWin32RenderWindowInteractor *)ren->GetInteractor();

  if ((uMsg == WM_USER+13)&&(wParam == 26))
    {
    // someone is telling us to set our OldProc
    me->OldProc = (WNDPROC)lParam;
    return 1;
    }

  switch (uMsg)
    {
    case WM_PAINT: 
      //me->GetRenderWindow()->Render();
      return me->OldProc(hWnd,uMsg,wParam,lParam);
      //return DefWindowProc(hWnd,uMsg,wParam,lParam);
      break;
	    
    case WM_SIZE: 
      me->UpdateSize(LOWORD(lParam),HIWORD(lParam)); 
      return me->OldProc(hWnd,uMsg,wParam,lParam);
      break;
	    
    case WM_LBUTTONDOWN: 
      me->SetEventPosition(LOWORD(lParam),me->Size[1] - HIWORD(lParam) - 1);
      if (wParam & MK_SHIFT)
	{
	if (me->MiddleButtonPressMethod) 
	  {
	  (*me->MiddleButtonPressMethod)(me->MiddleButtonPressMethodArg);
	  }
	else
	  {
	  me->FindPokedCamera(LOWORD(lParam),me->Size[1] - HIWORD(lParam) - 1);
	  me->StartPan();
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
	  me->FindPokedCamera(LOWORD(lParam),me->Size[1] - HIWORD(lParam) - 1);
	  me->StartRotate(); 
	  }
	}
      break;
	    
    case WM_LBUTTONUP: 
      me->SetEventPosition(LOWORD(lParam),me->Size[1] - HIWORD(lParam) - 1);
      if (wParam & MK_SHIFT)
	{
	if (me->MiddleButtonReleaseMethod) 
	  {
	  (*me->MiddleButtonReleaseMethod)(me->MiddleButtonReleaseMethodArg);
	  }
	else
	  {
	  me->EndPan(); 
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
	  me->EndRotate(); 
	  }
	}
      break;
	    
    case WM_RBUTTONDOWN: 
      me->SetEventPosition(LOWORD(lParam),me->Size[1] - HIWORD(lParam) - 1);
      if (me->RightButtonPressMethod) 
	{
	(*me->RightButtonPressMethod)(me->RightButtonPressMethodArg);
	}
      else
	{
	me->FindPokedCamera(LOWORD(lParam),me->Size[1] - HIWORD(lParam) - 1);
	me->StartZoom(); 
	}
      break;
	    
    case WM_RBUTTONUP: 
      me->SetEventPosition(LOWORD(lParam),me->Size[1] - HIWORD(lParam) - 1);
      if (me->RightButtonReleaseMethod) 
	{
	(*me->RightButtonReleaseMethod)(me->RightButtonReleaseMethodArg);
	}
      else
	{
	me->EndZoom(); 
	}
      break;
	    
    case WM_MOUSEMOVE: me->LastPosition = lParam; break;

    case WM_CLOSE:
      if (me->ExitMethod)
	(*me->ExitMethod)(me->ExitMethodArg);
      else if (me->ClassExitMethod)
	(*me->ClassExitMethod)(me->ClassExitMethodArg);
      else PostQuitMessage(0); 
      break;
	    
    case WM_CHAR:
      switch (wParam)
	{
	case 'a':
	  if (me->AnimationState != VTKXI_ANIMATE) 
		{
		me->StartAnimation();
		}
	  else
		{
		me->EndAnimation();
		}
	  break;	
	case 'e': 
	  if (me->ExitMethod)
	    (*me->ExitMethod)(me->ExitMethodArg);
	  else if (me->ClassExitMethod)
	    (*me->ClassExitMethod)(me->ClassExitMethodArg);
	  else PostQuitMessage(0); 
	  break;
	case 'u':
	  if (me->UserMethod) (*me->UserMethod)(me->UserMethodArg);
	  break;
	case 'r':
	  me->FindPokedRenderer(LOWORD(me->LastPosition),
		  me->Size[1]-HIWORD(me->LastPosition)-1);
	  me->CurrentRenderer->ResetCamera();
	  me->RenderWindow->Render();
	  break;
	case 'w':
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
	  {
	  if (me->RenderWindow->GetStereoRender())
	    {
	    me->RenderWindow->StereoRenderOff();
	    }
	  else
	    {
	    me->RenderWindow->StereoRenderOn();
	    }
	  me->RenderWindow->Render();
	  }
	  break;
	case 'p':
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
	  me->HighlightActor(me->Picker->GetAssembly());
	  if (me->EndPickMethod)
	    {
	    (*me->EndPickMethod)(me->EndPickMethodArg);
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
	case VTKXI_START :
	  me->RenderWindow->Render();
	  break;
	case VTKXI_ROTATE :
	  xf = (LOWORD(me->LastPosition) - me->Center[0]) * me->DeltaAzimuth;
	  yf = ((me->Size[1] - HIWORD(me->LastPosition)) - 
			 me->Center[1]) * me->DeltaElevation;
	  me->CurrentCamera->Azimuth(xf);
	  me->CurrentCamera->Elevation(yf);
	  me->CurrentCamera->OrthogonalizeViewUp();
	  if (me->LightFollowCamera)
	    {
	    /* get the first light */
	    me->CurrentLight->SetPosition(me->CurrentCamera->GetPosition());
	    me->CurrentLight->SetFocalPoint(me->CurrentCamera->GetFocalPoint());
	    }
	  me->RenderWindow->Render();
	  break;
	case VTKXI_PAN :
	  {
	  float  FPoint[3];
	  float *PPoint;
	  float  APoint[3];
	  float  RPoint[4];

	  // get the current focal point and position
	  memcpy(FPoint,me->CurrentCamera->GetFocalPoint(),sizeof(float)*3);
	  PPoint = me->CurrentCamera->GetPosition();

	  xf = LOWORD(me->LastPosition);
	  yf = me->Size[1] - HIWORD(me->LastPosition);
	  APoint[0] = xf;
	  APoint[1] = yf;
	  APoint[2] = me->FocalDepth;
	  me->CurrentRenderer->SetDisplayPoint(APoint);
	  me->CurrentRenderer->DisplayToWorld();
	  memcpy(RPoint,me->CurrentRenderer->GetWorldPoint(),sizeof(float)*4);
	  if (RPoint[3])
	    {
	    RPoint[0] /= RPoint[3];
	    RPoint[1] /= RPoint[3];
	    RPoint[2] /= RPoint[3];
	    }
	  /*
	   * Compute a translation vector, moving everything 1/10 
	   * the distance to the cursor. (Arbitrary scale factor)
	   */		
	  me->CurrentCamera->SetFocalPoint(
					   (FPoint[0]-RPoint[0])/10.0 + FPoint[0],
					   (FPoint[1]-RPoint[1])/10.0 + FPoint[1],
					   (FPoint[2]-RPoint[2])/10.0 + FPoint[2]);
	  me->CurrentCamera->SetPosition(
					 (FPoint[0]-RPoint[0])/10.0 + PPoint[0],
					 (FPoint[1]-RPoint[1])/10.0 + PPoint[1],
					 (FPoint[2]-RPoint[2])/10.0 + PPoint[2]);
      
	  if (me->LightFollowCamera)
	    {
	    /* get the first light */
	    me->CurrentLight->SetPosition(me->CurrentCamera->GetPosition());
	    me->CurrentLight->SetFocalPoint(me->CurrentCamera->GetFocalPoint());
	    }
	  me->RenderWindow->Render();
	  }
	  break;
	case VTKXI_ZOOM :
	  {
	  float zoomFactor;
	  float *clippingRange;
	  yf = ((me->Size[1] - HIWORD(me->LastPosition)) - me->Center[1])/
	    (float)me->Center[1];
	  zoomFactor = pow((double)1.1,(double)yf);
	  if (me->CurrentCamera->GetParallelProjection())
	    {
	    me->CurrentCamera->
	      SetParallelScale(me->CurrentCamera->GetParallelScale()/zoomFactor);
	    }
	  else
	    {
	    clippingRange = me->CurrentCamera->GetClippingRange();
	    me->CurrentCamera->SetClippingRange(clippingRange[0]/zoomFactor,
						clippingRange[1]/zoomFactor);
	    me->CurrentCamera->Dolly(zoomFactor);
	    }
	  me->RenderWindow->Render();
	  }
	  break;
	}
      break;
    default:
      return me->OldProc(hWnd,uMsg,wParam,lParam);
    }
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
