/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderTclWindowInteractor.cxx
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
#include <X11/X.h>
#include <X11/keysym.h>
#include "vtkXRenderWindowInteractor.h"
#include "vtkXRenderWindow.h"
#include "vtkActor.h"
#include <X11/Shell.h>
#include <math.h>
#include "tk.h"

// steal the first two elements of the TkMainInfo stuct
// we don't care about the rest of the elements.
typedef struct TkMainInfo
{
  int refCount;
  struct TkWindow *winPtr;
};

extern TkMainInfo *tkMainWindowList;

// returns 1 if done
static int vtkTclEventProc(XtPointer clientData,XEvent *event)
{
  Boolean ctd;
  vtkXRenderWindow *rw;
      
  rw = (vtkXRenderWindow *)
    (((vtkXRenderWindowInteractor *)clientData)->GetRenderWindow());
  
  if (rw->GetWindowId() == ((XAnyEvent *)event)->window)
    {
    vtkXRenderWindowInteractorCallback((Widget)NULL,clientData, event, &ctd);
    ctd = 0;
    }
  else
    {
    ctd = 1;
    }

  return !ctd;
}

static void vtkXTclTimerProc(ClientData clientData)
{
  XtIntervalId id;
  vtkXRenderWindowInteractorTimer((XtPointer)clientData,&id);
}



// Construct object so that light follows camera motion.
vtkXRenderWindowInteractor::vtkXRenderWindowInteractor()
{
  this->State = VTKXI_START;
  this->App = 0;
  this->top = 0;
}

vtkXRenderWindowInteractor::~vtkXRenderWindowInteractor()
{
  if (this->Initialized)
    {
    Tk_DeleteGenericHandler((Tk_GenericProc *)vtkTclEventProc,
			    (ClientData)this);
    }
}

void  vtkXRenderWindowInteractor::SetWidget(Widget foo)
{
  this->top = foo;
} 

void  vtkXRenderWindowInteractor::Start()
{
  Tk_MainLoop();
}

// Initializes the event handlers
void vtkXRenderWindowInteractor::Initialize(XtAppContext app)
{
  this->App = app;

  this->Initialize();
}

// Begin processing keyboard strokes.
void vtkXRenderWindowInteractor::Initialize()
{
  vtkXRenderWindow *ren;
  int depth;
  Colormap cmap;
  Visual  *vis;
  int *size;
  int *position;

  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
    {
    vtkErrorMacro(<<"No renderer defined!");
    return;
    }

  this->Initialized = 1;
  ren = (vtkXRenderWindow *)(this->RenderWindow);

  // use the same display as tcl/tk
  ren->SetDisplayId(Tk_Display(tkMainWindowList->winPtr));
  this->DisplayId = ren->GetDisplayId();
  
  // get the info we need from the RenderingWindow
  depth   = ren->GetDesiredDepth();
  cmap    = ren->GetDesiredColormap();
  vis     = ren->GetDesiredVisual();
  size    = ren->GetSize();
  position= ren->GetPosition();
  
  size = ren->GetSize();
  ren->Render();
  this->WindowId = ren->GetWindowId();
  size = ren->GetSize();

  this->Size[0] = size[0];
  this->Size[1] = size[1];

  XSelectInput(this->DisplayId,this->WindowId,
	       KeyPressMask | ButtonPressMask | ExposureMask |
		    StructureNotifyMask | ButtonReleaseMask);

  // add in tcl init stuff
  Tk_CreateGenericHandler((Tk_GenericProc *)vtkTclEventProc,(ClientData)this);
}

void vtkXRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRenderWindowInteractor::PrintSelf(os,indent);
}


void  vtkXRenderWindowInteractor::UpdateSize(int x,int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0])||(y != this->Size[1]))
    {
    this->Size[0] = x;
    this->Size[1] = y;
    this->RenderWindow->SetSize(x,y);
    }

}
 
void  vtkXRenderWindowInteractor::StartRotate()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_ROTATE;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)this);
}

void  vtkXRenderWindowInteractor::EndRotate()
{
  if (this->State != VTKXI_ROTATE)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

void  vtkXRenderWindowInteractor::StartZoom()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_ZOOM;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)this);
}

void  vtkXRenderWindowInteractor::EndZoom()
{
  if (this->State != VTKXI_ZOOM)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

void  vtkXRenderWindowInteractor::StartPan()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  // calculation of focal depth has been moved to panning function.
  
  this->Preprocess = 1;
  this->State = VTKXI_PAN;  
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)this);
}

void  vtkXRenderWindowInteractor::EndPan()
{
  if (this->State != VTKXI_PAN)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

void  vtkXRenderWindowInteractor::StartSpin()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_SPIN;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)this);
}


