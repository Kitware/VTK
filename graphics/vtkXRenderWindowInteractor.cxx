/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowInteractor.cxx
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

typedef struct
{
  Visual	*visual;
  int	depth;
} OptionsRec;
OptionsRec	Options;

XtResource resources[] =
{
	{"visual", "Visual", XtRVisual, sizeof (Visual *),
	XtOffsetOf (OptionsRec, visual), XtRImmediate, NULL},
	{"depth", "Depth", XtRInt, sizeof (int),
	XtOffsetOf (OptionsRec, depth), XtRImmediate, NULL},
};

XrmOptionDescRec Desc[] =
{
	{"-visual", "*visual", XrmoptionSepArg, NULL},
	{"-depth", "*depth", XrmoptionSepArg, NULL}
};


// Construct an instance so that the light follows the camera motion.
vtkXRenderWindowInteractor::vtkXRenderWindowInteractor()
{
  this->State = VTKXI_START;
  this->App = 0;
  this->top = 0;
}

vtkXRenderWindowInteractor::~vtkXRenderWindowInteractor()
{
}

// Specify the Xt widget to use for interaction. This method is
// one of a couple steps that are required for setting up a
// vtkRenderWindowInteractor as a widget inside of another user 
// interface. You do not need to use this method if the render window
// will be a stand-alone window. This is only used when you want the
// render window to be a subwindow within a larger user interface.
// In that case, you must tell the render window what X display id
// to use, and then ask the render window what depth, visual and 
// colormap it wants. Then, you must create an Xt TopLevelShell with
// those settings. Then you can create the rest of your user interface
// as a child of the TopLevelShell you created. Eventually, you will 
// create a drawing area or some other widget to serve as the rendering
// window. You must use the SetWidget method to tell this Interactor
// about that widget. It's X and it's not terribly easy, but it looks cool.
void  vtkXRenderWindowInteractor::SetWidget(Widget foo)
{
  this->top = foo;
} 
  
// This will start up the X event loop and never return. If you
// call this method it will loop processing X events until the
// application is exited.
void vtkXRenderWindowInteractor::Start()
{
  XtAppMainLoop(this->App);
}

// Initializes the event handlers using an XtAppContext that you have
// provided.  This assumes that you want to own the event loop.
void vtkXRenderWindowInteractor::Initialize(XtAppContext app)
{
  this->App = app;

  this->Initialize();
}

// Initializes the event handlers without an XtAppContext.  This is
// good for when you don't have a user interface, but you still
// want to have mouse interaction.
void vtkXRenderWindowInteractor::Initialize()
{
  static int any_initialized = 0;
  static XtAppContext app;
  vtkXRenderWindow *ren;
  int depth;
  Colormap cmap;
  Visual  *vis;
  int *size;
  int *position;
  int argc = 0;

  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
    {
    vtkErrorMacro(<<"No renderer defined!");
    return;
    }

  this->Initialized = 1;
  ren = (vtkXRenderWindow *)(this->RenderWindow);

  // do initialization stuff if not initialized yet
  if (this->App)
    {
    any_initialized = 1;
    app = this->App;
    }
  if (!any_initialized)
    {
    XtToolkitInitialize();
    app = XtCreateApplicationContext();
    any_initialized = 1;
    }
  this->App = app;

  this->DisplayId = ren->GetDisplayId();
  if (!this->DisplayId)
    {
    this->DisplayId = 
      XtOpenDisplay(this->App,NULL,"VTK","vtk",NULL,0,&argc,NULL);
    }
  else
    {
    if (!this->top)
      {
      XtDisplayInitialize(this->App,this->DisplayId,
			  "VTK","vtk",NULL,0,&argc,NULL);
      }
    }
  
  // get the info we need from the RenderingWindow
  ren->SetDisplayId(this->DisplayId);

  size    = ren->GetSize();
  size[0] = ((size[0] > 0) ? size[0] : 300);
  size[1] = ((size[1] > 0) ? size[1] : 300);
  if (!this->top)
    {
    depth   = ren->GetDesiredDepth();
    cmap    = ren->GetDesiredColormap();
    vis     = ren->GetDesiredVisual();
    position= ren->GetPosition();

    this->top = XtVaAppCreateShell(this->RenderWindow->GetWindowName(),"vtk",
				   applicationShellWidgetClass,
				   this->DisplayId,
				   XtNdepth, depth,
				   XtNcolormap, cmap,
				   XtNvisual, vis,
				   XtNx, position[0],
				   XtNy, position[1],
				   XtNwidth, size[0],
				   XtNheight, size[1],
				   XtNinput, True,
				   XtNmappedWhenManaged, 0,
				   NULL);
    XtRealizeWidget(this->top);
    XSync(this->DisplayId,False);
    ren->SetWindowId(XtWindow(this->top));
    }
  else
    {
    XWindowAttributes attribs;
    
    XtRealizeWidget(this->top);
    XSync(this->DisplayId,False);
    ren->SetWindowId(XtWindow(this->top));

    //  Find the current window size 
    XGetWindowAttributes(this->DisplayId, 
                         XtWindow(this->top), &attribs);
    size[0] = attribs.width;
    size[1] = attribs.height;
    ren->SetSize(size[0], size[1]);
    }

  this->WindowId = XtWindow(this->top);
  ren->Render();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

void vtkXRenderWindowInteractor::Enable()
{
  // avoid cycles of calling Initialize() and Enable()
  if (this->Enabled)
    {
    return;
    }

  // Add the event handler to the system.
  // If we change the types of events processed by this handler, then
  // we need to change the Disable() routine to match.  In order for Disable()
  // to work properly, both the callback function AND the client data
  // passed to XtAddEventHandler and XtRemoveEventHandler must MATCH
  // PERFECTLY
  XtAddEventHandler(this->top,
		    KeyPressMask | ButtonPressMask | ExposureMask |
		    StructureNotifyMask | ButtonReleaseMask,
		    False,vtkXRenderWindowInteractorCallback,(XtPointer)this);
  this->Enabled = 1;

  this->Modified();
}

void vtkXRenderWindowInteractor::Disable()
{
  if (!this->Enabled)
    {
    return;
    }
  
  // Remove the event handler to the system.
  // If we change the types of events processed by this handler, then
  // we need to change the Disable() routine to match.  In order for Disable()
  // to work properly, both the callback function AND the client data
  // passed to XtAddEventHandler and XtRemoveEventHandler must MATCH
  // PERFECTLY.
  //
  // NOTE: we do not remove the StructureNotifyMask event since we need to
  // keep track of the window size (we will not render if we are disabled,
  // we simply track the window size changes for a possible Enable()).
  // Expose events are disabled.
  XtRemoveEventHandler(this->top,
		    KeyPressMask | ButtonPressMask | ExposureMask |
		    ButtonReleaseMask,
                    False,vtkXRenderWindowInteractorCallback,(XtPointer)this);

  this->Enabled = 0;
  this->Modified();
}


void vtkXRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRenderWindowInteractor::PrintSelf(os,indent);
  if (this->App)
    {
    os << indent << "App: " << this->App << "\n";
    }
  else
    {
    os << indent << "App: (none)\n";
    }
}


