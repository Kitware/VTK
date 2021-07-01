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
#include "vtkXRenderWindowInteractor.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkInteractorStyle.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <climits>
#include <cmath>
#include <map>
#include <sstream>

vtkStandardNewMacro(vtkXRenderWindowInteractor);

template <int EventType>
int XEventTypeEquals(Display*, XEvent* event, XPointer)
{
  return event->type == EventType;
}

struct vtkXRenderWindowInteractorTimer
{
  unsigned long duration;
  timeval lastFire;
};

// Map between the X native id to our own integer count id.  Note this
// is separate from the TimerMap in the vtkRenderWindowInteractor
// superclass.  This is used to avoid passing 64-bit values back
// through the "int" return type of InternalCreateTimer.
class vtkXRenderWindowInteractorInternals
{
public:
  vtkXRenderWindowInteractorInternals() { this->TimerIdCount = 1; }
  ~vtkXRenderWindowInteractorInternals() {}

  // duration is in milliseconds
  int CreateLocalTimer(unsigned long duration)
  {
    int id = this->TimerIdCount++;
    this->LocalToTimer[id].duration = duration;
    gettimeofday(&this->LocalToTimer[id].lastFire, nullptr);
    return id;
  }

  void DestroyLocalTimer(int id) { this->LocalToTimer.erase(id); }

  void GetTimeToNextTimer(timeval& tv)
  {
    uint64_t lowestDelta = 1000000;
    if (this->LocalToTimer.size())
    {
      timeval ctv;
      gettimeofday(&ctv, nullptr);
      for (auto& timer : this->LocalToTimer)
      {
        uint64_t delta = (ctv.tv_sec - timer.second.lastFire.tv_sec) * 1000000 + ctv.tv_usec -
          timer.second.lastFire.tv_usec;
        if (delta < lowestDelta)
        {
          lowestDelta = delta;
        }
      }
    }
    tv.tv_sec = lowestDelta / 1000000;
    tv.tv_usec = lowestDelta % 1000000;
  }

  void FireTimers(vtkXRenderWindowInteractor* rwi)
  {
    if (this->LocalToTimer.size())
    {
      timeval ctv;
      gettimeofday(&ctv, nullptr);
      std::vector<unsigned long> expired;
      for (auto& timer : this->LocalToTimer)
      {
        int64_t delta = (ctv.tv_sec - timer.second.lastFire.tv_sec) * 1000000 + ctv.tv_usec -
          timer.second.lastFire.tv_usec;
        if (delta / 1000 >= static_cast<int64_t>(timer.second.duration))
        {
          int timerId = rwi->GetVTKTimerId(timer.first);
          rwi->InvokeEvent(vtkCommand::TimerEvent, &timerId);
          if (rwi->IsOneShotTimer(timerId))
          {
            expired.push_back(timer.first);
          }
          else
          {
            timer.second.lastFire.tv_sec = ctv.tv_sec;
            timer.second.lastFire.tv_usec = ctv.tv_usec;
          }
        }
      }
      for (auto exp : expired)
      {
        this->DestroyLocalTimer(exp);
      }
    }
  }

private:
  int TimerIdCount;
  std::map<int, vtkXRenderWindowInteractorTimer> LocalToTimer;
};

int vtkXRenderWindowInteractor::BreakLoopFlag = 1;

// for some reason the X11 def of KeySym is getting messed up
typedef XID vtkKeySym;

//-------------------------------------------------------------------------
vtkXRenderWindowInteractor::vtkXRenderWindowInteractor()
{
  this->Internal = new vtkXRenderWindowInteractorInternals;
  this->DisplayId = nullptr;
  this->WindowId = 0;
  this->KillAtom = 0;
  this->XdndSource = 0;
  this->XdndPositionAtom = 0;
  this->XdndDropAtom = 0;
  this->XdndActionCopyAtom = 0;
  this->XdndStatusAtom = 0;
  this->XdndFinishedAtom = 0;
}

//-------------------------------------------------------------------------
vtkXRenderWindowInteractor::~vtkXRenderWindowInteractor()
{
  this->Disable();

  delete this->Internal;
}

