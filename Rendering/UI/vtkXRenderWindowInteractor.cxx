// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Must be included first to avoid conflicts with X11's `Status` define.
#include <vtksys/SystemTools.hxx>

#include "vtkXRenderWindowInteractor.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <sys/time.h>

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkInteractorStyle.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkStringArray.h"

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <climits>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
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

constexpr unsigned char XDND_VERSION = 5;

// Map between the X native id to our own integer count id.  Note this
// is separate from the TimerMap in the vtkRenderWindowInteractor
// superclass.  This is used to avoid passing 64-bit values back
// through the "int" return type of InternalCreateTimer.
class vtkXRenderWindowInteractorInternals
{
public:
  vtkXRenderWindowInteractorInternals()
  {
    this->DisplayConnection = -1;
    this->TimerIdCount = 1;
  }
  ~vtkXRenderWindowInteractorInternals() = default;

  // duration is in milliseconds
  int CreateLocalTimer(unsigned long duration)
  {
    int id = this->TimerIdCount++;
    this->LocalToTimer[id].duration = duration;
    gettimeofday(&this->LocalToTimer[id].lastFire, nullptr);
    return id;
  }

  void DestroyLocalTimer(int id) { this->LocalToTimer.erase(id); }

  /**
   * This interactor uses `poll()` to coordinate timers.
   * Returns true if `poll()` needs to block until some time elapses or a user interaction event
   * occurs. Returns false if `poll()` needs to block indefinitely until a user interaction event
   * occurs. The `timeout` arg is populated with a time interval (in milliseconds) that can be
   * used by `poll()` when it needs to use a timeout.
   */
  bool GetTimeToNextTimer(int& timeout)
  {
    bool useTimeout = false; // whether `poll()` must block for some time.
    uint64_t delta = 0;      // in microseconds
    if (!this->LocalToTimer.empty())
    {
      timeval ctv;
      gettimeofday(&ctv, nullptr);
      delta = VTK_UNSIGNED_INT_MAX; // arbitrary high value for time interval
      for (auto& timer : this->LocalToTimer)
      {
        uint64_t duration = timer.second.duration * 1000; // in microsecs
        uint64_t elapsed = (ctv.tv_sec - timer.second.lastFire.tv_sec) * 1000000 + ctv.tv_usec -
          timer.second.lastFire.tv_usec; // in microsecs
        // 0 lets the timer fire immediately.
        delta = duration > elapsed ? std::min<uint64_t>(duration - elapsed, delta) : 0;
        useTimeout = true;
      }
    }
    // max timeout will be VTK_UNSIGNED_INT_MAX/1000, or 4.3e6 milliseconds
    timeout = static_cast<int>(delta / 1000);
    return useTimeout;
  }

  void FireTimers(vtkXRenderWindowInteractor* rwi)
  {
    if (!this->LocalToTimer.empty())
    {
      timeval ctv;
      gettimeofday(&ctv, nullptr);
      std::vector<LocalToTimerType::value_type> timers(
        this->LocalToTimer.begin(), this->LocalToTimer.end());
      for (auto& timer : timers)
      {
        int64_t delta = (ctv.tv_sec - timer.second.lastFire.tv_sec) * 1000000 + ctv.tv_usec -
          timer.second.lastFire.tv_usec;
        if (delta / 1000 >= static_cast<int64_t>(timer.second.duration))
        {
          int timerId = rwi->GetVTKTimerId(timer.first);
          if (timerId != 0)
          {
            rwi->InvokeEvent(vtkCommand::TimerEvent, &timerId);
            if (rwi->IsOneShotTimer(timerId))
            {
              this->DestroyLocalTimer(timer.first);
            }
            else
            {
              auto it = this->LocalToTimer.find(timer.first);
              if (it != this->LocalToTimer.end())
              {
                it->second.lastFire.tv_sec = ctv.tv_sec;
                it->second.lastFire.tv_usec = ctv.tv_usec;
              }
            }
          }
        }
      }
    }
  }

  static std::set<vtkXRenderWindowInteractor*> Instances;

  // whether application was terminated
  bool LoopDone = false;
  int DisplayConnection;

private:
  int TimerIdCount;
  typedef std::map<int, vtkXRenderWindowInteractorTimer> LocalToTimerType;
  LocalToTimerType LocalToTimer;
};