void vtkXRenderWindowInteractor::UpdateSize(int x,int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0])||(y != this->Size[1]))
    {
    this->Size[0] = x;
    this->Size[1] = y;
    this->RenderWindow->SetSize(x,y);
    }

}
 
void vtkXRenderWindowInteractor::StartRotate()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_ROTATE;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  XtAppAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}
void vtkXRenderWindowInteractor::EndRotate()
{
  if (this->State != VTKXI_ROTATE)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

void vtkXRenderWindowInteractor::StartZoom()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_ZOOM;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  XtAppAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}
void vtkXRenderWindowInteractor::EndZoom()
{
  if (this->State != VTKXI_ZOOM)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

void vtkXRenderWindowInteractor::StartPan()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  
  // calculation of focal depth has been moved to panning function.

  this->Preprocess = 1;
  this->State = VTKXI_PAN;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  XtAppAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}

void vtkXRenderWindowInteractor::EndPan()
{
  if (this->State != VTKXI_PAN)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

void vtkXRenderWindowInteractor::StartSpin()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_SPIN;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  XtAppAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}

void vtkXRenderWindowInteractor::EndSpin()
{
  if (this->State != VTKXI_SPIN)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

void vtkXRenderWindowInteractor::StartDolly()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_DOLLY;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  XtAppAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}

void vtkXRenderWindowInteractor::EndDolly()
{
  if (this->State != VTKXI_DOLLY)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

void vtkXRenderWindowInteractor::StartUniformScale()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_USCALE;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  XtAppAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}

