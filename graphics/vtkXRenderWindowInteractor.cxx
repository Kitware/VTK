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
  this->TopLevelShell = NULL;
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

// This method will store the top level shell widget for the interactor.
// This method and the method invocation sequence applies for:
//     1 vtkRenderWindow-Interactor pair in a nested widget heirarchy
//     multiple vtkRenderWindow-Interactor pairs in the same top level shell
// It is not needed for
//     1 vtkRenderWindow-Interactor pair as the direct child of a top level shell
//     multiple vtkRenderWindow-Interactor pairs, each in its own top level shell
//
// The method, along with EnterNotify event, changes the keyboard focus among
// the widgets/vtkRenderWindow(s) so the Interactor(s) can receive the proper
// keyboard events. The following calls need to be made:
//     vtkRenderWindow's display ID need to be set to the top level shell's
//           display ID.
//     vtkXRenderWindowInteractor's Widget has to be set to the vtkRenderWindow's
//           container widget
//     vtkXRenderWindowInteractor's TopLevel has to be set to the top level
//           shell widget
// note that the procedure for setting up render window in a widget needs to
// be followed.  See vtkRenderWindowInteractor's SetWidget method.
//
// If multiple vtkRenderWindow-Interactor pairs in SEPARATE windows are desired,
// do not set the display ID (Interactor will create them as needed.  Alternatively,
// create and set distinct DisplayID for each vtkRenderWindow. Using the same
// display ID without setting the parent widgets will cause the display to be
// reinitialized every time an interactor is initialized), do not set the
// widgets (so the render windows would be in their own windows), and do
// not set TopLevelShell (each has its own top level shell already)
void vtkXRenderWindowInteractor::SetTopLevelShell(Widget topLevel)
{
  this->TopLevelShell = topLevel;
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
    vtkDebugMacro("toolkit init");
    XtToolkitInitialize();
    app = XtCreateApplicationContext();
    vtkDebugMacro("app ctx " << app);
    any_initialized = 1;
    }
  this->App = app;

  this->DisplayId = ren->GetDisplayId();
  if (!this->DisplayId)
    {
    vtkDebugMacro("opening display");
    this->DisplayId = 
      XtOpenDisplay(this->App,NULL,"VTK","vtk",NULL,0,&argc,NULL);
    vtkDebugMacro("opened display");
    }
  else
    {
    // if there is no parent widget
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
  ren->Start();
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
		    StructureNotifyMask | ButtonReleaseMask | EnterWindowMask,
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
		    ButtonReleaseMask | EnterWindowMask,
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

<<<<<<< vtkXRenderWindowInteractor.cxx
=======

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
  this->ExtAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
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
  this->ExtAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
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
  this->ExtAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
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
  this->ExtAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
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
  this->ExtAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
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
  this->ExtAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
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

void vtkXRenderWindowInteractor::StartTimer()
{
  if (this->State != VTKXI_START)
    {
    return;
    }
  this->Preprocess = 1;
  this->State = VTKXI_TIMER;
  this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
  this->ExtAddTimeOut(this->App,10,vtkXRenderWindowInteractorTimer,(XtPointer)this);
}

void vtkXRenderWindowInteractor::EndTimer()
{
  if (this->State != VTKXI_TIMER)
    {
    return;
    }
  this->State = VTKXI_START;
  this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
  this->RenderWindow->Render();
}

  
>>>>>>> 1.65
void vtkXRenderWindowInteractorCallback(Widget vtkNotUsed(w),
					XtPointer client_data,
					XEvent *event,
					Boolean *vtkNotUsed(ctd))
{
  vtkXRenderWindowInteractor *me;

  me = (vtkXRenderWindowInteractor *)client_data;

  // Huge piece of code deleted...
  // call
  //InteractionStyle->OnChar(...)
  //InteractionStyle->OnLButtonDown(...)
  //etc etc
  //and get shift and ctrl state

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
  me->GetMousePosition(&x, &y);

  if (me->TimerMethod)
    {
    me->SetEventPosition(x,me->Size[1] - y - 1);
    (*me->TimerMethod)(me->TimerMethodArg);
    };
<<<<<<< vtkXRenderWindowInteractor.cxx
=======
  
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
        me->ExtAddTimeOut(me->App,10,
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
        me->ExtAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
>>>>>>> 1.65

<<<<<<< vtkXRenderWindowInteractor.cxx
    // just call
    this->InteractorStyle->OnTimer();
    // not sure you need the guff above but I'll leave it there.
    // Mouse pos should be set by last mouse move or equiv message?
}
=======
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
        me->ExtAddTimeOut(me->App,10,
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
        me->ExtAddTimeOut(me->App,10,
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
        me->ExtAddTimeOut(me->App,10,
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
        me->ExtAddTimeOut(me->App,10,
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
        me->ExtAddTimeOut(me->App,10,
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
        me->ExtAddTimeOut(me->App,10,
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
        me->ExtAddTimeOut(me->App,10,
                        vtkXRenderWindowInteractorTimer,client_data);
        }
      break;
    
    case VTKXI_TIMER:
      me->ExtAddTimeOut(me->App,10,
                     vtkXRenderWindowInteractorTimer,client_data);
      break;
 
    }
}  
>>>>>>> 1.65

<<<<<<< vtkXRenderWindowInteractor.cxx
bool vtkXRenderWindowInteractor::CreateTimer(int timertype) {
    // Fix this
    this->AddTimeOut(this->App,10, vtkXRenderWindowInteractorTimer,client_data);
    return true;
}

bool vtkXRenderWindowInteractor::DestroyTimer(void) {
    return true;
}
=======
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
>>>>>>> 1.65

<<<<<<< vtkXRenderWindowInteractor.cxx
XtIntervalId vtkXRenderWindowInteractor::AddTimeOut(XtAppContext app_context,
=======
XtIntervalId vtkXRenderWindowInteractor::ExtAddTimeOut(XtAppContext app_context, 
>>>>>>> 1.65
			      unsigned long interval,
			      XtTimerCallbackProc proc,
			      XtPointer client_data)
{
  return XtAppAddTimeOut(app_context, interval, proc, client_data);
}

void vtkXRenderWindowInteractor::GetMousePosition(int *x, int *y)
{
  Window root,child;
  int root_x,root_y;
  unsigned int keys;

  XQueryPointer(this->DisplayId,this->WindowId,
		&root,&child,&root_x,&root_y,x,y,&keys);
}

void vtkXRenderWindowInteractor::Timer(XtPointer client_data,
                                       XtIntervalId *id) 
{
  vtkXRenderWindowInteractorTimer(client_data, id);
}

void vtkXRenderWindowInteractor::Callback(Widget w,
<<<<<<< vtkXRenderWindowInteractor.cxx
				   XtPointer client_data,
				   XEvent *event,
				   Boolean *ctd) 
{
  vtkXRenderWindowInteractorCallback(w, client_data, event, ctd);
}

=======
				   XtPointer client_data, 
				   XEvent *event, 
				   Boolean *ctd) {
 vtkXRenderWindowInteractorCallback(w, client_data, event, ctd);
}


void vtkXRenderWindowInteractor::ExtXRenderWindowInteractorTimer(XtPointer client_data,
				   XtIntervalId *id) {
  vtkXRenderWindowInteractorTimer(client_data, id);
}

void vtkXRenderWindowInteractor::ExtXRenderWindowInteractorCallback(Widget w,
				   XtPointer client_data, 
				   XEvent *event, 
				   Boolean *ctd) {
 vtkXRenderWindowInteractorCallback(w, client_data, event, ctd);
}














>>>>>>> 1.65