//-------------------------------------------------------------------------
// TerminateApp() notifies the event loop to exit.
// The event loop is started by Start() or by one own's method.
// This results in Start() returning to its caller.
void vtkXRenderWindowInteractor::TerminateApp()
{
  if (this->BreakLoopFlag)
  {
    return;
  }

  this->BreakLoopFlag = 1;

  // Send a VTK_BreakXtLoop ClientMessage event to be sure we pop out of the
  // event loop.  This "wakes up" the event loop.  Otherwise, it might sit idle
  // waiting for an event before realizing an exit was requested.
  XClientMessageEvent client;
  memset(&client, 0, sizeof(client));

  client.type = ClientMessage;
  // client.serial; //leave zeroed
  // client.send_event; //leave zeroed
  client.display = this->DisplayId;
  client.window = this->WindowId;
  client.message_type = XInternAtom(this->DisplayId, "VTK_BreakXtLoop", False);
  client.format = 32; // indicates size of data chunks: 8, 16 or 32 bits...
  // client.data; //leave zeroed

  XSendEvent(client.display, client.window, True, NoEventMask, reinterpret_cast<XEvent*>(&client));
  XFlush(client.display);
}

void vtkXRenderWindowInteractor::SetBreakLoopFlag(int f)
{
  if (f)
  {
    this->BreakLoopFlagOn();
  }
  else
  {
    this->BreakLoopFlagOff();
  }
}

void vtkXRenderWindowInteractor::BreakLoopFlagOff()
{
  this->BreakLoopFlag = 0;
  this->Modified();
}

void vtkXRenderWindowInteractor::BreakLoopFlagOn()
{
  this->TerminateApp();
  this->Modified();
}

void vtkXRenderWindowInteractor::ProcessEvents()
{
  XEvent event;
  while (XPending(this->DisplayId) && this->BreakLoopFlag == 0)
  {
    XNextEvent(this->DisplayId, &event);
    this->DispatchEvent(&event);
  }
}

//-------------------------------------------------------------------------
// This will start up the X event loop. If you
// call this method it will loop processing X events until the
// loop is exited.
void vtkXRenderWindowInteractor::StartEventLoop()
{
  int X11fd = ConnectionNumber(this->DisplayId);
  fd_set in_fds;
  struct timeval tv;

  this->BreakLoopFlag = 0;
  do
  {
    if (XPending(this->DisplayId) == 0)
    {
      // get how long to wait for the next timer
      this->Internal->GetTimeToNextTimer(tv);
      // select will wait until 'tv' elapses or something else wakes us
      FD_ZERO(&in_fds);
      FD_SET(X11fd, &in_fds);
      select(X11fd + 1, &in_fds, nullptr, nullptr, &tv);
    }
    else
    {
      XEvent event;
      XNextEvent(this->DisplayId, &event);
      this->DispatchEvent(&event);
    }
    this->FireTimers();
  } while (this->BreakLoopFlag == 0);
}

//-------------------------------------------------------------------------
// Initializes the event handlers without an XtAppContext.  This is
// good for when you don't have a user interface, but you still
// want to have mouse interaction.
void vtkXRenderWindowInteractor::Initialize()
{
  if (this->Initialized)
  {
    return;
  }

  vtkRenderWindow* ren;
  int* size;

  // make sure we have a RenderWindow and camera
  if (!this->RenderWindow)
  {
    vtkErrorMacro(<< "No renderer defined!");
    return;
  }

  this->Initialized = 1;
  ren = this->RenderWindow;

  this->DisplayId = static_cast<Display*>(ren->GetGenericDisplayId());
  if (!this->DisplayId)
  {
    vtkDebugMacro("opening display");
    this->DisplayId = XOpenDisplay(nullptr);
    vtkDebugMacro("opened display");
    ren->SetDisplayId(this->DisplayId);
  }

  size = ren->GetActualSize();
  size[0] = ((size[0] > 0) ? size[0] : 300);
  size[1] = ((size[1] > 0) ? size[1] : 300);
  XSync(this->DisplayId, False);

  ren->Start();
  ren->End();

  this->WindowId = reinterpret_cast<Window>(ren->GetGenericWindowId());

  XWindowAttributes attribs;
  //  Find the current window size
  XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);

  size[0] = attribs.width;
  size[1] = attribs.height;
  ren->SetSize(size[0], size[1]);

  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//-------------------------------------------------------------------------
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
  XSelectInput(this->DisplayId, this->WindowId,
    KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ExposureMask |
      StructureNotifyMask | EnterWindowMask | LeaveWindowMask | PointerMotionHintMask |
      PointerMotionMask);

  // Setup for capturing the window deletion
  this->KillAtom = XInternAtom(this->DisplayId, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(this->DisplayId, this->WindowId, &this->KillAtom, 1);

  // Enable drag and drop
  Atom xdndAwareAtom = XInternAtom(this->DisplayId, "XdndAware", False);
  char xdndVersion = 5;
  XChangeProperty(this->DisplayId, this->WindowId, xdndAwareAtom, XA_ATOM, 32, PropModeReplace,
    (unsigned char*)&xdndVersion, 1);
  this->XdndPositionAtom = XInternAtom(this->DisplayId, "XdndPosition", False);
  this->XdndDropAtom = XInternAtom(this->DisplayId, "XdndDrop", False);
  this->XdndActionCopyAtom = XInternAtom(this->DisplayId, "XdndActionCopy", False);
  this->XdndStatusAtom = XInternAtom(this->DisplayId, "XdndStatus", False);
  this->XdndFinishedAtom = XInternAtom(this->DisplayId, "XdndFinished", False);

  this->Enabled = 1;

  this->Modified();
}

