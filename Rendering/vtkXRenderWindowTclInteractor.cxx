/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowTclInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOldStyleCallbackCommand.h"
#include "vtkPoints.h"
#include "vtkXOpenGLRenderWindow.h"
#include "vtkXRenderWindowTclInteractor.h"
#include <X11/Shell.h>
#include <X11/X.h>
#include <X11/keysym.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vtkTk.h>

vtkCxxRevisionMacro(vtkXRenderWindowTclInteractor, "1.50");
vtkStandardNewMacro(vtkXRenderWindowTclInteractor);

// steal the first three elements of the TkMainInfo stuct
// we don't care about the rest of the elements.
struct TkMainInfo
{
  int refCount;
  struct TkWindow *winPtr;
  Tcl_Interp *interp;
};

#if ((TK_MAJOR_VERSION <= 4)||((TK_MAJOR_VERSION == 8)&&(TK_MINOR_VERSION == 0)))
extern TkMainInfo *tkMainWindowList;
#else
extern "C" {TkMainInfo *TkGetMainInfoList();}
#endif

// returns 1 if done
static int vtkTclEventProc(XtPointer clientData,XEvent *event)
{
  Boolean ctd;
  vtkXOpenGLRenderWindow *rw;
      
  rw = (vtkXOpenGLRenderWindow *)
    (((vtkXRenderWindowTclInteractor *)clientData)->GetRenderWindow());
  
  if (rw->GetWindowId() == (reinterpret_cast<XAnyEvent *>(event))->window)
    {
    vtkXRenderWindowTclInteractorCallback((Widget)NULL,clientData, event, &ctd);
    ctd = 0;
    }
  else
    {
    ctd = 1;
    }

  return !ctd;
}

extern "C"
{
  void vtkXTclTimerProc(ClientData clientData)
  {
    XtIntervalId id;
    
    vtkXRenderWindowTclInteractorTimer((XtPointer)clientData,&id);
  }
}



// Construct object so that light follows camera motion.
vtkXRenderWindowTclInteractor::vtkXRenderWindowTclInteractor()
{
  this->App = 0;
  this->top = 0;
  this->TopLevelShell = NULL;
  this->BreakLoopFlag = 0;
}

vtkXRenderWindowTclInteractor::~vtkXRenderWindowTclInteractor()
{
  if (this->Initialized)
    {
    Tk_DeleteGenericHandler((Tk_GenericProc *)vtkTclEventProc,
                            (ClientData)this);
    }
}

void  vtkXRenderWindowTclInteractor::SetWidget(Widget foo)
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
//     vtkXRenderWindowTclInteractor's Widget has to be set to the vtkRenderWindow's
//           container widget
//     vtkXRenderWindowTclInteractor's TopLevel has to be set to the top level
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
void vtkXRenderWindowTclInteractor::SetTopLevelShell(Widget topLevel)
{
  this->TopLevelShell = topLevel;
}


static void vtkBreakTclLoop(void *iren)
{
  ((vtkXRenderWindowTclInteractor*)iren)->SetBreakLoopFlag(1);
}

void  vtkXRenderWindowTclInteractor::Start()
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(vtkCommand::StartEvent))
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    return;
    }

  vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
  cbc->Callback = vtkBreakTclLoop;
  cbc->ClientData = this;
  unsigned long ExitTag = this->AddObserver(vtkCommand::ExitEvent,cbc, 0.5);
  cbc->Delete();
  
  this->BreakLoopFlag = 0;
  while(this->BreakLoopFlag == 0)
    {
    Tk_DoOneEvent(0);
    }
  this->RemoveObserver(ExitTag);
}

// Initializes the event handlers
void vtkXRenderWindowTclInteractor::Initialize(XtAppContext app)
{
  this->App = app;

  this->Initialize();
}

// Begin processing keyboard strokes.
void vtkXRenderWindowTclInteractor::Initialize()
{
  vtkXOpenGLRenderWindow *ren;
  int *size;

  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
    {
    vtkErrorMacro(<<"No renderer defined!");
    return;
    }

  this->Initialized = 1;
  ren = (vtkXOpenGLRenderWindow *)(this->RenderWindow);

  // use the same display as tcl/tk
#if ((TK_MAJOR_VERSION <= 4)||((TK_MAJOR_VERSION == 8)&&(TK_MINOR_VERSION == 0)))
  ren->SetDisplayId(Tk_Display(tkMainWindowList->winPtr));
#else
  ren->SetDisplayId(Tk_Display(TkGetMainInfoList()->winPtr));
#endif
  this->DisplayId = ren->GetDisplayId();
  
  // get the info we need from the RenderingWindow
  size    = ren->GetSize();
  
  size = ren->GetSize();
  ren->Start();
  this->WindowId = ren->GetWindowId();
  size = ren->GetSize();

  this->Size[0] = size[0];
  this->Size[1] = size[1];

  this->Enable();

  // Set the event handler
  Tk_CreateGenericHandler((Tk_GenericProc *)vtkTclEventProc,(ClientData)this);
}