std::set<vtkXRenderWindowInteractor*> vtkXRenderWindowInteractorInternals::Instances;

// for some reason the X11 def of KeySym is getting messed up
typedef XID vtkKeySym;

//------------------------------------------------------------------------------
vtkXRenderWindowInteractor::vtkXRenderWindowInteractor()
{
  this->Internal = new vtkXRenderWindowInteractorInternals;
  this->DisplayId = nullptr;
  this->WindowId = 0;
  this->KillAtom = 0;
  this->XdndSource = 0;
  this->XdndFormatAtom = 0;
  this->XdndURIListAtom = 0;
  this->XdndTypeListAtom = 0;
  this->XdndEnterAtom = 0;
  this->XdndPositionAtom = 0;
  this->XdndDropAtom = 0;
  this->XdndActionCopyAtom = 0;
  this->XdndStatusAtom = 0;
  this->XdndFinishedAtom = 0;
}

//------------------------------------------------------------------------------
vtkXRenderWindowInteractor::~vtkXRenderWindowInteractor()
{
  this->Finalize();

  this->Disable();

  delete this->Internal;
}

//------------------------------------------------------------------------------
// TerminateApp() notifies the event loop to exit.
// The event loop is started by Start() or by one own's method.
// This results in Start() returning to its caller.
void vtkXRenderWindowInteractor::TerminateApp()
{
  if (this->Done)
  {
    return;
  }

  this->Done = true;

  if (this->RenderWindow == nullptr || this->RenderWindow->GetGenericDisplayId() == nullptr)
  {
    return;
  }

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

void vtkXRenderWindowInteractor::ProcessEvents()
{
  auto& internals = (*this->Internal);
  auto& done = internals.LoopDone;

  std::map<Window, vtkXRenderWindowInteractor*> windowmap;
  std::set<Display*> dpys;
  // Make a copy of Instances, the original set might change during loop
  std::vector<vtkXRenderWindowInteractor*> instances(
    vtkXRenderWindowInteractorInternals::Instances.begin(),
    vtkXRenderWindowInteractorInternals::Instances.end());
  for (auto rwi : instances)
  {
    if (rwi->RenderWindow->GetGenericDisplayId() == nullptr)
    {
      // The window has closed the display connection
      rwi->Finalize();
      continue;
    }
    windowmap.insert({ rwi->WindowId, rwi });
    dpys.insert(rwi->DisplayId);
  }

  for (Display* dpy : dpys)
  {
    XEvent event;
    while (this->CheckDisplayId(dpy) && XPending(dpy) != 0)
    {
      // If events are pending, dispatch them to the right RenderWindowInteractor
      XNextEvent(dpy, &event);
      Window w = event.xany.window;
      auto iter = windowmap.find(w);
      if (iter != windowmap.end() && !iter->second->Done)
      {
        iter->second->DispatchEvent(&event);
        iter->second->FireTimers();
      }
    }
  }

  // Set event loop to terminate if there were no displays to check for events
  done = dpys.empty();

  for (auto rwi : vtkXRenderWindowInteractorInternals::Instances)
  {
    if (!rwi->Done)
    {
      rwi->FireTimers();
    }
    // Set event loop to terminate if SetDone(true) or TerminateApp() was
    // called on any of the interactors
    done = done || rwi->Done;
  }
}

//------------------------------------------------------------------------------
void vtkXRenderWindowInteractor::WaitForEvents()
{
  bool useTimeout = false;
  int soonestTimer = 0;

  // check to see how long we wait for the next timer
  for (auto rwi : vtkXRenderWindowInteractorInternals::Instances)
  {
    if (rwi->Done)
    {
      continue;
    }

    int t;
    bool haveTimer = rwi->Internal->GetTimeToNextTimer(t);
    if (haveTimer)
    {
      if (!useTimeout)
      {
        useTimeout = true;
        soonestTimer = t;
      }
      else
      {
        soonestTimer = std::min(soonestTimer, t);
      }
    }
  }

  // build the list of unique display connection fds to poll
  std::vector<pollfd> in_fds;
  for (auto rwi : vtkXRenderWindowInteractorInternals::Instances)
  {
    if (!rwi->Done && rwi->RenderWindow->GetGenericDisplayId() != nullptr)
    {
      int rwi_fd = rwi->Internal->DisplayConnection;
      auto iter = std::lower_bound(in_fds.begin(), in_fds.end(), rwi_fd,
        [](const pollfd& pfd, const int& fd) { return pfd.fd < fd; });

      if (iter == in_fds.end() || iter->fd != rwi_fd)
      {
        in_fds.insert(iter, { rwi_fd, POLLIN, 0 });
      }
    }
  }

  if (!in_fds.empty())
  {
    // poll() will wait until timeout elapses or something else wakes us
    int timeout = useTimeout ? soonestTimer : -1;
    vtkDebugMacro(<< "X event wait, timeout=" << timeout << "ms");
    poll(in_fds.data(), static_cast<nfds_t>(in_fds.size()), timeout);
  }
}

//------------------------------------------------------------------------------
// This will start up the X event loop. If you
// call this method it will loop processing X events until the
// loop is exited.
void vtkXRenderWindowInteractor::StartEventLoop()
{
  // cannot process events without an X display or window.
  if (!this->DisplayId || !this->WindowId)
  {
    return;
  }
  for (auto rwi : vtkXRenderWindowInteractorInternals::Instances)
  {
    rwi->Done = false;
  }

  auto& loopDone = this->Internal->LoopDone;
  do
  {
    // process pending events.
    this->ProcessEvents();
    // wait for events if application is not yet terminated.
    if (!loopDone)
    {
      this->WaitForEvents();
    }
  } while (!loopDone);
}

//------------------------------------------------------------------------------
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

  ren->EnsureDisplay();
  this->DisplayId = static_cast<Display*>(ren->GetGenericDisplayId());

  vtkXRenderWindowInteractorInternals::Instances.insert(this);

  size = ren->GetActualSize();
  size[0] = ((size[0] > 0) ? size[0] : 300);
  size[1] = ((size[1] > 0) ? size[1] : 300);
  if (this->DisplayId)
  {
    this->Internal->DisplayConnection = ConnectionNumber(this->DisplayId);
    XSync(this->DisplayId, False);
  }

  ren->Start();
  ren->End();

  this->WindowId = reinterpret_cast<Window>(ren->GetGenericWindowId());

  if (this->DisplayId && this->WindowId)
  {
    XWindowAttributes attribs;
    //  Find the current window size
    auto* previousHandler = XSetErrorHandler([](Display*, XErrorEvent*) { return 0; });
    if (XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs))
    {
      size[0] = attribs.width;
      size[1] = attribs.height;
    }
    else
    {
      // window does not exist. `ren` is not an X11 render window.
      this->WindowId = 0;
    }
    XSetErrorHandler(previousHandler);
  }
  ren->SetSize(size[0], size[1]);

  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//------------------------------------------------------------------------------
