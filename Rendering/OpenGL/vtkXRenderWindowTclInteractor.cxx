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
#include "vtkXRenderWindowTclInteractor.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkXOpenGLRenderWindow.h"

#include <vtksys/stl/map>

#include "vtkTk.h"

//-------------------------------------------------------------------------
vtkStandardNewMacro(vtkXRenderWindowTclInteractor);


//-------------------------------------------------------------------------
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


//-------------------------------------------------------------------------
class vtkXTclTimer
{
public:
  vtkXTclTimer()
    {
    this->Interactor = 0;
    this->ID = 0;
    this->TimerToken = 0;
    }

  vtkRenderWindowInteractor *Interactor;
  int ID;
  Tcl_TimerToken TimerToken;
};

//-------------------------------------------------------------------------
extern "C" void vtkXTclTimerProc(ClientData clientData)
{
  vtkXTclTimer* timer = static_cast<vtkXTclTimer*>(clientData);

  vtkXRenderWindowTclInteractor* me =
    static_cast<vtkXRenderWindowTclInteractor*>(timer->Interactor);

  int platformTimerId = timer->ID;
  int timerId = me->GetVTKTimerId(platformTimerId);

  if (me->GetEnabled())
    {
    me->InvokeEvent(vtkCommand::TimerEvent, &timerId);
    }

  if (!me->IsOneShotTimer(timerId))
    {
    me->ResetTimer(timerId);
    }
}

//-------------------------------------------------------------------------
// Map between the Tcl native timer token to our own int id.  Note this
// is separate from the TimerMap in the vtkRenderWindowInteractor
// superclass.  This is used to avoid passing 64-bit values back
// through the "int" return type of InternalCreateTimer.
class vtkXRenderWindowTclInteractorInternals
{
public:
  vtkXTclTimer* CreateTimer(vtkRenderWindowInteractor* iren,
    int timerId, unsigned long duration)
    {
    vtkXTclTimer* timer = &this->Timers[timerId];

    timer->Interactor = iren;
    timer->ID = timerId;
    timer->TimerToken = Tcl_CreateTimerHandler(static_cast<int>(duration),
                                               vtkXTclTimerProc,
                                               static_cast<ClientData>(timer));

    return timer;
    }

  int DestroyTimer(int timerId)
    {
    int destroyed = 0;

    vtkXTclTimer* timer = &this->Timers[timerId];

    if (0 != timer->ID)
      {
      Tcl_DeleteTimerHandler(timer->TimerToken);

      timer->Interactor = 0;
      timer->ID = 0;
      timer->TimerToken = 0;

      destroyed = 1;
      }

    this->Timers.erase(timerId);

    return destroyed;
    }

private:
  std::map<int, vtkXTclTimer> Timers;
};


//-------------------------------------------------------------------------
extern "C" int vtkTclEventProc(XtPointer clientData, XEvent *event)
{
  Boolean ctd;
  vtkXOpenGLRenderWindow *rw;

  rw = static_cast<vtkXOpenGLRenderWindow *>
    (static_cast<vtkXRenderWindowTclInteractor *>(
       clientData)->GetRenderWindow());

  if (rw->GetWindowId() == (reinterpret_cast<XAnyEvent *>(event))->window)
    {
    vtkXRenderWindowInteractorCallback(static_cast<Widget>(NULL), clientData,
                                       event, &ctd);
    ctd = 0;
    }
  else
    {
    ctd = 1;
    }

  return !ctd;
}


//-------------------------------------------------------------------------
vtkXRenderWindowTclInteractor::vtkXRenderWindowTclInteractor()
{
  this->Internal = new vtkXRenderWindowTclInteractorInternals;
}


//-------------------------------------------------------------------------
vtkXRenderWindowTclInteractor::~vtkXRenderWindowTclInteractor()
{
  if (this->Initialized)
    {
    Tk_DeleteGenericHandler(static_cast<Tk_GenericProc *>(vtkTclEventProc),
                            static_cast<ClientData>(this));
    }

  delete this->Internal;
  this->Internal = 0;
}


//-------------------------------------------------------------------------
void vtkXRenderWindowTclInteractor::Initialize()
{
  if (this->Initialized)
    {
    return;
    }

  // make sure we have a RenderWindow
  if (!this->RenderWindow)
    {
    vtkErrorMacro(<<"No RenderWindow defined!");
    return;
    }

  this->Initialized = 1;

  vtkXOpenGLRenderWindow* ren =
    static_cast<vtkXOpenGLRenderWindow *>(this->RenderWindow);

  // Use the same display as tcl/tk:
  //
#if ((TK_MAJOR_VERSION <= 4)||((TK_MAJOR_VERSION == 8)&&(TK_MINOR_VERSION == 0)))
  ren->SetDisplayId(Tk_Display(tkMainWindowList->winPtr));
#else
  ren->SetDisplayId(Tk_Display(TkGetMainInfoList()->winPtr));
#endif

  this->DisplayId = ren->GetDisplayId();

  // Create a Tcl/Tk event handler:
  //
  Tk_CreateGenericHandler(static_cast<Tk_GenericProc *>(vtkTclEventProc),
                          static_cast<ClientData>(this));

  ren->Start();
  this->WindowId = ren->GetWindowId();
  int* size = ren->GetSize();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
  this->Enable();
}


//-------------------------------------------------------------------------
void vtkXRenderWindowTclInteractor::Initialize(XtAppContext app)
{
  this->Superclass::Initialize(app);
}


//-------------------------------------------------------------------------
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


//-------------------------------------------------------------------------
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


//-------------------------------------------------------------------------
void vtkXRenderWindowTclInteractor::Start()
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(vtkCommand::StartEvent) && !this->HandleEventLoop)
    {
    this->InvokeEvent(vtkCommand::StartEvent, NULL);
    return;
    }

  if (!this->Initialized)
    {
    this->Initialize();
    }
  if (!this->Initialized)
    {
    return;
    }

  this->BreakLoopFlag = 0;
  do
    {
    Tk_DoOneEvent(0);
    }
  while (this->BreakLoopFlag == 0);
}

//-------------------------------------------------------------------------
int vtkXRenderWindowTclInteractor::InternalCreateTimer(int timerId,
                                                       int vtkNotUsed(timerType),
                                                       unsigned long duration)
{
  duration = (duration > 0 ? duration : this->TimerDuration);
  vtkXTclTimer* timer = this->Internal->CreateTimer(this, timerId, duration);
  return timer->ID;
}


//-------------------------------------------------------------------------
int vtkXRenderWindowTclInteractor::InternalDestroyTimer(int platformTimerId)
{
  return this->Internal->DestroyTimer(platformTimerId);
}


//-------------------------------------------------------------------------
void vtkXRenderWindowTclInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