void vtkXRenderWindowInteractor::EndUniformScale()
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
      // only render if we are currently accepting events
      if (me->GetEnabled())
	{
	me->GetRenderWindow()->Render();
	}
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

	// only render if we are currently accepting events
	if (me->GetEnabled())
	  {
	  me->GetRenderWindow()->Render();
	  }
	}
      }
      break;

    case ButtonPress: 
      me->SetEventPosition(((XButtonEvent*)event)->x,
			   me->Size[1] - ((XButtonEvent*)event)->y - 1);
      
      me->OldX = ((XButtonEvent*)event)->x;
      me->OldY = ((XButtonEvent*)event)->y;

      if (((XButtonEvent *)event)->state & ControlMask)
        {
	me->ControlMode = VTKXI_CONTROL_ON;
        }
      else
        {
	me->ControlMode = VTKXI_CONTROL_OFF;
        };
 
      me->FindPokedCamera(((XButtonEvent*)event)->x,
                          me->Size[1] - ((XButtonEvent*)event)->y - 1);

      if (me->ActorMode)
        {
        // Execute start method, if any
        if ( me->StartInteractionPickMethod ) 
          (*me->StartInteractionPickMethod)(me->StartInteractionPickMethodArg);
        
        // if in actor mode, select the actor below the mouse pointer
        me->InteractionPicker->Pick(((XButtonEvent*)event)->x,
                                    me->Size[1] -
                                    ((XButtonEvent*)event)->y - 1, 0.0,
                                    me->CurrentRenderer);
        
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
        
        if ( me->EndInteractionPickMethod ) 
          (*me->EndInteractionPickMethod)(me->EndInteractionPickMethodArg);
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
			   me->Size[1] - ((XButtonEvent*)event)->y - 1);

      // don't change actor or trackball modes in the middle of motion
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
	}
      
        me->OldX = 0.0;
        me->OldY = 0.0;
        if (me->ActorMode && me->ActorPicked)
          {
          me->HighlightActor(me->InteractionActor);
          }
        else if (me->ActorMode)
          {
          me->HighlightActor(NULL);
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
            exit(1);
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
                                  me->Size[1] - ((XKeyEvent*)event)->y - 1);
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
				me->Size[1] - ((XKeyEvent*)event)->y - 1);
	  ac = me->CurrentRenderer->GetActors();
	  for (ac->InitTraversal(); (anActor = ac->GetNextItem()); )
	    {
            for (anActor->InitPartTraversal();(aPart=anActor->GetNextPart()); )
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
			        me->Size[1] - ((XKeyEvent*)event)->y - 1);
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
	    me->RenderWindow->StereoRenderOff();
	    }
	  else
	    {
	    memcpy(me->PositionBeforeStereo,me->RenderWindow->GetPosition(),
		   sizeof(int)*2);
	    me->RenderWindow->StereoRenderOn();
	    }
	  me->RenderWindow->Render();
          break;

	case XK_p:
	case XK_P: //pick actors
          if (me->State == VTKXI_START)
            {
            me->FindPokedRenderer(((XKeyEvent*)event)->x,
                                  me->Size[1] - ((XKeyEvent*)event)->y - 1);
            // Execute start method, if any

            if ( me->StartPickMethod )
              {            
              (*me->StartPickMethod)(me->StartPickMethodArg);
              }
            me->Picker->Pick(((XButtonEvent*)event)->x,
                             me->Size[1] - ((XButtonEvent*)event)->y - 1, 0.0,
                             me->CurrentRenderer);
            
            // when user picks with their own picker, interaction actor is
            // reset, the picked item is highlighted.
            me->InteractionActor = NULL;
            me->ActorPicked = 0;
            me->HighlightActor(me->Picker->GetAssembly());
            
            if ( me->EndPickMethod )
              {
              (*me->EndPickMethod)(me->EndPickMethodArg);
              }
            }
	  break;

	case XK_j:
	case XK_J: //joystick style interaction
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
	case XK_T: //trackball style interaction
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
	case XK_O: //actor interaction
	  if (me->State == VTKXI_START) 
	    {
            if (me->ActorMode != VTKXI_ACTOR)
              {
              // reset the actor picking variables
              me->InteractionActor = NULL;
              me->ActorPicked = 0;
              me->HighlightActor(NULL);

              me->ActorMode = VTKXI_ACTOR;
              
              //vtkDebugMacro(<<"Swtich to Object/Actor interaction.");
              if (me->ActorModeMethod) 
                {
                (*me->ActorModeMethod)(me->ActorModeMethodArg);
                }
              }
            }
	  break;

	case XK_c:
	case XK_C: //camera interaction
	  if (me->State == VTKXI_START) 
	    {
            if (me->ActorMode != VTKXI_CAMERA)
              {
              // reset the actor picking variables
              me->InteractionActor = NULL;
              me->ActorPicked = 0;
              me->HighlightActor(NULL);

              me->ActorMode = VTKXI_CAMERA;
              // vtkDebugMacro(<<"Swtich to Camera mode interaction.");
              if (me->CameraModeMethod) 
                {
                (*me->CameraModeMethod)(me->CameraModeMethodArg);
                }
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
  unsigned int keys;

  me = (vtkXRenderWindowInteractor *)client_data;

  // get the pointer position
  XQueryPointer(me->DisplayId,me->WindowId,
		&root,&child,&root_x,&root_y,&x,&y,&keys);

  if (me->TimerMethod) 
    {
    me->SetEventPosition(x,me->Size[1] - y - 1);
    (*me->TimerMethod)(me->TimerMethodArg);
    };
  
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
        XtAppAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
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
        XtAppAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);

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
        XtAppAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
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
        XtAppAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
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
        XtAppAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
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
        XtAppAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
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
        XtAppAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
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
        XtAppAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
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
        XtAppAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
        }
      break;
    
    }
}  