//-------------------------------------------------------------------------
void vtkXRenderWindowInteractor::Disable()
{
  if (!this->Enabled)
  {
    return;
  }

  this->Enabled = 0;

  this->Modified();
}

//-------------------------------------------------------------------------
void vtkXRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "BreakLoopFlag: " << (this->BreakLoopFlag ? "On\n" : "Off\n");
}

//-------------------------------------------------------------------------
void vtkXRenderWindowInteractor::UpdateSize(int x, int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0]) || (y != this->Size[1]))
  {
    this->Size[0] = x;
    this->Size[1] = y;
    this->RenderWindow->SetSize(x, y);
  }
}

//-------------------------------------------------------------------------
void vtkXRenderWindowInteractor::UpdateSizeNoXResize(int x, int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0]) || (y != this->Size[1]))
  {
    this->Size[0] = x;
    this->Size[1] = y;
    // static_cast<vtkXOpenGLRenderWindow*>(this->RenderWindow)->SetSizeNoXResize(x, y);
    this->RenderWindow->SetSize(x, y);
  }
}

//-------------------------------------------------------------------------
void vtkXRenderWindowInteractor::FireTimers()
{
  if (this->GetEnabled())
  {
    this->Internal->FireTimers(this);
  }
}

//-------------------------------------------------------------------------
// X always creates one shot timers
int vtkXRenderWindowInteractor::InternalCreateTimer(
  int vtkNotUsed(timerId), int vtkNotUsed(timerType), unsigned long duration)
{
  duration = (duration > 0 ? duration : this->TimerDuration);
  int platformTimerId = this->Internal->CreateLocalTimer(duration);
  return platformTimerId;
}

//-------------------------------------------------------------------------
int vtkXRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  this->Internal->DestroyLocalTimer(platformTimerId);
  return 1;
}