bool vtkXRenderWindowInteractor::CheckDisplayId(Display* dpy)
{
  bool good = false;
  for (auto rwi : vtkXRenderWindowInteractorInternals::Instances)
  {
    if (rwi->DisplayId == dpy)
    {
      if (rwi->RenderWindow->GetGenericDisplayId() != nullptr)
      {
        good = true;
        continue;
      }
      vtkDebugMacro(<< "RenderWindow->DisplayId is null for " << rwi->GetObjectDescription());
    }
  }

  return good;
}

//------------------------------------------------------------------------------
void vtkXRenderWindowInteractor::Finalize()
{
  vtkXRenderWindowInteractorInternals::Instances.erase(this);

  if (this->RenderWindow)
  {
    // Finalize the window
    this->RenderWindow->Finalize();
  }

  // disconnect from the display, even if we didn't own it
  this->DisplayId = nullptr;
  this->Internal->DisplayConnection = -1;

  // revert to uninitialized state
  this->Initialized = false;
  this->Enabled = false;
}

//------------------------------------------------------------------------------
void vtkXRenderWindowInteractor::Enable()
{
  // avoid cycles of calling Initialize() and Enable()
  if (this->Enabled)
  {
    return;
  }
  // When we're attached to an offscreen render window,
  // there is no real X Display or X Window.
  if (!this->WindowId || !this->DisplayId)
  {
    this->Enabled = 1;
    this->Modified();
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
  XChangeProperty(
    this->DisplayId, this->WindowId, xdndAwareAtom, XA_ATOM, 32, PropModeReplace, &XDND_VERSION, 1);
  this->XdndURIListAtom = XInternAtom(this->DisplayId, "text/uri-list", False);
  this->XdndTypeListAtom = XInternAtom(this->DisplayId, "XdndTypeList", False);
  this->XdndEnterAtom = XInternAtom(this->DisplayId, "XdndEnter", False);
  this->XdndPositionAtom = XInternAtom(this->DisplayId, "XdndPosition", False);
  this->XdndDropAtom = XInternAtom(this->DisplayId, "XdndDrop", False);
  this->XdndActionCopyAtom = XInternAtom(this->DisplayId, "XdndActionCopy", False);
  this->XdndStatusAtom = XInternAtom(this->DisplayId, "XdndStatus", False);
  this->XdndFinishedAtom = XInternAtom(this->DisplayId, "XdndFinished", False);

  this->Enabled = 1;

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkXRenderWindowInteractor::Disable()
{
  if (!this->Enabled)
  {
    return;
  }

  this->Enabled = 0;

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkXRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkXRenderWindowInteractor::UpdateSizeNoXResize(int x, int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0]) || (y != this->Size[1]))
  {
    this->Size[0] = x;
    this->Size[1] = y;
    // change the ivars but don't resize the X window
    this->RenderWindow->vtkRenderWindow::SetSize(x, y);
  }
}

