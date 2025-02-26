// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkObjectFactory.h"

#include "vtkImageData.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkXWebGPURenderWindow.h"

// STL includes
// #include <sstream>

// X11 includes
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#if VTK_HAVE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>

VTK_ABI_NAMESPACE_BEGIN
//-------------------------------------------------------------------------------------------------
vtkStandardNewMacro(vtkXWebGPURenderWindow);

//-------------------------------------------------------------------------------------------------
vtkXWebGPURenderWindow::vtkXWebGPURenderWindow()
{
  this->ParentId = static_cast<Window>(0);
  this->OwnDisplay = 0;
  this->CursorHidden = 0;
  this->UsingHardware = 0;
  this->DisplayId = static_cast<Display*>(nullptr);
  this->WindowId = static_cast<Window>(0);
  this->NextWindowId = static_cast<Window>(0);
  this->ColorMap = static_cast<Colormap>(0);
  this->OwnWindow = 0;

  this->XCCrosshair = 0;
  this->XCArrow = 0;
  this->XCSizeAll = 0;
  this->XCSizeNS = 0;
  this->XCSizeWE = 0;
  this->XCSizeNE = 0;
  this->XCSizeNW = 0;
  this->XCSizeSE = 0;
  this->XCSizeSW = 0;
  this->XCHand = 0;
  this->XCCustom = 0;
}

