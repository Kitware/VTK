/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowInteractor.cxx
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/keysym.h>
#include "vtkXRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#include "vtkXOpenGLRenderWindow.h"
#include "vtkActor.h"
#include <X11/Shell.h>
#include <math.h>
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

//----------------------------------------------------------------------------
vtkXRenderWindowInteractor* vtkXRenderWindowInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXRenderWindowInteractor");
  if(ret)
    {
    return (vtkXRenderWindowInteractor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXRenderWindowInteractor;
}




typedef struct
{
  Visual	*visual;
  int	depth;
} OptionsRec;
OptionsRec	Options;

XtResource resources[] =
{
	{(char *) "visual", (char *) "Visual", XtRVisual, sizeof (Visual *),
	XtOffsetOf (OptionsRec, visual), XtRImmediate, NULL},
	{(char *) "depth", (char *) "Depth", XtRInt, sizeof (int),
	XtOffsetOf (OptionsRec, depth), XtRImmediate, NULL},
};

XrmOptionDescRec Desc[] =
{
	{(char *) "-visual", (char *) "*visual", XrmoptionSepArg, NULL},
	{(char *) "-depth", (char *) "*depth", XrmoptionSepArg, NULL}
};


// Construct an instance so that the light follows the camera motion.
vtkXRenderWindowInteractor::vtkXRenderWindowInteractor()
{
  this->App = 0;
  this->top = 0;
  this->TopLevelShell = NULL;
  this->BreakLoopFlag = 0;
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

// This function replaces TerminateApp() if Start() is called.
// This way, when the user hits the exit key, Start() returns
// and the application continues instead of calling exit().
// With this change, it is possible to have clean-up code after
// the interactor loop.
void BreakXtLoop(void *iren)
{
  ((vtkXRenderWindowInteractor*)iren)->SetBreakLoopFlag(1);
}

// This will start up the X event loop and never return. If you
// call this method it will loop processing X events until the
// application is exited.
void vtkXRenderWindowInteractor::Start()
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(vtkCommand::StartEvent))
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    return;
    }

  if (!this->Initialized)
    {
    this->Initialize();
    }
  if (! this->Initialized )
    {
    return;
    }

  this->SetExitMethod(BreakXtLoop, this);
  this->BreakLoopFlag = 0;
  do 
    {
    XEvent event;
    XtAppNextEvent(this->App, &event);
    XtDispatchEvent(&event);
    } 
  while (this->BreakLoopFlag == 0);
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
  vtkXOpenGLRenderWindow *ren;
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
  ren = (vtkXOpenGLRenderWindow *)(this->RenderWindow);

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
		    KeyPressMask | KeyReleaseMask | ButtonPressMask | 
		    ExposureMask | StructureNotifyMask | ButtonReleaseMask |
		    EnterWindowMask | LeaveWindowMask | 
		    PointerMotionHintMask | PointerMotionMask,
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
		    KeyPressMask | KeyReleaseMask | ButtonPressMask | 
		    ExposureMask | ButtonReleaseMask |
		    EnterWindowMask | LeaveWindowMask | 
		    PointerMotionHintMask | PointerMotionMask,
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
  os << indent << "BreakLoopFlag: " 
     << (this->BreakLoopFlag ? "On\n" : "Off\n");
}

//void vtkXRenderWindowInteractor::UpdateSize(int x,int y)
//{
//  // if the size changed send this on to the RenderWindow
//  if ((x != this->Size[0])||(y != this->Size[1]))
//    {
//    this->Size[0] = x;
//    this->Size[1] = y;
//    this->RenderWindow->SetSize(x,y);
//    }
//
//}
 
void vtkXRenderWindowInteractorTimer(XtPointer client_data,
				     XtIntervalId *vtkNotUsed(id))
{
  vtkXRenderWindowInteractor *me;
  me = (vtkXRenderWindowInteractor *)client_data;

  if (me->Enabled) 
    {
    me->InteractorStyle->OnTimer();
    }
}

