/*=========================================================================

  Program:   Visualization Toolkit
  Module:    %M%
  Language:  C++
  Date:      %G%
  Version:   %I%


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkWin32OglrRenderWindow.h"
#include "vtkWin32RenderWindowInteractor.h"
#include "vtkActor.h"
#include <math.h>

// states
#define VTKXI_START  0
#define VTKXI_ROTATE 1
#define VTKXI_ZOOM   2
#define VTKXI_PAN    3

// Description:
// Construct object so that light follows camera motion.
vtkWin32RenderWindowInteractor::vtkWin32RenderWindowInteractor()
{
  static timerId = 1;
  
  this->State = VTKXI_START;
  this->WindowId = 0;
  this->TimerId = timerId++;
}

vtkWin32RenderWindowInteractor::~vtkWin32RenderWindowInteractor()
{
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

// Description:
// Begin processing keyboard strokes.
void vtkWin32RenderWindowInteractor::Initialize()
{
  static int any_initialized = 0;
  vtkWin32OglrRenderWindow *ren;
  int depth;
  int *size;
  int *position;
  int argc = 0;
  vtkWin32OglrRenderWindow *tmp;

  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
    {
    vtkErrorMacro(<<"No renderer defined!");
    return;
    }

  if (this->Initialized) return;
  this->Initialized = 1;

  // get the info we need from the RenderingWindow
  ren = (vtkWin32OglrRenderWindow *)(this->RenderWindow);
  ren->Render();
  size    = ren->GetSize();
  position= ren->GetPosition();
  this->WindowId = ren->GetWindowId();
  this->OldProc = (WNDPROC)GetWindowLong(this->WindowId,GWL_WNDPROC);
  tmp = (vtkWin32OglrRenderWindow *)GetWindowLong(this->WindowId,GWL_USERDATA);
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
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  KillTimer(this->WindowId,this->TimerId);
  this->RenderWindow->Render();
}

void  vtkWin32RenderWindowInteractor::StartZoom()
{
  if (this->State != VTKXI_START) return;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  this->State = VTKXI_ZOOM;
  SetTimer(this->WindowId,this->TimerId,10,NULL);
}
void  vtkWin32RenderWindowInteractor::EndZoom()
{
  if (this->State != VTKXI_ZOOM) return;
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  KillTimer(this->WindowId,this->TimerId);
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

  SetTimer(this->WindowId,this->TimerId,10,NULL);
}
void  vtkWin32RenderWindowInteractor::EndPan()
{
  if (this->State != VTKXI_PAN) return;
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  KillTimer(this->WindowId,this->TimerId);
  this->RenderWindow->Render();
}

LRESULT CALLBACK vtkHandleMessage(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static LPARAM lastPos;
  float xf,yf;
  vtkWin32OglrRenderWindow *ren;
  vtkWin32RenderWindowInteractor *me;

  ren = (vtkWin32OglrRenderWindow *)GetWindowLong(hWnd,GWL_USERDATA);
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
      me->GetRenderWindow()->Render();
      return DefWindowProc(hWnd,uMsg,wParam,lParam);
      break;
	    
    case WM_SIZE: 
      me->UpdateSize(LOWORD(lParam),HIWORD(lParam)); 
      return me->OldProc(hWnd,uMsg,wParam,lParam);
      break;
	    
    case WM_LBUTTONDOWN: 
      me->FindPokedCamera(LOWORD(lParam),me->Size[1] - HIWORD(lParam));\
      if (wParam & MK_SHIFT)
	      {
	      me->StartPan();
	      }
      else
	      {
	      me->StartRotate(); 
	      }
      break;
	    
    case WM_LBUTTONUP: me->EndRotate(); me->EndPan(); break;
	    
    case WM_RBUTTONDOWN: 
      me->FindPokedCamera(LOWORD(lParam),me->Size[1] - HIWORD(lParam));
      me->StartZoom(); 
      break;
	    
    case WM_RBUTTONUP: me->EndZoom(); break;
	    
    case WM_MOUSEMOVE: lastPos = lParam; break;
	    
    case WM_CHAR:
      switch (wParam)
	      {
	      case 'e': 
          {
          // do a cleaner exit from windows
          // first free up the windows resources
          // me->RenderWindow->Delete();
          PostQuitMessage(0); 
          }
        break;
	    case 'u':
	      if (me->UserMethod) (*me->UserMethod)(me->UserMethodArg);
	      break;
	    case 'r':
	      me->FindPokedRenderer(LOWORD(lastPos),me->Size[1]-HIWORD(lastPos));
	      me->CurrentRenderer->ResetCamera();
	      me->RenderWindow->Render();
	      break;
	    case 'w':
	      {
	      vtkActorCollection *ac;
	      vtkActor *anActor, *aPart;
		
	      me->FindPokedRenderer(LOWORD(lastPos),me->Size[1]-HIWORD(lastPos));
	      ac = me->CurrentRenderer->GetActors();
	      for (ac->InitTraversal(); anActor = ac->GetNextItem(); )
	        {
	        for (anActor->InitPartTraversal(); aPart=anActor->GetNextPart(); )
	          {
	          aPart->GetProperty()->SetWireframe();
	          }
	        }
		
	      me->RenderWindow->Render();
	      }
	      break;
	    case 's':
	      {
	      vtkActorCollection *ac;
	      vtkActor *anActor, *aPart;
		
	      me->FindPokedRenderer(LOWORD(lastPos),me->Size[1]-HIWORD(lastPos));
	      ac = me->CurrentRenderer->GetActors();
	      for (ac->InitTraversal(); anActor = ac->GetNextItem(); )
	        {
	        for (anActor->InitPartTraversal(); aPart=anActor->GetNextPart(); )
	          {
	          aPart->GetProperty()->SetSurface();
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
	      me->FindPokedRenderer(LOWORD(lastPos),me->Size[1]-HIWORD(lastPos));
	      if (me->StartPickMethod)
	        {
	        (*me->StartPickMethod)(me->StartPickMethodArg);
	        }
	      me->Picker->Pick(LOWORD(lastPos), me->Size[1]-HIWORD(lastPos),
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
      switch (me->State)
	{
	case VTKXI_ROTATE :
	  xf = (LOWORD(lastPos) - me->Center[0]) * me->DeltaAzimuth;
	  yf = ((me->Size[1] - HIWORD(lastPos)) - me->Center[1]) * me->DeltaElevation;
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

	  xf = LOWORD(lastPos);
	  yf = me->Size[1] - HIWORD(lastPos);
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
	      yf = ((me->Size[1] - HIWORD(lastPos)) - me->Center[1])/
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