void  vtkXRenderWindowInteractor::EndSpin()
{
  if (this->State != VTKXI_SPIN)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

void  vtkXRenderWindowInteractor::StartDolly()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_DOLLY;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)this);
}


void  vtkXRenderWindowInteractor::EndDolly()
{
  if (this->State != VTKXI_DOLLY)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}


void  vtkXRenderWindowInteractor::StartUniformScale()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_USCALE;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)this);
}


void  vtkXRenderWindowInteractor::EndUniformScale()
{
  if (this->State != VTKXI_USCALE)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}


void vtkXRenderWindowInteractorCallback(Widget vtkNotUsed(w),
					XtPointer client_data, 
					XEvent *event, 
					Boolean *vtkNotUsed(ctd))
{
  vtkXRenderWindowInteractor *me;
  
  me = (vtkXRenderWindowInteractor *)client_data;
  
  switch (event->type) 
    {
    case Expose:
      {
      XEvent result;
      while (XCheckTypedWindowEvent(me->DisplayId, me->WindowId,
				    Expose, &result))
	{
	// just getting the expose configure event
	event = &result;
	}
      me->GetRenderWindow()->Render();
      }
      break;
      
    case ConfigureNotify: 
      {
      XEvent result;
      while (XCheckTypedWindowEvent(me->DisplayId, me->WindowId,
				    ConfigureNotify, &result))
	{
	// just getting the last configure event
	event = &result;
	}
      if ((((XConfigureEvent *)event)->width != me->Size[0]) ||
	  (((XConfigureEvent *)event)->height != me->Size[1]))
	{
	me->UpdateSize(((XConfigureEvent *)event)->width,
		       ((XConfigureEvent *)event)->height); 

	me->GetRenderWindow()->Render(); 
	}
      }
      break;

    case ButtonPress: 
      me->SetEventPosition(((XButtonEvent*)event)->x,
			   ((XButtonEvent*)event)->y);

      
      me->OldX = ((XButtonEvent*)event)->x;
      me->OldY = ((XButtonEvent*)event)->y;

      if (((XButtonEvent *)event)->state & ControlMask)
        {
	me->ControlMode = VTKXI_CONTROL_ON;
        }
      else
        {
	me->ControlMode = VTKXI_CONTROL_OFF;
        }

      me->FindPokedCamera(((XButtonEvent*)event)->x,
                          me->Size[1] - ((XButtonEvent*)event)->y);
      
      if (me->ActorMode)
        {
        // Execute start method, if any
        if ( me->StartPickMethod ) 
          {
          (*me->StartPickMethod)(me->StartPickMethodArg);
          }
        
        me->Picker->Pick(((XButtonEvent*)event)->x,
                         me->Size[1] - ((XButtonEvent*)event)->y, 0.0,
                         me->CurrentRenderer);

        // if in actor mode, select the actor below the mouse pointer
        me->InteractionPicker->Pick(((XButtonEvent*)event)->x,
                                    me->Size[1] - 
                                    ((XButtonEvent*)event)->y, 0.0,
                                    me->CurrentRenderer);
        me->InteractionActor = me->InteractionPicker->GetAssembly();
        // refine the answer to whether an actor was picked.  CellPicker()
        // returns true from Pick() if the bounding box was picked,
        // but we only want something to be picked if a cell was actually
        // selected
        me->ActorPicked = (me->InteractionActor != NULL);
        // highlight actor at the end of interaction
        
        if ( me->EndPickMethod )
          {
          (*me->EndPickMethod)(me->EndPickMethodArg);
          }
        }

      switch (((XButtonEvent *)event)->button)
	{
	case Button1: 
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
	  break;
          
	case Button2: 
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
          
	case Button3: 
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
          
	}
      break;

    case ButtonRelease: 
      me->SetEventPosition(((XButtonEvent*)event)->x,
			   ((XButtonEvent*)event)->y);

      // don't change actor or trackball mode in the middle of motion
      // don't change control mode in the middle of mouse movement

      switch (((XButtonEvent *)event)->button)
	{
	case Button1:
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
	  break;
          
	case Button2:
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
	  break;
          
	case Button3: 
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
	  break;
          
	};

        me->OldX = 0.0;
        me->OldY = 0.0;
        if (me->ActorMode && me->ActorPicked)
          {
          me->HighlightActor(me->InteractionActor);
          }
      break;

    case KeyPress:
      KeySym ks;
      static char buffer[20];

      XLookupString((XKeyEvent *)event,buffer,20,&ks,NULL);

      switch (ks)
	{
	case XK_q:
	case XK_Q:
	case XK_e:
	case XK_E: 
          if (me->ExitMethod)
            {
            (*me->ExitMethod)(me->ExitMethodArg);
            }
	  else
            {
            Tcl_Exit(1);
            }
	  break;
          
	case XK_u:
	case XK_U:
          if (me->UserMethod)
            {
            (*me->UserMethod)(me->UserMethodArg);
            }
          break;
          
	case XK_r:
	case XK_R: //reset
          if (me->ActorMode)
            {
            //vtkDebugMacro(<<"Please switch to camera mode then reset");
	    }
          else
            {
            me->FindPokedRenderer(((XKeyEvent*)event)->x,
                                  me->Size[1] - ((XKeyEvent*)event)->y);
            me->CurrentRenderer->ResetCamera();
            me->RenderWindow->Render();
	    }
          break;

	case XK_w:
	case XK_W: //change all actors to wireframe
          {
	  vtkActorCollection *ac;
	  vtkActor *anActor, *aPart;
	  
          me->FindPokedRenderer(((XKeyEvent*)event)->x,
				me->Size[1] - ((XKeyEvent*)event)->y);
	  ac = me->CurrentRenderer->GetActors();
	  for (ac->InitTraversal(); (anActor = ac->GetNextItem()); )
	    {
            for (anActor->InitPartTraversal();(aPart=anActor->GetNextPart());)
              {
              aPart->GetProperty()->SetRepresentationToWireframe();
              }
	    }
	  
	  me->RenderWindow->Render();
	  }
          break;

	case XK_s:
	case XK_S: //change all actors to "surface" or solid
	  {
          vtkActorCollection *ac;
	  vtkActor *anActor, *aPart;
	  
          me->FindPokedRenderer(((XKeyEvent*)event)->x,
			        me->Size[1] - ((XKeyEvent*)event)->y);
	  ac = me->CurrentRenderer->GetActors();
	  for (ac->InitTraversal(); (anActor = ac->GetNextItem()); )
	    {
            for (anActor->InitPartTraversal();(aPart=anActor->GetNextPart()); )
              {
              aPart->GetProperty()->SetRepresentationToSurface();
              }
	    }
	  
	  me->RenderWindow->Render();
	  }
          break;

	case XK_3: //3d stereo
	  // prepare the new window
	  if (me->RenderWindow->GetStereoRender())
	    {
	    if (me->RenderWindow->GetRemapWindow())
	      {
	      me->SetupNewWindow(1);
	      }
	    me->RenderWindow->StereoRenderOff();
	    }
	  else
	    {
	    memcpy(me->PositionBeforeStereo,me->RenderWindow->GetPosition(),
		   sizeof(int)*2);
	    if (me->RenderWindow->GetRemapWindow())
	      {
	      me->SetupNewWindow(1);
	      }
	    me->RenderWindow->StereoRenderOn();
	    }
	  me->RenderWindow->Render();
	  if (me->RenderWindow->GetRemapWindow())
	    {
	    me->FinishSettingUpNewWindow();
	    }
          break;

	case XK_p:
	case XK_P: //pick actors
          me->FindPokedRenderer(((XKeyEvent*)event)->x,
                                me->Size[1] - ((XKeyEvent*)event)->y);

          // Execute start method, if any 
          if ( me->StartPickMethod )
            {            
            (*me->StartPickMethod)(me->StartPickMethodArg);
            }
          me->Picker->Pick(((XButtonEvent*)event)->x,
                           me->Size[1] - ((XButtonEvent*)event)->y, 0.0,
                           me->CurrentRenderer);

          // set actor in all modes, so when switching, actor still selected
          me->InteractionPicker->Pick(((XButtonEvent*)event)->x,
                                      me->Size[1] -
                                      ((XButtonEvent*)event)->y, 0.0,
                                      me->CurrentRenderer);
          me->InteractionActor = (me->InteractionPicker)->GetAssembly();
          me->ActorPicked = (me->InteractionActor != NULL);
          me->HighlightActor(me->InteractionActor);
          
          if ( me->EndPickMethod ) 
            {
            (*me->EndPickMethod)(me->EndPickMethodArg);
            }
	  break;
          
	case XK_j:
	case XK_J: // joystick style interaction
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
          
	case XK_t:
	case XK_T: // trackball style interaction
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
          
	case XK_o:
	case XK_O: // actor interaction
	  if (me->State == VTKXI_START) 
	    {
	    me->ActorMode = VTKXI_ACTOR;
	    //vtkDebugMacro(<<"Swtich to Actor interaction.");
	    if (me->ActorModeMethod) 
	      {
	      (*me->ActorModeMethod)(me->ActorModeMethodArg);
	      }	    
            }
	  break;
          
	case XK_c:
	case XK_C: // camera interaction
          if (me->State == VTKXI_START) 
	    {
	    me->ActorMode = VTKXI_CAMERA;
	    //vtkDebugMacro(<<"Swtich to Camera interaction.");
	    if (me->CameraModeMethod) 
	      {
	      (*me->CameraModeMethod)(me->CameraModeMethodArg);
	      }
            }
	  break;
          
        }
      break;
    }
}