//-------------------------------------------------------------------------
void vtkXRenderWindowInteractor::DispatchEvent(XEvent* event)
{
  int xp, yp;

  switch (event->type)
  {
    case Expose:
    {
      if (!this->Enabled)
      {
        return;
      }
      XEvent result;
      while (XCheckTypedWindowEvent(this->DisplayId, this->WindowId, Expose, &result))
      {
        // just getting the expose configure event
        event = &result;
      }
      XExposeEvent* exposeEvent = reinterpret_cast<XExposeEvent*>(event);
      this->SetEventSize(exposeEvent->width, exposeEvent->height);
      xp = exposeEvent->x;
      yp = exposeEvent->y;
      yp = this->Size[1] - yp - 1;
      this->SetEventPosition(xp, yp);

      // only render if we are currently accepting events
      if (this->Enabled)
      {
        this->InvokeEvent(vtkCommand::ExposeEvent, nullptr);
        this->Render();
      }
    }
    break;

    case MapNotify:
    {
      // only render if we are currently accepting events
      if (this->Enabled && this->GetRenderWindow()->GetNeverRendered())
      {
        this->Render();
      }
    }
    break;

    case ConfigureNotify:
    {
      XEvent result;
      while (XCheckTypedWindowEvent(this->DisplayId, this->WindowId, ConfigureNotify, &result))
      {
        // just getting the last configure event
        event = &result;
      }
      int width = (reinterpret_cast<XConfigureEvent*>(event))->width;
      int height = (reinterpret_cast<XConfigureEvent*>(event))->height;
      if (width != this->Size[0] || height != this->Size[1])
      {
        bool resizeSmaller = width <= this->Size[0] && height <= this->Size[1];
        this->UpdateSizeNoXResize(width, height);
        xp = (reinterpret_cast<XButtonEvent*>(event))->x;
        yp = (reinterpret_cast<XButtonEvent*>(event))->y;
        this->SetEventPosition(xp, this->Size[1] - yp - 1);
        // only render if we are currently accepting events
        if (this->Enabled)
        {
          this->InvokeEvent(vtkCommand::ConfigureEvent, nullptr);
          if (resizeSmaller)
          {
            // Don't call Render when the window is resized to be larger:
            //
            // - if the window is resized to be larger, an Expose event will
            // be triggered by the X server which will trigger a call to
            // Render().
            // - if the window is resized to be smaller, no Expose event will
            // be triggered by the X server, as no new area become visible.
            // only in this case, we need to explicitly call Render()
            // in ConfigureNotify.
            this->Render();
          }
        }
      }
    }
    break;

    case ButtonPress:
    {
      if (!this->Enabled)
      {
        return;
      }
      int ctrl = ((reinterpret_cast<XButtonEvent*>(event))->state & ControlMask) ? 1 : 0;
      int shift = ((reinterpret_cast<XButtonEvent*>(event))->state & ShiftMask) ? 1 : 0;
      int alt = ((reinterpret_cast<XButtonEvent*>(event))->state & Mod1Mask) ? 1 : 0;
      xp = (reinterpret_cast<XButtonEvent*>(event))->x;
      yp = (reinterpret_cast<XButtonEvent*>(event))->y;

      // check for double click
      static int MousePressTime = 0;
      int repeat = 0;
      // 400 ms threshold by default is probably good to start
      int eventTime = static_cast<int>(reinterpret_cast<XButtonEvent*>(event)->time);
      if ((eventTime - MousePressTime) < 400)
      {
        MousePressTime -= 2000; // no double click next time
        repeat = 1;
      }
      else
      {
        MousePressTime = eventTime;
      }

      this->SetEventInformationFlipY(xp, yp, ctrl, shift, 0, repeat);
      this->SetAltKey(alt);
      switch ((reinterpret_cast<XButtonEvent*>(event))->button)
      {
        case Button1:
          this->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
          break;
        case Button2:
          this->InvokeEvent(vtkCommand::MiddleButtonPressEvent, nullptr);
          break;
        case Button3:
          this->InvokeEvent(vtkCommand::RightButtonPressEvent, nullptr);
          break;
        case Button4:
          this->InvokeEvent(vtkCommand::MouseWheelForwardEvent, nullptr);
          break;
        case Button5:
          this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, nullptr);
          break;
      }
    }
    break;

    case ButtonRelease:
    {
      if (!this->Enabled)
      {
        return;
      }
      int ctrl = ((reinterpret_cast<XButtonEvent*>(event))->state & ControlMask) ? 1 : 0;
      int shift = ((reinterpret_cast<XButtonEvent*>(event))->state & ShiftMask) ? 1 : 0;
      int alt = ((reinterpret_cast<XButtonEvent*>(event))->state & Mod1Mask) ? 1 : 0;
      xp = (reinterpret_cast<XButtonEvent*>(event))->x;
      yp = (reinterpret_cast<XButtonEvent*>(event))->y;
      this->SetEventInformationFlipY(xp, yp, ctrl, shift);
      this->SetAltKey(alt);
      switch ((reinterpret_cast<XButtonEvent*>(event))->button)
      {
        case Button1:
          this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
          break;
        case Button2:
          this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, nullptr);
          break;
        case Button3:
          this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, nullptr);
          break;
      }
    }
    break;

    case EnterNotify:
    {
      // Force the keyboard focus to be this render window
      XSetInputFocus(this->DisplayId, this->WindowId, RevertToPointerRoot, CurrentTime);
      if (this->Enabled)
      {
        XEnterWindowEvent* e = reinterpret_cast<XEnterWindowEvent*>(event);
        this->SetEventInformationFlipY(
          e->x, e->y, (e->state & ControlMask) != 0, (e->state & ShiftMask) != 0);
        this->SetAltKey(((reinterpret_cast<XButtonEvent*>(event))->state & Mod1Mask) ? 1 : 0);
        this->InvokeEvent(vtkCommand::EnterEvent, nullptr);
      }
    }
    break;

    case LeaveNotify:
    {
      if (this->Enabled)
      {
        XLeaveWindowEvent* e = reinterpret_cast<XLeaveWindowEvent*>(event);
        this->SetEventInformationFlipY(
          e->x, e->y, (e->state & ControlMask) != 0, (e->state & ShiftMask) != 0);
        this->SetAltKey(((reinterpret_cast<XButtonEvent*>(event))->state & Mod1Mask) ? 1 : 0);
        this->InvokeEvent(vtkCommand::LeaveEvent, nullptr);
      }
    }
    break;

    case KeyPress:
    {
      if (!this->Enabled)
      {
        return;
      }
      int ctrl = ((reinterpret_cast<XButtonEvent*>(event))->state & ControlMask) ? 1 : 0;
      int shift = ((reinterpret_cast<XButtonEvent*>(event))->state & ShiftMask) ? 1 : 0;
      int alt = ((reinterpret_cast<XButtonEvent*>(event))->state & Mod1Mask) ? 1 : 0;
      vtkKeySym ks;
      static char buffer[20];
      buffer[0] = '\0';
      XLookupString(reinterpret_cast<XKeyEvent*>(event), buffer, 20, &ks, nullptr);
      xp = (reinterpret_cast<XKeyEvent*>(event))->x;
      yp = (reinterpret_cast<XKeyEvent*>(event))->y;
      this->SetEventInformationFlipY(xp, yp, ctrl, shift, buffer[0], 1, XKeysymToString(ks));
      this->SetAltKey(alt);
      this->InvokeEvent(vtkCommand::KeyPressEvent, nullptr);
      this->InvokeEvent(vtkCommand::CharEvent, nullptr);
    }
    break;

    case KeyRelease:
    {
      if (!this->Enabled)
      {
        return;
      }
      int ctrl = ((reinterpret_cast<XButtonEvent*>(event))->state & ControlMask) ? 1 : 0;
      int shift = ((reinterpret_cast<XButtonEvent*>(event))->state & ShiftMask) ? 1 : 0;
      int alt = ((reinterpret_cast<XButtonEvent*>(event))->state & Mod1Mask) ? 1 : 0;
      vtkKeySym ks;
      static char buffer[20];
      buffer[0] = '\0';
      XLookupString(reinterpret_cast<XKeyEvent*>(event), buffer, 20, &ks, nullptr);
      xp = (reinterpret_cast<XKeyEvent*>(event))->x;
      yp = (reinterpret_cast<XKeyEvent*>(event))->y;
      this->SetEventInformationFlipY(xp, yp, ctrl, shift, buffer[0], 1, XKeysymToString(ks));
      this->SetAltKey(alt);
      this->InvokeEvent(vtkCommand::KeyReleaseEvent, nullptr);
    }
    break;

    case MotionNotify:
    {
      if (!this->Enabled)
      {
        return;
      }
      int ctrl = ((reinterpret_cast<XButtonEvent*>(event))->state & ControlMask) ? 1 : 0;
      int shift = ((reinterpret_cast<XButtonEvent*>(event))->state & ShiftMask) ? 1 : 0;
      int alt = ((reinterpret_cast<XButtonEvent*>(event))->state & Mod1Mask) ? 1 : 0;

      // Note that even though the (x,y) location of the pointer is event structure,
      // we must call XQueryPointer for the hints (motion event compression) to
      // work properly.
      this->GetMousePosition(&xp, &yp);
      this->SetEventInformation(xp, yp, ctrl, shift);
      this->SetAltKey(alt);
      this->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
    }
    break;

    // Selection request for drag and drop has been delivered
    case SelectionNotify:
    {
      // Sanity checks
      if (!event->xselection.property || !this->XdndSource)
      {
        return;
      }

      // Recover the dropped file
      char* data = nullptr;
      Atom actualType;
      int actualFormat;
      unsigned long itemCount, bytesAfter;
      XGetWindowProperty(this->DisplayId, event->xselection.requestor, event->xselection.property,
        0, LONG_MAX, False, event->xselection.target, &actualType, &actualFormat, &itemCount,
        &bytesAfter, (unsigned char**)&data);

      // Conversion checks
      if ((event->xselection.target != AnyPropertyType && actualType != event->xselection.target) ||
        itemCount == 0)
      {
        return;
      }

      // Recover filepaths from uris and invoke DropFilesEvent
      std::stringstream uris(data);
      std::string uri, protocol, hostname, filePath;
      std::string unused0, unused1, unused2, unused3;
      vtkNew<vtkStringArray> filePaths;
      while (std::getline(uris, uri, '\n'))
      {
        if (vtksys::SystemTools::ParseURL(
              uri, protocol, unused0, unused1, hostname, unused3, filePath, true))
        {
          if (protocol == "file" && (hostname.empty() || hostname == "localhost"))
          {
            // The uris can be crlf delimited, remove ending \r if any
            if (filePath.back() == '\r')
            {
              filePath.pop_back();
            }

            // The extracted filepath miss the first slash
            filePath.insert(0, "/");

            filePaths->InsertNextValue(filePath);
          }
        }
      }
      this->InvokeEvent(vtkCommand::DropFilesEvent, filePaths);
      XFree(data);

      // Inform the source the the drag and drop operation was sucessfull
      XEvent reply;
      memset(&reply, 0, sizeof(reply));

      reply.type = ClientMessage;
      reply.xclient.window = event->xclient.data.l[0];
      reply.xclient.message_type = this->XdndFinishedAtom;
      reply.xclient.format = 32;
      reply.xclient.data.l[0] = this->WindowId;
      reply.xclient.data.l[1] = itemCount;
      reply.xclient.data.l[2] = this->XdndActionCopyAtom;

      XSendEvent(this->DisplayId, this->XdndSource, False, NoEventMask, &reply);
      XFlush(this->DisplayId);
      this->XdndSource = 0;
    }
    break;

    case ClientMessage:
    {
      if (event->xclient.message_type == this->XdndPositionAtom)
      {
        // Drag and drop event inside the window

        // Recover the position
        int xWindow, yWindow;
        int xRoot = event->xclient.data.l[2] >> 16;
        int yRoot = event->xclient.data.l[2] & 0xffff;
        Window root = DefaultRootWindow(this->DisplayId);
        Window child;
        XTranslateCoordinates(
          this->DisplayId, root, this->WindowId, xRoot, yRoot, &xWindow, &yWindow, &child);

        // Convert it to VTK compatible location
        double location[2];
        location[0] = static_cast<double>(xWindow);
        location[1] = static_cast<double>(this->Size[1] - yWindow - 1);
        this->InvokeEvent(vtkCommand::UpdateDropLocationEvent, location);

        // Reply that we are ready to copy the dragged data
        XEvent reply;
        memset(&reply, 0, sizeof(reply));

        reply.type = ClientMessage;
        reply.xclient.window = event->xclient.data.l[0];
        reply.xclient.message_type = this->XdndStatusAtom;
        reply.xclient.format = 32;
        reply.xclient.data.l[0] = this->WindowId;
        reply.xclient.data.l[1] = 1; // Always accept the dnd with no rectangle
        reply.xclient.data.l[2] = 0; // Specify an empty rectangle
        reply.xclient.data.l[3] = 0;
        reply.xclient.data.l[4] = this->XdndActionCopyAtom;

        XSendEvent(this->DisplayId, event->xclient.data.l[0], False, NoEventMask, &reply);
        XFlush(this->DisplayId);
      }
      else if (event->xclient.message_type == this->XdndDropAtom)
      {
        // Item dropped in the window
        // Store the source of the drag and drop
        this->XdndSource = event->xclient.data.l[0];

        // Ask for a conversion of the selection. This will trigger a SelectioNotify event later.
        Atom xdndSelectionAtom = XInternAtom(this->DisplayId, "XdndSelection", False);
        XConvertSelection(this->DisplayId, xdndSelectionAtom,
          XInternAtom(this->DisplayId, "UTF8_STRING", False), xdndSelectionAtom, this->WindowId,
          CurrentTime);
      }
      else if (static_cast<Atom>(event->xclient.data.l[0]) == this->KillAtom)
      {
        this->ExitCallback();
      }
    }
    break;
  }
}

//-------------------------------------------------------------------------
void vtkXRenderWindowInteractor::GetMousePosition(int* x, int* y)
{
  Window root, child;
  int root_x, root_y;
  unsigned int keys;

  XQueryPointer(this->DisplayId, this->WindowId, &root, &child, &root_x, &root_y, x, y, &keys);

  *y = this->Size[1] - *y - 1;
}

//-------------------------------------------------------------------------
// void vtkXRenderWindowInteractor::Timer(XtPointer client_data, XtIntervalId* id)
// {
//   vtkXRenderWindowInteractorTimer(client_data, id);
// }

//-------------------------------------------------------------------------
// void vtkXRenderWindowInteractor::Callback(
//   Widget w, XtPointer client_data, XEvent* event, Boolean* ctd)
// {
//   vtkXRenderWindowInteractorCallback(w, client_data, event, ctd);
// }