//-------------------------------------------------------------------------------------------------
// free up memory & close the window
vtkXWebGPURenderWindow::~vtkXWebGPURenderWindow()
{
  vtkRenderer* renderer;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((renderer = this->Renderers->GetNextRenderer(rit)))
  {
    renderer->ReleaseGraphicsResources(this);
    renderer->SetRenderWindow(nullptr);
  }
  this->Renderers->RemoveAllItems();
  // Finalize in turn destroys the WGPUInstance. As a result, it must be called after all renderers
  // are destroyed. Otherwise, the destructors of WGPU objects held on to by the vtkRenderer will
  // occur after WGPUInstance is gone, which can crash applications.
  this->Finalize();
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Display Id: " << this->GetDisplayId() << "\n";
  os << indent << "Next Window Id: " << this->NextWindowId << "\n";
  os << indent << "Window Id: " << this->GetWindowId() << "\n";
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::Frame()
{
  if (!this->AbortRender && this->WindowId != 0)
  {
    this->Superclass::Frame();
  }
}

//------------------------------------------------------------------------------------------------
bool vtkXWebGPURenderWindow::InitializeFromCurrentContext()
{
  this->Superclass::InitializeFromCurrentContext();
  // TODO: Initialize the window/display parameters
  return false;
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::SetStereoCapableWindow(vtkTypeBool capable)
{
  if (!this->WGPUConfiguration)
  {
    vtkErrorMacro(
      << "vtkWebGPUConfiguration is null! Please provide one with SetWGPUConfiguration");
  }
  if (!this->WGPUConfiguration->GetDevice().Get())
  {
    this->Superclass::SetStereoCapableWindow(capable);
  }
  else
  {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
  }
}

//------------------------------------------------------------------------------------------------
template <int EventType>
int XEventTypeEquals(Display*, XEvent* event, XPointer winptr)
{
  return (event->type == EventType &&
    *(reinterpret_cast<Window*>(winptr)) == reinterpret_cast<XAnyEvent*>(event)->window);
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::SetShowWindow(bool val)
{
  if (val == this->ShowWindow)
  {
    return;
  }

  if (this->WindowId)
  {
    if (val)
    {
      vtkDebugMacro(" Mapping the xwindow\n");
      XMapWindow(this->DisplayId, this->WindowId);
      XSync(this->DisplayId, False);
      // guarantee that the window is mapped before the program continues
      // on to do the OpenGL rendering.
      XWindowAttributes winattr;
      XGetWindowAttributes(this->DisplayId, this->WindowId, &winattr);
      if (winattr.map_state == IsUnmapped)
      {
        XEvent e;
        XIfEvent(this->DisplayId, &e, XEventTypeEquals<MapNotify>,
          reinterpret_cast<XPointer>(&this->WindowId));
      }
      this->Mapped = 1;
    }
    else
    {
      vtkDebugMacro(" UnMapping the xwindow\n");
      XUnmapWindow(this->DisplayId, this->WindowId);
      XSync(this->DisplayId, False);
      XWindowAttributes winattr;
      XGetWindowAttributes(this->DisplayId, this->WindowId, &winattr);
      // guarantee that the window is unmapped before the program continues
      if (winattr.map_state != IsUnmapped)
      {
        XEvent e;
        XIfEvent(this->DisplayId, &e, XEventTypeEquals<UnmapNotify>,
          reinterpret_cast<XPointer>(&this->WindowId));
      }
      this->Mapped = 0;
    }
  }
  this->Superclass::SetShowWindow(val);
}

//------------------------------------------------------------------------------------------------
std::string vtkXWebGPURenderWindow::MakeDefaultWindowNameWithBackend()
{
  if (this->WGPUConfiguration)
  {
    return std::string("Visualization Toolkit - ") + "X11 " +
      this->WGPUConfiguration->GetBackendInUseAsString();
  }
  else
  {
    return "Visualization Toolkit - X11 undefined backend";
  }
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::CreateAWindow()
{
  XVisualInfo *v, matcher;
  XSetWindowAttributes attr;
  int x, y, width, height, nItems;
  XWindowAttributes winattr;
  XSizeHints xsh;
  XClassHint xch;

  xsh.flags = USSize;
  if ((this->Position[0] >= 0) && (this->Position[1] >= 0))
  {
    xsh.flags |= USPosition;
    xsh.x = static_cast<int>(this->Position[0]);
    xsh.y = static_cast<int>(this->Position[1]);
  }

  x = this->Position[0];
  y = this->Position[1];
  width = ((this->Size[0] > 0) ? this->Size[0] : 300);
  height = ((this->Size[1] > 0) ? this->Size[1] : 300);

  xsh.width = width;
  xsh.height = height;

  // get the default display connection
  if (!this->EnsureDisplay())
  {
    vtkErrorMacro(<< "Aborting in CreateAWindow(), no Display\n");
    abort();
  }

  attr.override_redirect = False;
  if (this->Borders == 0.0)
  {
    attr.override_redirect = True;
  }

  // create our own window ?
  this->OwnWindow = 0;
  if (!this->WindowId)
  {
    int screenId = XDefaultScreen(this->DisplayId);

    XVisualInfo vInfoTemplate = {};
    vInfoTemplate.screen = screenId;
    v = XGetVisualInfo(this->DisplayId, VisualScreenMask, &vInfoTemplate, &nItems);
    if (!v)
    {
      vtkErrorMacro(<< "Could not find a decent visual\n");
      abort();
    }
    this->ColorMap = XCreateColormap(
      this->DisplayId, XRootWindow(this->DisplayId, v->screen), v->visual, AllocNone);

    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = this->ColorMap;
    attr.event_mask = StructureNotifyMask | ExposureMask;

    // get a default parent if one has not been set.
    if (!this->ParentId)
    {
      this->ParentId = XRootWindow(this->DisplayId, XDefaultScreen(this->DisplayId));
    }
    this->WindowId =
      XCreateWindow(this->DisplayId, this->ParentId, x, y, static_cast<unsigned int>(width),
        static_cast<unsigned int>(height), 0, v->depth, InputOutput, v->visual,
        CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect | CWEventMask, &attr);
    XStoreName(this->DisplayId, this->WindowId, this->WindowName);
    XSetNormalHints(this->DisplayId, this->WindowId, &xsh);

    char classStr[4] = "Vtk";
    char nameStr[4] = "vtk";
    xch.res_class = classStr;
    xch.res_name = nameStr;
    XSetClassHint(this->DisplayId, this->WindowId, &xch);

    this->OwnWindow = 1;
  }
  else
  {
    XChangeWindowAttributes(this->DisplayId, this->WindowId, CWOverrideRedirect, &attr);
    XGetWindowAttributes(this->DisplayId, this->WindowId, &winattr);
    matcher.visualid = XVisualIDFromVisual(winattr.visual);
    matcher.screen = XDefaultScreen(DisplayId);
    v = XGetVisualInfo(this->DisplayId, VisualIDMask | VisualScreenMask, &matcher, &nItems);
  }

  if (this->OwnWindow)
  {
    // RESIZE THE WINDOW TO THE DESIRED SIZE
    vtkDebugMacro(<< "Resizing the xwindow\n");
    XResizeWindow(this->DisplayId, this->WindowId,
      ((this->Size[0] > 0) ? static_cast<unsigned int>(this->Size[0]) : 300),
      ((this->Size[1] > 0) ? static_cast<unsigned int>(this->Size[1]) : 300));
    XSync(this->DisplayId, False);
  }

  if (this->OwnWindow && this->ShowWindow)
  {
    vtkDebugMacro(" Mapping the xwindow\n");
    XMapWindow(this->DisplayId, this->WindowId);
    XSync(this->DisplayId, False);
    XEvent e;
    XIfEvent(this->DisplayId, &e, XEventTypeEquals<MapNotify>,
      reinterpret_cast<XPointer>(&this->WindowId));
    XGetWindowAttributes(this->DisplayId, this->WindowId, &winattr);
    // if the specified window size is bigger than the screen size,
    // we have to reset the window size to the screen size
    width = winattr.width;
    height = winattr.height;
    this->Mapped = 1;

    if (this->FullScreen)
    {
      XGrabKeyboard(
        this->DisplayId, this->WindowId, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    }
  }
  // free the visual info
  if (v)
  {
    XFree(v);
  }
  this->Size[0] = width;
  this->Size[1] = height;
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::DestroyWindow()
{
  // free the cursors
  if (this->DisplayId)
  {
    if (this->WindowId)
    {
      // we will only have a cursor defined if a CurrentCursor has been
      // set > 0 or if the cursor has been hidden... if we undefine without
      // checking, bad things can happen (BadWindow)
      if (this->GetCurrentCursor() || this->CursorHidden)
      {
        XUndefineCursor(this->DisplayId, this->WindowId);
      }
    }
    if (this->XCArrow)
    {
      XFreeCursor(this->DisplayId, this->XCArrow);
    }
    if (this->XCCrosshair)
    {
      XFreeCursor(this->DisplayId, this->XCCrosshair);
    }
    if (this->XCSizeAll)
    {
      XFreeCursor(this->DisplayId, this->XCSizeAll);
    }
    if (this->XCSizeNS)
    {
      XFreeCursor(this->DisplayId, this->XCSizeNS);
    }
    if (this->XCSizeWE)
    {
      XFreeCursor(this->DisplayId, this->XCSizeWE);
    }
    if (this->XCSizeNE)
    {
      XFreeCursor(this->DisplayId, this->XCSizeNE);
    }
    if (this->XCSizeNW)
    {
      XFreeCursor(this->DisplayId, this->XCSizeNW);
    }
    if (this->XCSizeSE)
    {
      XFreeCursor(this->DisplayId, this->XCSizeSE);
    }
    if (this->XCSizeSW)
    {
      XFreeCursor(this->DisplayId, this->XCSizeSW);
    }
    if (this->XCHand)
    {
      XFreeCursor(this->DisplayId, this->XCHand);
    }
    if (this->XCCustom)
    {
      XFreeCursor(this->DisplayId, this->XCCustom);
    }
  }

  this->XCCrosshair = 0;
  this->XCArrow = 0;
  this->XCSizeAll = 0;
  this->XCSizeNS = 0;
  this->XCSizeWE = 0;
  this->XCSizeNE = 0;
  this->XCSizeNW = 0;
  this->XCSizeSE = 0;
  this->XCSizeSW = 0;
  this->XCHand = 0;
  this->XCCustom = 0;

  // Release resources
  this->ReleaseGraphicsResources(this);

  if (this->DisplayId && this->WindowId)
  {
    if (this->OwnWindow)
    {
      // close the window if we own it
      XDestroyWindow(this->DisplayId, this->WindowId);
      this->WindowId = static_cast<Window>(0);
    }
    else
    {
      // if we don't own it, simply unmap the window
      XUnmapWindow(this->DisplayId, this->WindowId);
    }
    this->Mapped = 0;
  }

  this->CloseDisplay();

  // make sure all other code knows we're not mapped anymore
  this->Mapped = 0;
}

//------------------------------------------------------------------------------------------------
// Initialize the window for rendering
void vtkXWebGPURenderWindow::WindowInitialize()
{
  this->CreateAWindow();

  // tell our renderers about us
  vtkRenderer* renderer;
  for (this->Renderers->InitTraversal(); (renderer = this->Renderers->GetNextItem());)
  {
    renderer->SetRenderWindow(nullptr);
    renderer->SetRenderWindow(this);
  }
}

//------------------------------------------------------------------------------------------------
// Initialize the rendering window.
bool vtkXWebGPURenderWindow::WindowSetup()
{
  if (!this->WGPUConfiguration)
  {
    vtkErrorMacro(
      << "vtkWebGPUConfiguration is null! Please provide one with SetWGPUConfiguration");
    return false;
  }
  if (!this->WindowId || !this->DisplayId)
  {
    // initialize the window
    this->WindowInitialize();
  }

  if (this->WGPUInit())
  {
    wgpu::SurfaceSourceXlibWindow x11SurfDesc = {};
    x11SurfDesc.display = this->GetDisplayId();
    x11SurfDesc.window = this->GetWindowId();
    wgpu::SurfaceDescriptor surfDesc = {};
    surfDesc.label = "VTK X11 surface";
    surfDesc.nextInChain = &x11SurfDesc;
    this->Surface = this->WGPUConfiguration->GetInstance().CreateSurface(&surfDesc);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::Finalize()
{
  if (this->Initialized)
  {
    this->WGPUFinalize();
  }
  // clean and destroy window
  this->DestroyWindow();
}

//------------------------------------------------------------------------------------------------
// Change the window to fill the entire screen.
void vtkXWebGPURenderWindow::SetFullScreen(vtkTypeBool arg)
{
  int* temp;

  if (this->UseOffScreenBuffers)
  {
    return;
  }

  if (this->FullScreen == arg)
  {
    return;
  }

  this->FullScreen = arg;

  if (!this->Mapped)
  {
    this->PrefFullScreen();
    return;
  }

  // set the mode
  if (this->FullScreen <= 0)
  {
    this->Position[0] = this->OldScreen[0];
    this->Position[1] = this->OldScreen[1];
    this->Size[0] = this->OldScreen[2];
    this->Size[1] = this->OldScreen[3];
    this->Borders = this->OldScreen[4];
  }
  else
  {
    // if window already up get its values
    if (this->WindowId)
    {
      XWindowAttributes attribs;

      //  Find the current window size
      XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);

      this->OldScreen[2] = attribs.width;
      this->OldScreen[3] = attribs.height;

      temp = this->GetPosition();
      this->OldScreen[0] = temp[0];
      this->OldScreen[1] = temp[1];

      this->OldScreen[4] = this->Borders;
      this->PrefFullScreen();
    }
  }

  // remap the window
  this->WindowRemap();

  this->Modified();
}

//------------------------------------------------------------------------------------------------
// Set the preferred window size to full screen.
void vtkXWebGPURenderWindow::PrefFullScreen()
{
  // use full screen
  this->Position[0] = 0;
  this->Position[1] = 0;

  if (this->UseOffScreenBuffers)
  {
    this->Size[0] = 1280;
    this->Size[1] = 1024;
  }
  else
  {
    const int* size = this->GetScreenSize();
    this->Size[0] = size[0];
    this->Size[1] = size[1];
  }

  // don't show borders
  this->Borders = 0;
}

//------------------------------------------------------------------------------------------------
// Resize the window.
void vtkXWebGPURenderWindow::WindowRemap()
{
  // shut everything down
  this->Finalize();

  // set the default windowid
  this->WindowId = this->NextWindowId;
  this->NextWindowId = static_cast<Window>(0);

  // set everything up again
  this->Initialize();
}

//------------------------------------------------------------------------------------------------
// Specify the size of the rendering window.
void vtkXWebGPURenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width) || (this->Size[1] != height))
  {
    this->Superclass::SetSize(width, height);

    if (this->WindowId)
    {
      if (this->Interactor)
      {
        this->Interactor->SetSize(width, height);
      }

      // get baseline serial number for X requests generated from XResizeWindow
      unsigned long serial = NextRequest(this->DisplayId);

      // request a new window size from the X server
      XResizeWindow(this->DisplayId, this->WindowId, static_cast<unsigned int>(width),
        static_cast<unsigned int>(height));

      // flush output queue and wait for X server to processes the request
      XSync(this->DisplayId, False);

      // The documentation for XResizeWindow includes this important note:
      //
      //   If the override-redirect flag of the window is False and some
      //   other client has selected SubstructureRedirectMask on the parent,
      //   the X server generates a ConfigureRequest event, and no further
      //   processing is performed.
      //
      // What this means, essentially, is that if this window is a top-level
      // window, then it's the window manager (the "other client") that is
      // responsible for changing this window's size.  So when we call
      // XResizeWindow() on a top-level window, then instead of resizing
      // the window immediately, the X server informs the window manager,
      // and then the window manager sets our new size (usually it will be
      // the size we asked for).  We receive a ConfigureNotify event when
      // our new size has been set.

      // check our override-redirect flag
      XWindowAttributes attrs;
      XGetWindowAttributes(this->DisplayId, this->WindowId, &attrs);
      if (!attrs.override_redirect && this->ParentId)
      {
        // check if parent has SubstructureRedirectMask
        XWindowAttributes parentAttrs;
        XGetWindowAttributes(this->DisplayId, this->ParentId, &parentAttrs);
        if ((parentAttrs.all_event_masks & SubstructureRedirectMask) == SubstructureRedirectMask)
        {
          // set the wait timeout to be 2 seconds from now
          double maxtime = 2.0 + vtksys::SystemTools::GetTime();
          // look for a ConfigureNotify that came *after* XResizeWindow
          XEvent e;
          while (!XCheckIfEvent(this->DisplayId, &e, XEventTypeEquals<ConfigureNotify>,
                   reinterpret_cast<XPointer>(&this->WindowId)) ||
            e.xconfigure.serial < serial)
          {
            // wait for 10 milliseconds and try again until time runs out
            vtksys::SystemTools::Delay(10);
            if (vtksys::SystemTools::GetTime() > maxtime)
            {
              vtkWarningMacro(<< "Timeout while waiting for response to XResizeWindow.");
              break;
            }
          }
        }
      }
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------------------------
vtkTypeBool vtkXWebGPURenderWindowFoundMatch;

extern "C"
{
  Bool vtkXWebGPURenderWindowPredProc(Display* vtkNotUsed(disp), XEvent* event, char* arg)
  {
    Window win = (Window)arg;

    if (((reinterpret_cast<XAnyEvent*>(event))->window == win) && ((event->type == ButtonPress)))
    {
      vtkXWebGPURenderWindowFoundMatch = 1;
    }

    return 0;
  }
}

//------------------------------------------------------------------------------------------------
vtkTypeBool vtkXWebGPURenderWindow::GetEventPending()
{
  XEvent report;

  vtkXWebGPURenderWindowFoundMatch = 0;
  if (!this->ShowWindow)
  {
    return vtkXWebGPURenderWindowFoundMatch;
  }
  XCheckIfEvent(this->DisplayId, &report, vtkXWebGPURenderWindowPredProc,
    reinterpret_cast<char*>(this->WindowId));
  return vtkXWebGPURenderWindowFoundMatch;
}

//------------------------------------------------------------------------------------------------
// Get the size of the screen in pixels
int* vtkXWebGPURenderWindow::GetScreenSize()
{
  // get the default display connection
  if (!this->EnsureDisplay())
  {
    this->ScreenSize[0] = 0;
    this->ScreenSize[1] = 0;
    return this->ScreenSize;
  }

  this->ScreenSize[0] = XDisplayWidth(this->DisplayId, XDefaultScreen(this->DisplayId));
  this->ScreenSize[1] = XDisplayHeight(this->DisplayId, XDefaultScreen(this->DisplayId));

  return this->ScreenSize;
}

//------------------------------------------------------------------------------------------------
// Get the position in screen coordinates (pixels) of the window.
int* vtkXWebGPURenderWindow::GetPosition()
{
  XWindowAttributes attribs;
  int x, y;
  Window child;

  if (!this->WindowId)
  {
    return this->Position;
  }

  //  Find the current window size
  XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);
  x = attribs.x;
  y = attribs.y;

  XTranslateCoordinates(this->DisplayId, this->ParentId,
    XRootWindowOfScreen(XScreenOfDisplay(this->DisplayId, 0)), x, y, &this->Position[0],
    &this->Position[1], &child);

  return this->Position;
}

//------------------------------------------------------------------------------------------------
// Get this RenderWindow's X display id.
Display* vtkXWebGPURenderWindow::GetDisplayId()
{
  vtkDebugMacro(<< "Returning DisplayId of " << static_cast<void*>(this->DisplayId) << "\n");

  return this->DisplayId;
}

//------------------------------------------------------------------------------------------------
bool vtkXWebGPURenderWindow::EnsureDisplay()
{
  if (!this->DisplayId)
  {
    this->DisplayId = XOpenDisplay(static_cast<char*>(nullptr));
    if (this->DisplayId == nullptr)
    {
      vtkWarningMacro(<< "bad X server connection. DISPLAY="
                      << vtksys::SystemTools::GetEnv("DISPLAY"));
    }
    else
    {
      this->OwnDisplay = 1;
    }
  }

  return this->DisplayId != nullptr;
}

//------------------------------------------------------------------------------------------------
// Get this RenderWindow's parent X window id.
Window vtkXWebGPURenderWindow::GetParentId()
{
  vtkDebugMacro(<< "Returning ParentId of " << reinterpret_cast<void*>(this->ParentId) << "\n");
  return this->ParentId;
}

//------------------------------------------------------------------------------------------------
// Get this RenderWindow's X window id.
Window vtkXWebGPURenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << reinterpret_cast<void*>(this->WindowId) << "\n");
  return this->WindowId;
}

//------------------------------------------------------------------------------------------------
// Move the window to a new position on the display.
void vtkXWebGPURenderWindow::SetPosition(int x, int y)
{
  // if we aren't mapped then just set the ivars
  if (!this->WindowId)
  {
    if ((this->Position[0] != x) || (this->Position[1] != y))
    {
      this->Modified();
    }
    this->Position[0] = x;
    this->Position[1] = y;
    return;
  }

  XMoveWindow(this->DisplayId, this->WindowId, x, y);
  XSync(this->DisplayId, False);
}

//------------------------------------------------------------------------------------------------
// Sets the parent of the window that WILL BE created.
void vtkXWebGPURenderWindow::SetParentId(Window arg)
{
  vtkDebugMacro(<< "Setting ParentId to " << reinterpret_cast<void*>(arg) << "\n");

  this->ParentId = arg;
}

//------------------------------------------------------------------------------------------------
// Set this RenderWindow's X window id to a pre-existing window.
void vtkXWebGPURenderWindow::SetWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << reinterpret_cast<void*>(arg) << "\n");

  this->WindowId = arg;

  if (this->CursorHidden)
  {
    this->CursorHidden = 0;
    this->HideCursor();
  }
}

//------------------------------------------------------------------------------------------------
// Set this RenderWindow's X window id to a pre-existing window.
void vtkXWebGPURenderWindow::SetWindowInfo(const char* info)
{
  // note: potential Display/Window mismatch here
  this->EnsureDisplay();

  int tmp;
  sscanf(info, "%i", &tmp);

  this->SetWindowId(static_cast<Window>(tmp));
}

//------------------------------------------------------------------------------------------------
// Set this RenderWindow's X window id to a pre-existing window.
void vtkXWebGPURenderWindow::SetNextWindowInfo(const char* info)
{
  int tmp;
  sscanf(info, "%i", &tmp);

  this->SetNextWindowId(static_cast<Window>(tmp));
}

//------------------------------------------------------------------------------------------------
// Sets the X window id of the window that WILL BE created.
void vtkXWebGPURenderWindow::SetParentInfo(const char* info)
{
  // note: potential Display/Window mismatch here
  this->EnsureDisplay();

  int tmp;
  sscanf(info, "%i", &tmp);

  this->SetParentId(static_cast<Window>(tmp));
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::SetWindowId(void* arg)
{
  this->SetWindowId(reinterpret_cast<Window>(arg));
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::SetParentId(void* arg)
{
  this->SetParentId(reinterpret_cast<Window>(arg));
}

//------------------------------------------------------------------------------------------------
const char* vtkXWebGPURenderWindow::ReportCapabilities()
{
  if (!this->DisplayId)
  {
    return "display id not set";
  }

  // TODO: Ask WebGPU to
  // return this->Capabilities;
  return "";
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::CloseDisplay()
{
  // if we create the display, we'll delete it
  if (this->OwnDisplay && this->DisplayId)
  {
    XCloseDisplay(this->DisplayId);
  }

  // disconnect from the display, even if we didn't own it
  this->DisplayId = nullptr;
  this->OwnDisplay = false;
}

//------------------------------------------------------------------------------------------------
vtkTypeBool vtkXWebGPURenderWindow::IsDirect()
{
  this->UsingHardware = 0;
  if (this->GetDisplayId() && this->GetDevice())
  {
    this->UsingHardware = true;
  }
  return this->UsingHardware;
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::SetWindowName(const char* cname)
{
  char* name = new char[strlen(cname) + 1];
  strcpy(name, cname);
  XTextProperty win_name_text_prop;

  this->Superclass::SetWindowName(name);

  if (this->WindowId)
  {
    if (XStringListToTextProperty(&name, 1, &win_name_text_prop) == 0)
    {
      XFree(win_name_text_prop.value);
      vtkWarningMacro(<< "Can't rename window");
      delete[] name;
      return;
    }

    XSetWMName(this->DisplayId, this->WindowId, &win_name_text_prop);
    XSetWMIconName(this->DisplayId, this->WindowId, &win_name_text_prop);
    XFree(win_name_text_prop.value);
  }
  delete[] name;
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::SetIcon(vtkImageData* img)
{
  int dim[3];
  img->GetDimensions(dim);

  int nbComp = img->GetNumberOfScalarComponents();

  if (img->GetScalarType() != VTK_UNSIGNED_CHAR || dim[2] != 1 || nbComp < 3 || nbComp > 4)
  {
    vtkErrorMacro(
      "Icon image should be 2D, have 3 or 4 components, and its type must be unsigned char.");
    return;
  }

  unsigned char* imgScalars = static_cast<unsigned char*>(img->GetScalarPointer());

  std::vector<unsigned long> pixels(2 + dim[0] * dim[1]);
  pixels[0] = dim[0];
  pixels[1] = dim[1];

  // Convert vtkImageData buffer to X icon.
  // We need to flip Y and use ARGB 32-bits encoded convention
  for (int col = 0; col < dim[1]; col++)
  {
    for (int line = 0; line < dim[0]; line++)
    {
      unsigned char* inPixel = imgScalars + nbComp * ((dim[0] - col - 1) * dim[1] + line); // flip Y
      unsigned long* outPixel = pixels.data() + col * dim[1] + line + 2;
      if (nbComp == 4)
      {
        *outPixel = nbComp == 4 ? inPixel[3] : 0xff;
      }
      *outPixel = (*outPixel << 8) + inPixel[0];
      *outPixel = (*outPixel << 8) + inPixel[1];
      *outPixel = (*outPixel << 8) + inPixel[2];
    }
  }

  Atom iconAtom = XInternAtom(this->DisplayId, "_NET_WM_ICON", False);
  Atom typeAtom = XInternAtom(this->DisplayId, "CARDINAL", False);
  XChangeProperty(this->DisplayId, this->WindowId, iconAtom, typeAtom, 32, PropModeReplace,
    reinterpret_cast<unsigned char*>(pixels.data()), pixels.size());
}

//------------------------------------------------------------------------------------------------
// Specify the X window id to use if a WindowRemap is done.
void vtkXWebGPURenderWindow::SetNextWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting NextWindowId to " << reinterpret_cast<void*>(arg) << "\n");

  this->NextWindowId = arg;
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::SetNextWindowId(void* arg)
{
  this->SetNextWindowId(reinterpret_cast<Window>(arg));
}

//------------------------------------------------------------------------------------------------
// Set the X display id for this RenderWindow to use to a pre-existing
// X display id.
void vtkXWebGPURenderWindow::SetDisplayId(Display* arg)
{
  vtkDebugMacro(<< "Setting DisplayId to " << static_cast<void*>(arg) << "\n");

  this->DisplayId = arg;
  this->OwnDisplay = 0;
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::SetDisplayId(void* arg)
{
  this->SetDisplayId(static_cast<Display*>(arg));
  this->OwnDisplay = 0;
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::Render()
{
  XWindowAttributes attribs;

  // To avoid the expensive XGetWindowAttributes call,
  // compute size at the start of a render and use
  // the ivar other times.
  if (this->Mapped && !this->UseOffScreenBuffers)
  {
    //  Find the current window size
    XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);

    this->Size[0] = attribs.width;
    this->Size[1] = attribs.height;
  }

  // Now do the superclass stuff
  this->Superclass::Render();
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::HideCursor()
{
  static char blankBits[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00 };

  static XColor black = { 0, 0, 0, 0, 0, 0 };

  if (!this->DisplayId || !this->WindowId)
  {
    this->CursorHidden = 1;
  }
  else if (!this->CursorHidden)
  {
    Pixmap blankPixmap = XCreateBitmapFromData(this->DisplayId, this->WindowId, blankBits, 16, 16);

    Cursor blankCursor =
      XCreatePixmapCursor(this->DisplayId, blankPixmap, blankPixmap, &black, &black, 7, 7);

    XDefineCursor(this->DisplayId, this->WindowId, blankCursor);

    XFreePixmap(this->DisplayId, blankPixmap);

    this->CursorHidden = 1;
  }
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::ShowCursor()
{
  if (!this->DisplayId || !this->WindowId)
  {
    this->CursorHidden = 0;
  }
  else if (this->CursorHidden)
  {
    XUndefineCursor(this->DisplayId, this->WindowId);
    this->CursorHidden = 0;
  }
}

//------------------------------------------------------------------------------------------------
// This probably has been moved to superclass.
void* vtkXWebGPURenderWindow::GetGenericWindowId()
{
  return reinterpret_cast<void*>(this->WindowId);
}

//------------------------------------------------------------------------------------------------
void vtkXWebGPURenderWindow::SetCurrentCursor(int shape)
{
  if (this->InvokeEvent(vtkCommand::CursorChangedEvent, &shape))
  {
    return;
  }
  this->Superclass::SetCurrentCursor(shape);
  if (!this->DisplayId || !this->WindowId)
  {
    return;
  }

  if (shape == VTK_CURSOR_DEFAULT)
  {
    XUndefineCursor(this->DisplayId, this->WindowId);
    return;
  }

  switch (shape)
  {
    case VTK_CURSOR_CROSSHAIR:
      if (!this->XCCrosshair)
      {
        this->XCCrosshair = XCreateFontCursor(this->DisplayId, XC_crosshair);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCCrosshair);
      break;
    case VTK_CURSOR_ARROW:
      if (!this->XCArrow)
      {
        this->XCArrow = XCreateFontCursor(this->DisplayId, XC_top_left_arrow);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCArrow);
      break;
    case VTK_CURSOR_SIZEALL:
      if (!this->XCSizeAll)
      {
        this->XCSizeAll = XCreateFontCursor(this->DisplayId, XC_fleur);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCSizeAll);
      break;
    case VTK_CURSOR_SIZENS:
      if (!this->XCSizeNS)
      {
        this->XCSizeNS = XCreateFontCursor(this->DisplayId, XC_sb_v_double_arrow);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCSizeNS);
      break;
    case VTK_CURSOR_SIZEWE:
      if (!this->XCSizeWE)
      {
        this->XCSizeWE = XCreateFontCursor(this->DisplayId, XC_sb_h_double_arrow);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCSizeWE);
      break;
    case VTK_CURSOR_SIZENE:
      if (!this->XCSizeNE)
      {
        this->XCSizeNE = XCreateFontCursor(this->DisplayId, XC_top_right_corner);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCSizeNE);
      break;
    case VTK_CURSOR_SIZENW:
      if (!this->XCSizeNW)
      {
        this->XCSizeNW = XCreateFontCursor(this->DisplayId, XC_top_left_corner);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCSizeNW);
      break;
    case VTK_CURSOR_SIZESE:
      if (!this->XCSizeSE)
      {
        this->XCSizeSE = XCreateFontCursor(this->DisplayId, XC_bottom_right_corner);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCSizeSE);
      break;
    case VTK_CURSOR_SIZESW:
      if (!this->XCSizeSW)
      {
        this->XCSizeSW = XCreateFontCursor(this->DisplayId, XC_bottom_left_corner);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCSizeSW);
      break;
    case VTK_CURSOR_HAND:
      if (!this->XCHand)
      {
        this->XCHand = XCreateFontCursor(this->DisplayId, XC_hand1);
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCHand);
      break;
    case VTK_CURSOR_CUSTOM:
#if VTK_HAVE_XCURSOR
      this->XCCustom = XcursorFilenameLoadCursor(this->DisplayId, this->GetCursorFileName());
      if (!this->XCCustom)
      {
        vtkErrorMacro(<< "Failed to load cursor from Xcursor file: " << this->GetCursorFileName());
        break;
      }
      XDefineCursor(this->DisplayId, this->WindowId, this->XCCustom);
#else
    {
      static bool once = false;
      if (!once)
      {
        once = true;
        vtkWarningMacro("VTK built without Xcursor support; ignoring requests for custom cursors.");
      }
    }
#endif
      break;
  }
}

//------------------------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_END