void vtkXRenderWindowInteractorTimer(XtPointer client_data,
				     XtIntervalId *vtkNotUsed(id))
{
  vtkXRenderWindowInteractor *me;
  Window root,child;
  int root_x,root_y;
  int x,y;
  float xf,yf;
  unsigned int keys;

  me = (vtkXRenderWindowInteractor *)client_data;

  // get the pointer position
  XQueryPointer(me->DisplayId,me->WindowId,
		&root,&child,&root_x,&root_y,&x,&y,&keys);

  if (me->TimerMethod) 
    {
    me->SetEventPosition(x,y);
    (*me->TimerMethod)(me->TimerMethodArg);
    }

  switch (me->State)
    {
    case VTKXI_ROTATE:
      if (me->ActorMode && me->ActorPicked)
        { 
        if (me->TrackballMode)
          {
          me->TrackballRotateActor(x, y);
          }
        else
          {
          me->JoystickRotateActor(x, y);
          }
        Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)client_data);
        }
      else if (!(me->ActorMode))
        {
        if (me->TrackballMode)
          {
          me->TrackballRotateCamera(x, y);
          }
        else
          {
          me->JoystickRotateCamera(x, y);
          }
        Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)client_data);
        }
      break;
    
    case VTKXI_PAN:
      if (me->ActorMode && me->ActorPicked)
        { 
        if (me->TrackballMode)
          {
          me->TrackballPanActor(x, y);
          }
        else
          {
          me->JoystickPanActor(x, y);
          }
        Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)client_data);
        }
      else if (!(me->ActorMode))
        {
        if (me->TrackballMode)
          {
          me->TrackballPanCamera(x, y);
          }
        else
          {
          me->JoystickPanCamera(x, y);
          }
        Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)client_data);
        }
      break;
    
    case VTKXI_ZOOM:
      if (!(me->ActorMode))
        {
        if (me->TrackballMode)
          {
          me->TrackballDollyCamera(x, y);
          }
        else
          {
          me->JoystickDollyCamera(x, y);
          }
        Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)client_data);
        }
      break;
    
    case VTKXI_SPIN:
      if (me->ActorMode && me->ActorPicked)
        {
        if (me->TrackballMode)
          {
          me->TrackballSpinActor(x, y);
          }
        else
          {
          me->JoystickSpinActor(x, y);
          }
        Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)client_data);
        }
      else if (!(me->ActorMode))
        {
        if (me->TrackballMode)
          {
          me->TrackballSpinCamera(x, y);
          }
        else
          {
          me->JoystickSpinCamera(x, y);
          }
        Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)client_data);
        }
      break;

    case VTKXI_DOLLY:
      if (me->ActorMode && me->ActorPicked)
        {
        if (me->TrackballMode)
          {
          me->TrackballDollyActor(x, y);
          }
        else
          {
          me->JoystickDollyActor(x, y);
          }
        Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)client_data);
        }
      break;
    
    case VTKXI_USCALE:
      if (me->ActorMode && me->ActorPicked)
        {
        if (me->TrackballMode)
          {
          me->TrackballScaleActor(x, y);
          }
        else
          {
          me->JoystickScaleActor(x, y);
          }
        Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)client_data);
        }
      break;
      
    }
}  