int vtkXRenderWindowInteractor::CreateTimer(int vtkNotUsed(timertype)) 
{
  this->AddTimeOut(this->App, 10,
		   vtkXRenderWindowInteractorTimer,
		   (XtPointer)this);
  return 1;
}

int vtkXRenderWindowInteractor::DestroyTimer(void) 
{
  // timers automatically expire in X windows
  return 1;
}

void vtkXRenderWindowInteractorCallback(Widget vtkNotUsed(w),
					XtPointer client_data,
					XEvent *event,
					Boolean *vtkNotUsed(ctd))
{
  vtkXRenderWindowInteractor *me;
  int xp,yp;

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

    case MapNotify:
      {
      // only render if we are currently accepting events
      if (me->GetEnabled() && me->GetRenderWindow()->GetNeverRendered())
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
      if (((reinterpret_cast<XConfigureEvent *>(event))->width != me->Size[0]) ||
	  ((reinterpret_cast<XConfigureEvent *>(event))->height != me->Size[1]))
	{
	me->UpdateSize((reinterpret_cast<XConfigureEvent *>(event))->width,
		       (reinterpret_cast<XConfigureEvent *>(event))->height); 
	// only render if we are currently accepting events
        if (me->GetEnabled())
          {
	  me->InteractorStyle->OnConfigure((reinterpret_cast<XConfigureEvent *>(event))->width,
					   (reinterpret_cast<XConfigureEvent *>(event))->height);
          me->GetRenderWindow()->Render();
          }
	}
      }
      break;
            
    case ButtonPress: 
      {
      if (!me->Enabled) return;
      int ctrl = 0;
      if ((reinterpret_cast<XButtonEvent *>(event))->state & ControlMask)
        {
	ctrl = 1;
        }
      int shift = 0;
      if ((reinterpret_cast<XButtonEvent *>(event))->state & ShiftMask)
        {
	shift = 1;
        }
      xp = (reinterpret_cast<XButtonEvent *>(event))->x;
      yp = me->Size[1] - (reinterpret_cast<XButtonEvent *>(event))->y - 1;
      switch ((reinterpret_cast<XButtonEvent *>(event))->button)
	{
	case Button1: 
	  me->InteractorStyle->OnLeftButtonDown(ctrl, shift, xp, yp);
	  break;
	case Button2: 
	  me->InteractorStyle->OnMiddleButtonDown(ctrl, shift, xp, yp);
	  break;
	case Button3: 
	  me->InteractorStyle->OnRightButtonDown(ctrl, shift, xp, yp);
	  break;
	}
      }
      break;
      
    case ButtonRelease: 
      {
      if (!me->Enabled) return;
      int ctrl = 0;
      if ((reinterpret_cast<XButtonEvent *>(event))->state & ControlMask)
        {
	ctrl = 1;
        }
      int shift = 0;
      if ((reinterpret_cast<XButtonEvent *>(event))->state & ShiftMask)
        {
	shift = 1;
        }
      xp = (reinterpret_cast<XButtonEvent *>(event))->x;
      yp = me->Size[1] - (reinterpret_cast<XButtonEvent *>(event))->y - 1;
      switch ((reinterpret_cast<XButtonEvent *>(event))->button)
	{
	case Button1: 
	  me->InteractorStyle->OnLeftButtonUp(ctrl, shift, xp, yp);
	  break;
	case Button2: 
	  me->InteractorStyle->OnMiddleButtonUp(ctrl, shift, xp, yp);
	  break;
	case Button3: 
	  me->InteractorStyle->OnRightButtonUp(ctrl, shift, xp, yp);
	  break;
	}
      }
      break;

    case EnterNotify:
      {
      // Force the keyboard focus to be this render window
      if (me->TopLevelShell != NULL)
        {
        XtSetKeyboardFocus(me->TopLevelShell, me->top);
        }
      if (me->Enabled)
	{
	XEnterWindowEvent *e = reinterpret_cast<XEnterWindowEvent *>(event);

	me->InteractorStyle->OnEnter((e->state & ControlMask) != 0,
				     (e->state & ShiftMask) != 0,
				     e->x, me->Size[1] - e->y - 1);
	}
      }
      break;

    case LeaveNotify:
      {
      if (me->Enabled)
	{
	XLeaveWindowEvent *e = reinterpret_cast<XLeaveWindowEvent *>(event);

	me->InteractorStyle->OnLeave((e->state & ControlMask) != 0,
				     (e->state & ShiftMask) != 0,
				     e->x, me->Size[1] - e->y - 1);
	}
      }
      break;

    case KeyPress:
      {
      int ctrl = 0;
      if ((reinterpret_cast<XKeyEvent *>(event))->state & ControlMask)
        {
	ctrl = 1;
        }
      int shift = 0;
      if ((reinterpret_cast<XKeyEvent *>(event))->state & ShiftMask)
        {
	shift = 1;
        }
      KeySym ks;
      static char buffer[20];
      int n;
      buffer[0] = '\0';
      n = XLookupString(reinterpret_cast<XKeyEvent *>(event),buffer,20,&ks,NULL);
      xp = (reinterpret_cast<XKeyEvent *>(event))->x;
      yp = me->Size[1] - (reinterpret_cast<XKeyEvent *>(event))->y - 1;
      if (!me->Enabled) return;
      me->InteractorStyle->OnMouseMove(0,0,xp,yp);
      me->InteractorStyle->OnKeyPress(ctrl, shift, buffer[0], 
				      XKeysymToString(ks), 1);
      for (int i = 0; i < n && n > 0; i++)
	{
	me->InteractorStyle->OnChar(ctrl, shift, buffer[i], 1);
	}
      }
      break;      
      
    case KeyRelease:
      {
      int ctrl = 0;
      if ((reinterpret_cast<XKeyEvent *>(event))->state & ControlMask)
        {
	ctrl = 1;
        }
      int shift = 0;
      if ((reinterpret_cast<XKeyEvent *>(event))->state & ShiftMask)
        {
	shift = 1;
        }
      KeySym ks;
      static char buffer[20];
      buffer[0] = '\0';
      XLookupString(reinterpret_cast<XKeyEvent *>(event),buffer,20,&ks,NULL);
      xp = (reinterpret_cast<XKeyEvent *>(event))->x;
      yp = me->Size[1] - (reinterpret_cast<XKeyEvent *>(event))->y - 1;
      if (!me->Enabled) return;
      me->InteractorStyle->OnMouseMove(0,0,xp,yp);
      me->InteractorStyle->OnKeyRelease(ctrl, shift, buffer[0], 
					XKeysymToString(ks), 1);
      me->InteractorStyle->OnKeyUp(ctrl, shift, buffer[0], 1);
      }
      break;      
      
    case MotionNotify: 
      {
      if (!me->Enabled) return;
      int ctrl = 0;
      if ((reinterpret_cast<XMotionEvent *>(event))->state & ControlMask)
        {
	ctrl = 1;
        }
      int shift = 0;
      if ((reinterpret_cast<XMotionEvent *>(event))->state & ShiftMask)
        {
	shift = 1;
        }

      // Note that even though the (x,y) location of the pointer is event structure,
      // we must call XQueryPointer for the hints (motion event compression) to
      // work properly.
      me->GetMousePosition(&xp, &yp);

      me->InteractorStyle->OnMouseMove(ctrl, shift, xp, yp);
      }
      break;
    }
}


XtIntervalId vtkXRenderWindowInteractor::AddTimeOut(XtAppContext app_context,
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

  *y = this->Size[1] - *y - 1;
}

void vtkXRenderWindowInteractor::Timer(XtPointer client_data,
                                       XtIntervalId *id) 
{
  vtkXRenderWindowInteractorTimer(client_data, id);
}

void vtkXRenderWindowInteractor::Callback(Widget w,
					  XtPointer client_data,
					  XEvent *event,
					  Boolean *ctd) 
{
  vtkXRenderWindowInteractorCallback(w, client_data, event, ctd);
}


