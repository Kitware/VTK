/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowTclInteractor.cxx
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
#include "vtkXRenderWindowTclInteractor.h"
#include "vtkInteractorStyle.h"
#include "vtkXOpenGLRenderWindow.h"
#include "vtkActor.h"
#include <X11/Shell.h>
#include <math.h>
#include "tk.h"
#include "vtkActorCollection.h"
#include "vtkPoints.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkXRenderWindowTclInteractor* vtkXRenderWindowTclInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXRenderWindowTclInteractor");
  if(ret)
    {
    return (vtkXRenderWindowTclInteractor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXRenderWindowTclInteractor;
}




// steal the first two elements of the TkMainInfo stuct
// we don't care about the rest of the elements.
struct TkMainInfo
{
  int refCount;
  struct TkWindow *winPtr;
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

static void vtkXTclTimerProc(ClientData clientData)
{
  XtIntervalId id;
  
  vtkXRenderWindowTclInteractorTimer((XtPointer)clientData,&id);
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

  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = vtkBreakTclLoop;
  cbc->ClientData = this;
  this->RemoveObserver(this->ExitTag);

  this->ExitTag = this->AddObserver(vtkCommand::ExitEvent,cbc);
  this->BreakLoopFlag = 0;
  while(this->BreakLoopFlag == 0)
    {
    Tk_DoOneEvent(0);
    }
  this->RemoveObserver(this->ExitTag);
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
  XSelectInput(this->DisplayId,this->WindowId,
	       KeyPressMask | ButtonPressMask | ExposureMask |
	       StructureNotifyMask | ButtonReleaseMask | EnterWindowMask |
               PointerMotionMask);

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
  vtkRenderWindowInteractor::PrintSelf(os,indent);
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
  int xp,yp;
  
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
      if (((reinterpret_cast<XConfigureEvent *>(event))->width != me->Size[0]) ||
	  ((reinterpret_cast<XConfigureEvent *>(event))->height != me->Size[1]))
	{
	me->UpdateSize((reinterpret_cast<XConfigureEvent *>(event))->width,
		       (reinterpret_cast<XConfigureEvent *>(event))->height); 
	
	// only render if we are currently accepting events
	if (me->GetEnabled())
	  {
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
      xp = (reinterpret_cast<XButtonEvent*>(event))->x;
      yp = me->Size[1] - (reinterpret_cast<XButtonEvent*>(event))->y - 1;
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
      xp = (reinterpret_cast<XButtonEvent*>(event))->x;
      yp = me->Size[1] - (reinterpret_cast<XButtonEvent*>(event))->y - 1;
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
      buffer[0] = '\0';
      XLookupString(reinterpret_cast<XKeyEvent *>(event),buffer,20,&ks,NULL);
      xp = (reinterpret_cast<XKeyEvent*>(event))->x;
      yp = me->Size[1] - (reinterpret_cast<XKeyEvent*>(event))->y - 1;
      if (!me->Enabled) return;
      me->InteractorStyle->OnMouseMove(0,0,xp,yp);
      me->InteractorStyle->OnChar(ctrl, shift, buffer[0], 1);
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
      xp = (reinterpret_cast<XMotionEvent*>(event))->x;
      yp = me->Size[1] - (reinterpret_cast<XMotionEvent*>(event))->y - 1;
      me->InteractorStyle->OnMouseMove(ctrl, shift, xp, yp);
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
  XQueryPointer(me->DisplayId,me->WindowId,
		&root,&child,&root_x,&root_y,&x,&y,&keys);
  if (!me->Enabled) return;
  me->InteractorStyle->OnMouseMove(0,0,x,me->Size[1] - y);
  me->InteractorStyle->OnTimer();
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
#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION > 1
  Tcl_Finalize();
#else
  Tcl_Exit(1);
#endif
}
