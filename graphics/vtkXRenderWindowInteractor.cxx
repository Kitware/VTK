/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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


// states
#define VTKXI_START  0
#define VTKXI_ROTATE 1
#define VTKXI_ZOOM   2
#define VTKXI_PAN    3


// Description:
// Construct an instance so that the light follows the camera motion.
vtkXRenderWindowInteractor::vtkXRenderWindowInteractor()
{
  this->State = VTKXI_START;
  this->App = 0;
  this->top = 0;
  this->WaitingForMarker = 0;
}

vtkXRenderWindowInteractor::~vtkXRenderWindowInteractor()
{
}

// Description:
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
  
// Description:
// This will start up the X event loop and never return. If you
// call this method it will loop processing X events until the
// application is exited.
void vtkXRenderWindowInteractor::Start()
{
  XtAppMainLoop(this->App);
}

// Description: 
// Initializes the event handlers using an XtAppContext that you have
// provided.  This assumes that you want to own the event loop.
void vtkXRenderWindowInteractor::Initialize(XtAppContext app)
{
  this->App = app;

  this->Initialize();
}

// Description:
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
    }

  XtRealizeWidget(this->top);

  /* add callback */
  XSync(this->DisplayId,False);
  ren->SetWindowId(XtWindow(this->top));
  this->WindowId = XtWindow(this->top);
  ren->Render();
  XtAddEventHandler(this->top,
		    KeyPressMask | ButtonPressMask | ExposureMask |
		    StructureNotifyMask | ButtonReleaseMask,
		    False,vtkXRenderWindowInteractorCallback,(XtPointer)this);
  this->Size[0] = size[0];
  this->Size[1] = size[1];
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
  if (this->State != VTKXI_START) return;
  this->State = VTKXI_ROTATE;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  XtAppAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}
void  vtkXRenderWindowInteractor::EndRotate()
{
  if (this->State != VTKXI_ROTATE) return;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->State = VTKXI_START;
}

void  vtkXRenderWindowInteractor::StartZoom()
{
  if (this->State != VTKXI_START) return;
  this->State = VTKXI_ZOOM;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  XtAppAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}
void  vtkXRenderWindowInteractor::EndZoom()
{
  if (this->State != VTKXI_ZOOM) return;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->State = VTKXI_START;
}

void  vtkXRenderWindowInteractor::StartPan()
{
  float *FocalPoint;
  float *Result;

  if (this->State != VTKXI_START) return;

  this->State = VTKXI_PAN;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);

  // calculate the focal depth since we'll be using it a lot
  FocalPoint = this->CurrentCamera->GetFocalPoint();
      
  this->CurrentRenderer->SetWorldPoint(FocalPoint[0],FocalPoint[1],
				       FocalPoint[2],1.0);
  this->CurrentRenderer->WorldToDisplay();
  Result = this->CurrentRenderer->GetDisplayPoint();
  this->FocalDepth = Result[2];

  XtAppAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}
void  vtkXRenderWindowInteractor::EndPan()
{
  if (this->State != VTKXI_PAN) return;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->State = VTKXI_START;
}