//------------------------------------------------------------------------------
void vtkXRenderWindowInteractor::FireTimers()
{
  if (this->GetEnabled())
  {
    this->Internal->FireTimers(this);
  }
}

//------------------------------------------------------------------------------
// X always creates one shot timers
int vtkXRenderWindowInteractor::InternalCreateTimer(
  int vtkNotUsed(timerId), int vtkNotUsed(timerType), unsigned long duration)
{
  duration = (duration > 0 ? duration : this->TimerDuration);
  int platformTimerId = this->Internal->CreateLocalTimer(duration);
  return platformTimerId;
}

//------------------------------------------------------------------------------
int vtkXRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  this->Internal->DestroyLocalTimer(platformTimerId);
  return 1;
}

//------------------------------------------------------------------------------
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
      XConfigureEvent* configureEvent = reinterpret_cast<XConfigureEvent*>(event);
      int width = configureEvent->width;
      int height = configureEvent->height;
      if (width != this->Size[0] || height != this->Size[1])
      {
        bool resizeSmaller = width <= this->Size[0] && height <= this->Size[1];
        this->UpdateSizeNoXResize(width, height);
        xp = configureEvent->x;
        yp = configureEvent->y;
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
      XButtonEvent* buttonEvent = reinterpret_cast<XButtonEvent*>(event);
      int ctrl = (buttonEvent->state & ControlMask) ? 1 : 0;
      int shift = (buttonEvent->state & ShiftMask) ? 1 : 0;
      int alt = (buttonEvent->state & Mod1Mask) ? 1 : 0;
      xp = buttonEvent->x;
      yp = buttonEvent->y;

      // check for double click
      static int MousePressTime = 0;
      int repeat = 0;
      // 400 ms threshold by default is probably good to start
      int eventTime = static_cast<int>(buttonEvent->time);
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
      switch (buttonEvent->button)
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
      XButtonEvent* buttonEvent = reinterpret_cast<XButtonEvent*>(event);
      int ctrl = (buttonEvent->state & ControlMask) ? 1 : 0;
      int shift = (buttonEvent->state & ShiftMask) ? 1 : 0;
      int alt = (buttonEvent->state & Mod1Mask) ? 1 : 0;
      xp = buttonEvent->x;
      yp = buttonEvent->y;
      this->SetEventInformationFlipY(xp, yp, ctrl, shift);
      this->SetAltKey(alt);
      switch (buttonEvent->button)
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
        this->SetAltKey((e->state & Mod1Mask) ? 1 : 0);
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
        this->SetAltKey((e->state & Mod1Mask) ? 1 : 0);
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
      XKeyEvent* keyEvent = reinterpret_cast<XKeyEvent*>(event);
      int ctrl = (keyEvent->state & ControlMask) ? 1 : 0;
      int shift = (keyEvent->state & ShiftMask) ? 1 : 0;
      int alt = (keyEvent->state & Mod1Mask) ? 1 : 0;

      // XLookupString provide a keycode as a char in Basic Latin and Latin1
      // unicode blocks. We care only for the first char of the keycode.
      char keyCode;
      vtkKeySym keySym;
      XLookupString(keyEvent, &keyCode, 1, &keySym, nullptr);

      xp = keyEvent->x;
      yp = keyEvent->y;
      this->SetEventInformationFlipY(xp, yp, ctrl, shift, keyCode, 1, XKeysymToString(keySym));
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
      XKeyEvent* keyEvent = reinterpret_cast<XKeyEvent*>(event);
      int ctrl = (keyEvent->state & ControlMask) ? 1 : 0;
      int shift = (keyEvent->state & ShiftMask) ? 1 : 0;
      int alt = (keyEvent->state & Mod1Mask) ? 1 : 0;

      // XLookupString provide a keycode as a char in Basic Latin and Latin1
      // unicode blocks. We care only for the first char of the keycode.
      char keyCode;
      vtkKeySym keySym;
      XLookupString(keyEvent, &keyCode, 1, &keySym, nullptr);

      xp = keyEvent->x;
      yp = keyEvent->y;
      this->SetEventInformationFlipY(xp, yp, ctrl, shift, keyCode, 1, XKeysymToString(keySym));
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
      XMotionEvent* motionEvent = reinterpret_cast<XMotionEvent*>(event);
      int ctrl = (motionEvent->state & ControlMask) ? 1 : 0;
      int shift = (motionEvent->state & ShiftMask) ? 1 : 0;
      int alt = (motionEvent->state & Mod1Mask) ? 1 : 0;

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
        &bytesAfter, reinterpret_cast<unsigned char**>(&data));

      // Conversion checks
      if ((event->xselection.target != AnyPropertyType && actualType != event->xselection.target) ||
        itemCount == 0)
      {
        return;
      }

      // Recover filepaths from uris and invoke DropFilesEvent
      std::stringstream uris(data);
      std::string uri, protocol, hostname, filePath;
      std::string unused0, unused1, unused3;
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

      // Inform the source the the drag and drop operation was successful
      XEvent reply;
      memset(&reply, 0, sizeof(reply));

      reply.type = ClientMessage;
      reply.xclient.window = this->XdndSource;
      reply.xclient.message_type = this->XdndFinishedAtom;
      reply.xclient.format = 32;
      reply.xclient.data.l[0] = this->WindowId;
      reply.xclient.data.l[1] = 1;

      if (this->XdndSourceVersion >= 2)
      {
        reply.xclient.data.l[2] = this->XdndActionCopyAtom;
      }

      XSendEvent(this->DisplayId, this->XdndSource, False, NoEventMask, &reply);
      XFlush(this->DisplayId);
      this->XdndSource = 0;
    }
    break;

    case ClientMessage:
    {
      if (event->xclient.message_type == this->XdndEnterAtom)
      {
        // Drag and drop event enter the window
        this->XdndSource = event->xclient.data.l[0];

        // Check version
        this->XdndSourceVersion = event->xclient.data.l[1] >> 24;
        if (this->XdndSourceVersion > XDND_VERSION)
        {
          return;
        }

        // Recover the formats provided by the dnd source
        Atom* formats = nullptr;
        unsigned long count;
        bool list = event->xclient.data.l[1] & 1;
        if (list)
        {
          Atom actualType;
          int actualFormat;
          unsigned long bytesAfter;
          XGetWindowProperty(this->DisplayId, this->XdndSource, this->XdndTypeListAtom, 0, LONG_MAX,
            False, XA_ATOM, &actualType, &actualFormat, &count, &bytesAfter,
            reinterpret_cast<unsigned char**>(&formats));
        }
        else
        {
          count = 3;
          formats = reinterpret_cast<Atom*>(event->xclient.data.l + 2);
        }

        // Check one of these format is an URI list
        // Which is the only supported format
        for (unsigned long i = 0; i < count; i++)
        {
          if (formats[i] == this->XdndURIListAtom)
          {
            this->XdndFormatAtom = XdndURIListAtom;
            break;
          }
        }

        // Free the allocated formats
        if (list && formats)
        {
          XFree(formats);
        }
      }
      if (event->xclient.message_type == this->XdndPositionAtom)
      {
        // Drag and drop event inside the window
        if (this->XdndSource != static_cast<Window>(event->xclient.data.l[0]))
        {
          vtkWarningMacro("Only one dnd action at a time is supported");
          return;
        }

        // Recover the position
        int location[2];
        unsigned int keys;
        this->GetMousePositionAndModifierKeysState(&location[0], &location[1], &keys);
        int ctrl = keys & ControlMask ? 1 : 0;
        int shift = keys & ShiftMask ? 1 : 0;
        int alt = keys & Mod1Mask ? 1 : 0;
        this->SetEventInformationFlipY(location[0], location[1], ctrl, shift);
        this->SetAltKey(alt);
        this->InvokeEvent(vtkCommand::UpdateDropLocationEvent, location);

        // Reply that we are ready to copy the dragged data
        XEvent reply;
        memset(&reply, 0, sizeof(reply));

        reply.type = ClientMessage;
        reply.xclient.window = this->XdndSource;
        reply.xclient.message_type = this->XdndStatusAtom;
        reply.xclient.format = 32;
        reply.xclient.data.l[0] = this->WindowId;
        reply.xclient.data.l[1] = 1; // Always accept the dnd with no rectangle
        reply.xclient.data.l[2] = 0; // Specify an empty rectangle
        reply.xclient.data.l[3] = 0;
        reply.xclient.data.l[4] = this->XdndActionCopyAtom;

        XSendEvent(this->DisplayId, this->XdndSource, False, NoEventMask, &reply);
        XFlush(this->DisplayId);
      }
      else if (event->xclient.message_type == this->XdndDropAtom)
      {
        // Item dropped in the window
        if (this->XdndSource != static_cast<Window>(event->xclient.data.l[0]))
        {
          vtkWarningMacro("Only one dnd action at a time is supported");
          return;
        }

        if (this->XdndFormatAtom)
        {
          // Ask for a conversion of the selection. This will trigger a SelectionNotify event later.
          Atom xdndSelectionAtom = XInternAtom(this->DisplayId, "XdndSelection", False);
          XConvertSelection(this->DisplayId, xdndSelectionAtom, this->XdndFormatAtom,
            xdndSelectionAtom, this->WindowId, CurrentTime);
        }
        else if (this->XdndSourceVersion >= 2)
        {
          XEvent reply;
          memset(&reply, 0, sizeof(reply));

          reply.type = ClientMessage;
          reply.xclient.window = this->XdndSource;
          reply.xclient.message_type = this->XdndFinishedAtom;
          reply.xclient.format = 32;
          reply.xclient.data.l[0] = this->WindowId;
          reply.xclient.data.l[1] = 0; // The drag was rejected
          reply.xclient.data.l[2] = None;

          XSendEvent(this->DisplayId, this->XdndSource, False, NoEventMask, &reply);
          XFlush(this->DisplayId);
        }
      }
      else if (static_cast<Atom>(event->xclient.data.l[0]) == this->KillAtom)
      {
        this->ExitCallback();
      }
    }
    break;
  }
}

//------------------------------------------------------------------------------
void vtkXRenderWindowInteractor::GetMousePosition(int* x, int* y)
{
  unsigned int keys;
  this->GetMousePositionAndModifierKeysState(x, y, &keys);
}

//------------------------------------------------------------------------------
void vtkXRenderWindowInteractor::GetMousePositionAndModifierKeysState(
  int* x, int* y, unsigned int* keys)
{
  Window root, child;
  int root_x, root_y;
  XQueryPointer(this->DisplayId, this->WindowId, &root, &child, &root_x, &root_y, x, y, keys);
  *y = this->Size[1] - *y - 1;
}

//------------------------------------------------------------------------------
// void vtkXRenderWindowInteractor::Timer(XtPointer client_data, XtIntervalId* id)
// {
//   vtkXRenderWindowInteractorTimer(client_data, id);
// }

//------------------------------------------------------------------------------
// void vtkXRenderWindowInteractor::Callback(
//   Widget w, XtPointer client_data, XEvent* event, Boolean* ctd)
// {
//   vtkXRenderWindowInteractorCallback(w, client_data, event, ctd);
// }
VTK_ABI_NAMESPACE_END