void vtkXRenderWindowTclInteractor::Enable()
{
  // avoid cycles of calling Initialize() and Enable()
  if (this->Enabled)
    {
    return;
    }

  // Select the events that we want to respond to
  // (Multiple calls to XSelectInput overrides the previous settings)
  XSelectInput(this->DisplayId, this->WindowId,
               KeyPressMask | KeyReleaseMask |
               ButtonPressMask | ButtonReleaseMask |
               ExposureMask | StructureNotifyMask |
               EnterWindowMask | LeaveWindowMask |
               PointerMotionMask | PointerMotionMask);

  // Setup for capturing the window deletion
  this->KillAtom = XInternAtom(this->DisplayId,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(this->DisplayId,this->WindowId,&this->KillAtom,1);

  this->Enabled = 1;

  this->Modified();
}

void vtkXRenderWindowTclInteractor::Disable()
{
  if (!this->Enabled)
    {
    return;
    }
  
  // Remove the all the events that we registered for EXCEPT for
  // StructureNotifyMask event since we need to keep track of the window
  // size (we will not render if we are disabled, we simply track the window
  // size changes for a possible Enable()). Expose events are disabled.
  // (Multiple calls to XSelectInput overrides the previous settings)
  XSelectInput(this->DisplayId,this->WindowId,
               StructureNotifyMask );

  this->Enabled = 0;
  this->Modified();
}


void vtkXRenderWindowTclInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (this->App)
    {
    os << indent << "App: " << this->App << "\n";
    }
  else
    {
    os << indent << "App: (none)\n";
    }
  os << indent << "Break Loop Flag: " 
     << (this->BreakLoopFlag ? "On\n" : "Off\n");
}


void  vtkXRenderWindowTclInteractor::UpdateSize(int x,int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0])||(y != this->Size[1]))
    {
    this->Size[0] = x;
    this->Size[1] = y;
    this->RenderWindow->SetSize(x,y);
    }

} 


void vtkXRenderWindowTclInteractorCallback(Widget vtkNotUsed(w),
                                        XtPointer client_data, 
                                        XEvent *event, 
                                        Boolean *vtkNotUsed(ctd))
{
  vtkXRenderWindowTclInteractor *me;
  
  me = (vtkXRenderWindowTclInteractor *)client_data;
  int xp, yp;
  
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
        me->Render();
        }
      }
      break;
 
    case MapNotify:
      {
      // only render if we are currently accepting events
      if (me->Enabled && me->GetRenderWindow()->GetNeverRendered())
        {
        me->Render();
        }
      }
      break;

    case ConfigureNotify: 
      {
      XEvent result;
      while (XCheckTypedWindowEvent(me->DisplayId, 
                                    me->WindowId,
                                    ConfigureNotify, 
                                    &result))
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
          me->Render();
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
      me->SetEventInformationFlipY(xp, 
                                   yp,
                                   ctrl, 
                                   shift);
      switch ((reinterpret_cast<XButtonEvent *>(event))->button)
        {
        case Button1: 
          me->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
          break;
        case Button2: 
          me->InvokeEvent(vtkCommand::MiddleButtonPressEvent, NULL);
          break;
        case Button3: 
          me->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
          break;
        case Button4: 
          me->InvokeEvent(vtkCommand::MouseWheelForwardEvent, NULL);
          break;
        case Button5: 
          me->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, NULL);
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
          me->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
          break;
        case Button2: 
          me->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
          break;
        case Button3: 
          me->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
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
      xp = (reinterpret_cast<XMotionEvent*>(event))->x;
      yp = (reinterpret_cast<XMotionEvent*>(event))->y;
      me->SetEventInformationFlipY(xp, 
                                   yp,
                                   ctrl, 
                                   shift);
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

void vtkXRenderWindowTclInteractorTimer(XtPointer client_data,
                                        XtIntervalId *vtkNotUsed(id))
{
  vtkXRenderWindowTclInteractor *me;
  me = (vtkXRenderWindowTclInteractor *)client_data;
  Window root,child;
  int root_x,root_y;
  int x,y;
  unsigned int keys;

  // get the pointer position
  XQueryPointer(me->DisplayId,
                me->WindowId,
                &root,
                &child,
                &root_x,
                &root_y,
                &x,
                &y,
                &keys);
  if (!me->Enabled) 
    {
    return;
    }
  me->SetEventInformationFlipY(x, 
                               y,
                               0, 
                               0);
  me->InvokeEvent(vtkCommand::TimerEvent, NULL);
}

int vtkXRenderWindowTclInteractor::CreateTimer(int vtkNotUsed(timertype)) 
{
  Tk_CreateTimerHandler(10,vtkXTclTimerProc,(ClientData)this);
  return 1;
}

int vtkXRenderWindowTclInteractor::DestroyTimer(void) 
{
  // timers automatically expire in X windows
  return 1;
}

void vtkXRenderWindowTclInteractor::TerminateApp(void) 
{
#if ((TK_MAJOR_VERSION <= 4)||((TK_MAJOR_VERSION == 8)&&(TK_MINOR_VERSION == 0)))
  Tcl_Interp* interp = tkMainWindowList->interp;
#else
  Tcl_Interp* interp = TkGetMainInfoList()->interp;
#endif

#if TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION <= 2
  char es[12];
  strcpy(es,"exit");
  Tcl_GlobalEval(interp, es);
#else
  Tcl_EvalEx(interp, "exit", -1, TCL_EVAL_GLOBAL);
#endif
}