// Setup a new window before a WindowRemap
void vtkXRenderWindowInteractor::SetupNewWindow(int Stereo)
{
  vtkXRenderWindow *ren;
  int depth;
  Colormap cmap;
  Visual  *vis;
  int *size;
  int *position;
  int zero_pos[2];
  
  // get the info we need from the RenderingWindow
  ren = (vtkXRenderWindow *)(this->RenderWindow);
  this->DisplayId = ren->GetDisplayId();
  depth   = ren->GetDesiredDepth();
  cmap    = ren->GetDesiredColormap();
  vis     = ren->GetDesiredVisual();
  size    = ren->GetSize();
  position= ren->GetPosition();

  if (Stereo)
    {
    if (this->RenderWindow->GetStereoRender())
      {
      position = this->PositionBeforeStereo;
      }
    else
      {
      zero_pos[0] = 0;
      zero_pos[1] = 0;
      position = zero_pos;
      }
    }
}

// Finish setting up a new window after the WindowRemap.
void vtkXRenderWindowInteractor::FinishSettingUpNewWindow()
{
  int *size;

  // free the previous widget
  XSync(this->DisplayId,False);
  this->WindowId = ((vtkXRenderWindow *)(this->RenderWindow))->GetWindowId();
  XSync(this->DisplayId,False);

  XSelectInput(this->DisplayId,this->WindowId,
		    KeyPressMask | ButtonPressMask | ExposureMask |
		    StructureNotifyMask | ButtonReleaseMask);

  size = this->RenderWindow->GetSize();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

