/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/keysym.h>
#include "vtkXRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#include "vtkXOpenGLRenderWindow.h"
#include "vtkCallbackCommand.h"
#include "vtkActor.h"
#include <X11/Shell.h>
#include <math.h>
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkXRenderWindowInteractor, "1.121");
vtkStandardNewMacro(vtkXRenderWindowInteractor);

// Initialize static members:
int vtkXRenderWindowInteractor::NumAppInitialized = 0;
XtAppContext vtkXRenderWindowInteractor::App = 0;


typedef struct
{
  Visual        *visual;
  int   depth;
} OptionsRec;
OptionsRec      Options;

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
  this->Top = 0;
  this->OwnTop = 0;
  this->OwnApp = 0;
  this->TopLevelShell = NULL;
  this->BreakLoopFlag = 0;
  this->BreakXtLoopCallback = vtkCallbackCommand::New();
  this->BreakXtLoopCallback->SetClientData(this);
  this->BreakXtLoopCallback->SetCallback(
    &vtkXRenderWindowInteractor::BreakXtLoop);
}

vtkXRenderWindowInteractor::~vtkXRenderWindowInteractor()
{
  this->Disable();
  if(this->OwnTop)
    {
    XtDestroyWidget(this->Top);
    }
  this->BreakXtLoopCallback->Delete();
  if (vtkXRenderWindowInteractor::App)
    {
    if(vtkXRenderWindowInteractor::NumAppInitialized == 1)
      {
      if(this->OwnApp)
        {
        XtDestroyApplicationContext(vtkXRenderWindowInteractor::App);
        }
      vtkXRenderWindowInteractor::App = 0;
      }
    vtkXRenderWindowInteractor::NumAppInitialized--;
    }
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
  this->Top = foo;
  this->OwnTop = 0;
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
void vtkXRenderWindowInteractor::BreakXtLoop(vtkObject*, unsigned long,
                                             void* iren, void*)
{
  static_cast<vtkXRenderWindowInteractor*>(iren)->SetBreakLoopFlag(1);
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
  
  this->AddObserver(vtkCommand::ExitEvent, this->BreakXtLoopCallback);
  this->BreakLoopFlag = 0;
  do 
    {
    XEvent event;
    XtAppNextEvent(vtkXRenderWindowInteractor::App, &event);
    XtDispatchEvent(&event);
    } 
  while (this->BreakLoopFlag == 0);
  this->RemoveObserver(this->BreakXtLoopCallback);
}

// Initializes the event handlers using an XtAppContext that you have
// provided.  This assumes that you want to own the event loop.
void vtkXRenderWindowInteractor::Initialize(XtAppContext app)
{
  vtkXRenderWindowInteractor::App = app;

  this->Initialize();
}

// Initializes the event handlers without an XtAppContext.  This is
// good for when you don't have a user interface, but you still
// want to have mouse interaction.
void vtkXRenderWindowInteractor::Initialize()
{
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
  if (vtkXRenderWindowInteractor::App)
    {
    vtkXRenderWindowInteractor::NumAppInitialized++;
    }
  if (!vtkXRenderWindowInteractor::NumAppInitialized)
    {
    vtkDebugMacro( << "Toolkit init :" << vtkXRenderWindowInteractor::App );
    XtToolkitInitialize();
    vtkXRenderWindowInteractor::App = XtCreateApplicationContext();
    this->OwnApp = 1;
    vtkDebugMacro( << "App context :" << vtkXRenderWindowInteractor::App);
    vtkXRenderWindowInteractor::NumAppInitialized = 1;
    }
  
  this->DisplayId = ren->GetDisplayId();
  if (!this->DisplayId)
    {
    vtkDebugMacro("opening display");
    this->DisplayId = 
      XtOpenDisplay(vtkXRenderWindowInteractor::App,NULL,"VTK","vtk",NULL,0,&argc,NULL);
    vtkDebugMacro("opened display");
    }
  else
    {
    // if there is no parent widget
    if (!this->Top)
      {
      XtDisplayInitialize(vtkXRenderWindowInteractor::App,this->DisplayId,
                          "VTK","vtk",NULL,0,&argc,NULL);
      }
    }
  
  // get the info we need from the RenderingWindow
  ren->SetDisplayId(this->DisplayId);

  size    = ren->GetSize();
  size[0] = ((size[0] > 0) ? size[0] : 300);
  size[1] = ((size[1] > 0) ? size[1] : 300);
  if (!this->Top)
    {
    depth   = ren->GetDesiredDepth();
    cmap    = ren->GetDesiredColormap();
    vis     = ren->GetDesiredVisual();
    position= ren->GetPosition();

    this->Top = XtVaAppCreateShell(this->RenderWindow->GetWindowName(),"vtk",
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
    this->OwnTop = 1;
    XtRealizeWidget(this->Top);
    XSync(this->DisplayId,False);
    ren->SetWindowId(XtWindow(this->Top));
    
    XMapWindow(this->DisplayId, XtWindow(this->Top));
    XSync(this->DisplayId,False);
    }
  else
    {
    XWindowAttributes attribs;
    
    XtRealizeWidget(this->Top);
    XSync(this->DisplayId,False);
    ren->SetWindowId(XtWindow(this->Top));
    
    XMapWindow(this->DisplayId, XtWindow(this->Top));
    XSync(this->DisplayId,False);

    //  Find the current window size 
    XGetWindowAttributes(this->DisplayId, 
                         XtWindow(this->Top), &attribs);
    size[0] = attribs.width;
    size[1] = attribs.height;
    ren->SetSize(size[0], size[1]);
    }

  this->WindowId = XtWindow(this->Top);
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
  XtAddEventHandler(this->Top,
                    KeyPressMask | KeyReleaseMask | 
                    ButtonPressMask | ButtonReleaseMask |
                    ExposureMask | StructureNotifyMask | 
                    EnterWindowMask | LeaveWindowMask | 
                    PointerMotionHintMask | PointerMotionMask,
                    True,  // True means we also observe ClientMessage
                    vtkXRenderWindowInteractorCallback,(XtPointer)this);

   // Setup for capturing the window deletion
  this->KillAtom = XInternAtom(this->DisplayId,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(this->DisplayId,this->WindowId,&this->KillAtom,1);

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
  XtRemoveEventHandler(this->Top,
                    KeyPressMask | KeyReleaseMask | ButtonPressMask | 
                    ExposureMask | ButtonReleaseMask |
                    EnterWindowMask | LeaveWindowMask | 
                    PointerMotionHintMask | PointerMotionMask,
                    True,vtkXRenderWindowInteractorCallback,(XtPointer)this);

  this->Enabled = 0;
  this->Modified();
}


void vtkXRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (vtkXRenderWindowInteractor::App)
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
    me->InvokeEvent(vtkCommand::TimerEvent,NULL);
    }
}

int vtkXRenderWindowInteractor::CreateTimer(int vtkNotUsed(timertype)) 
{
  this->AddTimeOut(vtkXRenderWindowInteractor::App, 10,
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
  int xp, yp;

  me = (vtkXRenderWindowInteractor *)client_data;
  
  switch (event->type) 
    {
    case Expose:
      {
      if (!me->Enabled) 
       {
       return;
       }
      XEvent result;
      while (XCheckTypedWindowEvent(me->DisplayId, 
                                    me->WindowId,
                                    Expose, 
                                    &result))
        {
        // just getting the expose configure event
        event = &result;
        }
      int width = (reinterpret_cast<XConfigureEvent *>(event))->width;
      int height = (reinterpret_cast<XConfigureEvent *>(event))->height;
      me->SetEventSize(width, height);
      xp = (reinterpret_cast<XButtonEvent*>(event))->x;
      yp = (reinterpret_cast<XButtonEvent*>(event))->y;
      yp = me->Size[1] - xp - 1;
      me->SetEventPosition(xp, yp);
      // only render if we are currently accepting events
      if (me->Enabled)
        {
        me->InvokeEvent(vtkCommand::ExposeEvent,NULL);
        me->GetRenderWindow()->Render();
        }
      }
      break;

    case MapNotify:
      {
      // only render if we are currently accepting events
      if (me->Enabled && me->GetRenderWindow()->GetNeverRendered())
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
      int width = (reinterpret_cast<XConfigureEvent *>(event))->width;
      int height = (reinterpret_cast<XConfigureEvent *>(event))->height;
      if (width != me->Size[0] || height != me->Size[1])
        {
        me->UpdateSize(width, height);
        xp = (reinterpret_cast<XButtonEvent*>(event))->x;
        yp = (reinterpret_cast<XButtonEvent*>(event))->y;
        me->SetEventPosition(xp, me->Size[1] - yp - 1);
        // only render if we are currently accepting events
        if (me->Enabled)
          {
          me->InvokeEvent(vtkCommand::ConfigureEvent,NULL);
          me->GetRenderWindow()->Render();
          }
        }
      }
      break;
            
    case ButtonPress: 
      {
      if (!me->Enabled) 
        {
        return;
        }
      int ctrl = 
        (reinterpret_cast<XButtonEvent *>(event))->state & ControlMask ? 1 : 0;
      int shift =
        (reinterpret_cast<XButtonEvent *>(event))->state & ShiftMask ? 1 : 0;
      xp = (reinterpret_cast<XButtonEvent*>(event))->x;
      yp = (reinterpret_cast<XButtonEvent*>(event))->y;

      // check for double click
      static int MousePressTime = 0;
      int repeat = 0;
      // 400 ms threshold by default is probably good to start
      if((reinterpret_cast<XButtonEvent*>(event)->time - MousePressTime) < 400)
        {
        MousePressTime -= 2000;  // no double click next time
        repeat = 1;
        }
      else
        {
          MousePressTime = reinterpret_cast<XButtonEvent*>(event)->time;
        }

      me->SetEventInformationFlipY(xp, 
                                   yp,
                                   ctrl, 
                                   shift,
                                   0,
                                   repeat);
      switch ((reinterpret_cast<XButtonEvent *>(event))->button)
        {
        case Button1:  
          me->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
          break;
        case Button2: 
          me->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);
          break;
        case Button3: 
          me->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);
          break;
        case Button4: 
          me->InvokeEvent(vtkCommand::MouseWheelForwardEvent,NULL);
          break;
        case Button5: 
          me->InvokeEvent(vtkCommand::MouseWheelBackwardEvent,NULL);
          break;
        }
      }
      break;
      
    case ButtonRelease: 
      {
      if (!me->Enabled) 
        {
        return;
        }
      int ctrl = 
        (reinterpret_cast<XButtonEvent *>(event))->state & ControlMask ? 1 : 0;
      int shift =
        (reinterpret_cast<XButtonEvent *>(event))->state & ShiftMask ? 1 : 0;
      xp = (reinterpret_cast<XButtonEvent*>(event))->x;
      yp = (reinterpret_cast<XButtonEvent*>(event))->y; 
      me->SetEventInformationFlipY(xp, 
                                   yp,
                                   ctrl, 
                                   shift);
      switch ((reinterpret_cast<XButtonEvent *>(event))->button)
        {
        case Button1: 
          me->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
          break;
        case Button2: 
          me->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent,NULL);
          break;
        case Button3: 
          me->InvokeEvent(vtkCommand::RightButtonReleaseEvent,NULL);
          break;
        }
      }
      break;

    case EnterNotify:
      {
      // Force the keyboard focus to be this render window
      if (me->TopLevelShell != NULL)
        {
        XtSetKeyboardFocus(me->TopLevelShell, me->Top);
        }
      if (me->Enabled)
        {
        XEnterWindowEvent *e = reinterpret_cast<XEnterWindowEvent *>(event);
        me->SetEventInformationFlipY(e->x, 
                                     e->y,
                                     (e->state & ControlMask) != 0, 
                                     (e->state & ShiftMask) != 0);
        me->InvokeEvent(vtkCommand::EnterEvent, NULL);
        }
      }
      break;

    case LeaveNotify:
      {
      if (me->Enabled)
        {
        XLeaveWindowEvent *e = reinterpret_cast<XLeaveWindowEvent *>(event);
        me->SetEventInformationFlipY(e->x, 
                                     e->y,
                                     (e->state & ControlMask) != 0, 
                                     (e->state & ShiftMask) != 0); 
        me->InvokeEvent(vtkCommand::LeaveEvent, NULL);
        }
      }
      break;

    case KeyPress:
      {
      if (!me->Enabled) 
        {
        return;
        }
      int ctrl = 
        (reinterpret_cast<XButtonEvent *>(event))->state & ControlMask ? 1 : 0;
      int shift =
        (reinterpret_cast<XButtonEvent *>(event))->state & ShiftMask ? 1 : 0;
      KeySym ks;
      static char buffer[20];
      buffer[0] = '\0';
      XLookupString(reinterpret_cast<XKeyEvent *>(event),buffer, 20, &ks,NULL);
      xp = (reinterpret_cast<XKeyEvent*>(event))->x;
      yp = (reinterpret_cast<XKeyEvent*>(event))->y;
      me->SetEventInformationFlipY(xp, 
                                   yp,
                                   ctrl, 
                                   shift, 
                                   buffer[0], 
                                   1, 
                                   XKeysymToString(ks));
      me->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
      me->InvokeEvent(vtkCommand::CharEvent, NULL);
      }
      break;      
      
    case KeyRelease:
      {
      if (!me->Enabled) 
        {
        return;
        }
      int ctrl = 
        (reinterpret_cast<XButtonEvent *>(event))->state & ControlMask ? 1 : 0;
      int shift =
        (reinterpret_cast<XButtonEvent *>(event))->state & ShiftMask ? 1 : 0;
      KeySym ks;
      static char buffer[20];
      buffer[0] = '\0';
      XLookupString(reinterpret_cast<XKeyEvent *>(event),buffer, 20, &ks,NULL);
      xp = (reinterpret_cast<XKeyEvent *>(event))->x;
      yp = (reinterpret_cast<XKeyEvent *>(event))->y;
      me->SetEventInformationFlipY(xp, 
                                   yp,
                                   ctrl, 
                                   shift, 
                                   buffer[0], 
                                   1, 
                                   XKeysymToString(ks));
      me->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
      }
      break;      
      
    case MotionNotify: 
      {
      if (!me->Enabled) 
        {
        return;
        }
      int ctrl =
        (reinterpret_cast<XButtonEvent *>(event))->state & ControlMask ? 1 : 0;
      int shift =
        (reinterpret_cast<XButtonEvent *>(event))->state & ShiftMask ? 1 : 0;

      // Note that even though the (x,y) location of the pointer is event structure,
      // we must call XQueryPointer for the hints (motion event compression) to
      // work properly.
      me->GetMousePosition(&xp, &yp);
      me->SetEventInformation(xp, yp, ctrl, shift);
      me->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
      }
      break;

    case ClientMessage: 
      {
      if( static_cast<Atom>(event->xclient.data.l[0]) == me->KillAtom )
        {
        me->InvokeEvent(vtkCommand::ExitEvent, NULL);
        }
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