void vtkXRenderWindowInteractorCallback(Widget vtkNotUsed(w),
					XtPointer client_data, 
					XEvent *event, 
					Boolean *vtkNotUsed(ctd))
{
  XEvent marker;
  vtkXRenderWindowInteractor *me;

  me = (vtkXRenderWindowInteractor *)client_data;

  switch (event->type) 
    {
    case ClientMessage :
      me->WaitingForMarker = 0;
      break;
  
    case Expose : 
      if (!me->WaitingForMarker)
	{
	// put in a marker
	marker.type = ClientMessage;
	marker.xclient.display = me->DisplayId;
	marker.xclient.window = me->WindowId;
	marker.xclient.format = 32;
	XSendEvent(me->DisplayId, me->WindowId,
		   (Bool) 0, (long) 0, &marker);
	XSync(me->DisplayId,False);
	me->WaitingForMarker = 1;
	me->GetRenderWindow()->Render();
	}
      break;
      
    case ConfigureNotify : 
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
	// while we are at it clear out any expose events
	// put in a marker
	marker.type = ClientMessage;
	marker.xclient.display = me->DisplayId;
	marker.xclient.window = me->WindowId;
	marker.xclient.format = 32;
	XSendEvent(me->DisplayId, me->WindowId,
		   (Bool) 0, (long) 0, &marker);
	XSync(me->DisplayId,False);
	me->WaitingForMarker = 1;
	me->GetRenderWindow()->Render(); 
	}
      }
      break;

    case ButtonPress : 
      {
      switch (((XButtonEvent *)event)->button)
	{
	case Button1 : 
          me->FindPokedCamera(((XButtonEvent*)event)->x,
			      me->Size[1] - ((XButtonEvent*)event)->y);
	  me->StartRotate(); 
	  break;
	case Button2 : 
          me->FindPokedCamera(((XButtonEvent*)event)->x,
			      me->Size[1] - ((XButtonEvent*)event)->y);
	  me->StartPan(); 
	  break;
	case Button3 : 
          me->FindPokedCamera(((XButtonEvent*)event)->x,
			      me->Size[1] - ((XButtonEvent*)event)->y);
	  me->StartZoom(); 
	  break;
	}
      }
      break;

    case ButtonRelease : 
      {
      switch (((XButtonEvent *)event)->button)
	{
	case Button1 : me->EndRotate(); break;
	case Button2 : me->EndPan(); break;
	case Button3 : me->EndZoom(); break;
	}
      }
      break;

    case KeyPress :
      {
      KeySym ks;
      static char buffer[20];

      XLookupString((XKeyEvent *)event,buffer,20,&ks,NULL);
      switch (ks)
	{
	case XK_e : exit(1); break;
	case XK_u :
	  if (me->UserMethod) (*me->UserMethod)(me->UserMethodArg);
	  break;
	case XK_r : //reset
	  {
          me->FindPokedRenderer(((XKeyEvent*)event)->x,
			        me->Size[1] - ((XKeyEvent*)event)->y);
	  me->CurrentRenderer->ResetCamera();
	  me->RenderWindow->Render();
          }
	  break;

	case XK_w : //change all actors to wireframe
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
              aPart->GetProperty()->SetWireframe();
              }
	    }
	  
	  me->RenderWindow->Render();
	  }
	  break;

	case XK_s : //change all actors to "surface" or solid
	  {
	  vtkActorCollection *ac;
	  vtkActor *anActor, *aPart;
	  
          me->FindPokedRenderer(((XKeyEvent*)event)->x,
			        me->Size[1] - ((XKeyEvent*)event)->y);
	  ac = me->CurrentRenderer->GetActors();
	  for (ac->InitTraversal(); (anActor = ac->GetNextItem()); )
	    {
            for (anActor->InitPartTraversal(); 
		 (aPart=anActor->GetNextPart()); )
              {
              aPart->GetProperty()->SetSurface();
              }
	    }
	  
	  me->RenderWindow->Render();
          }
	  break;

	case XK_3 : //3d stereo
	  {
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
          }
	  break;

	case XK_p : //pick actors
	  {
          me->FindPokedRenderer(((XKeyEvent*)event)->x,
			        me->Size[1] - ((XKeyEvent*)event)->y);
          // Execute start method, if any

          if ( me->StartPickMethod ) 
            (*me->StartPickMethod)(me->StartPickMethodArg);
          me->Picker->Pick(((XButtonEvent*)event)->x,
                             me->Size[1] - ((XButtonEvent*)event)->y, 0.0,
                             me->CurrentRenderer);
          me->HighlightActor(me->Picker->GetAssembly());
          if ( me->EndPickMethod ) 
            (*me->EndPickMethod)(me->EndPickMethodArg);
          }
	  break;
        }
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

  switch (me->State)
    {
    case VTKXI_ROTATE :
      // get the pointer position
      XQueryPointer(me->DisplayId,me->WindowId,
		    &root,&child,&root_x,&root_y,&x,&y,&keys);
      xf = (x - me->Center[0]) * me->DeltaAzimuth;
      yf = ((me->Size[1] - y) - me->Center[1]) * me->DeltaElevation;
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
      XtAppAddTimeOut(me->App,10,vtkXRenderWindowInteractorTimer,client_data);
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

      // get the pointer position
      XQueryPointer(me->DisplayId,me->WindowId,
		    &root,&child,&root_x,&root_y,&x,&y,&keys);

      APoint[0] = x;
      APoint[1] = me->Size[1] - y;
      APoint[2] = me->FocalDepth;
      me->CurrentRenderer->SetDisplayPoint(APoint);
      me->CurrentRenderer->DisplayToWorld();
      memcpy(RPoint,me->CurrentRenderer->GetWorldPoint(),sizeof(float)*4);
      if (RPoint[3])
	{
	RPoint[0] = RPoint[0]/RPoint[3];
	RPoint[1] = RPoint[1]/RPoint[3];
	RPoint[2] = RPoint[2]/RPoint[3];
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
      
      me->RenderWindow->Render();
      XtAppAddTimeOut(me->App,10,vtkXRenderWindowInteractorTimer,client_data);
      }
      break;
    case VTKXI_ZOOM :
      {
      float zoomFactor;
      float *clippingRange;

      // get the pointer position
      XQueryPointer(me->DisplayId,me->WindowId,
		    &root,&child,&root_x,&root_y,&x,&y,&keys);
      yf = ((me->Size[1] - y) - me->Center[1])/(float)me->Center[1];
      zoomFactor = pow(1.1,yf);
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
      XtAppAddTimeOut(me->App,10,vtkXRenderWindowInteractorTimer,client_data);
      }
      break;
    }
}  



// Description:
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

  this->oldTop = this->top;
  
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
				 XtNmappedWhenManaged, 0,
				 NULL);

  XtRealizeWidget(this->top);
  
  /* add callback */
  XSync(this->DisplayId,False);
  ren->SetNextWindowId(XtWindow(this->top));
  this->WindowId = XtWindow(this->top);
}

// Description:
// Finish setting up a new window after the WindowRemap.
void vtkXRenderWindowInteractor::FinishSettingUpNewWindow()
{
  int *size;

  // free the previous widget
  XtDestroyWidget(this->oldTop);
  XSync(this->DisplayId,False);

  XtAddEventHandler(this->top,
		    KeyPressMask | ButtonPressMask | ExposureMask |
		    StructureNotifyMask | ButtonReleaseMask,
		    False,vtkXRenderWindowInteractorCallback,(XtPointer)this);

  size = this->RenderWindow->GetSize();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

