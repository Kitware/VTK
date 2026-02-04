// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// vtk includes
#include "vtkXlibHardwareWindow.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkStringScanner.h"
#include "vtksys/SystemTools.hxx"

// STL includes
#include <assert.h>
#include <string_view>

VTK_ABI_NAMESPACE_BEGIN
//-------------------------------------------------------------------------------------------------
/*******************************************************************************
 * Motif style hint definitions
 *
 * The definitions in this section are taken from here:
 *
 *     https://sources.debian.org/src/motif/2.3.4-6+deb8u1/lib/Xm/MwmUtil.h/
 *
 * These are likely to be supported as long as xlib is, and the extended
 * window manager hints documented at freedesktop.org don't seem to have a
 * good alternative:
 *
 *     https://specifications.freedesktop.org/wm-spec/latest/ar01s05.html#id-1.6.7
 *
 * The _NET_WM_WINDOW_TYPE_SPLASH window type mentioned there comes close, but
 * does not result in task bar entries that can be used to bring the windows
 * to the front.
 */
typedef struct
{
  int flags;
  int functions;
  int decorations;
  int input_mode;
  int status;
} MotifWmHints;

typedef MotifWmHints MwmHints;

/* bit definitions for MwmHints.flags */
#define MWM_HINTS_FUNCTIONS (1L << 0)
#define MWM_HINTS_DECORATIONS (1L << 1)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL (1L << 0)

/* number of elements of size 32 in _MWM_HINTS */
#define PROP_MOTIF_WM_HINTS_ELEMENTS 5
#define PROP_MWM_HINTS_ELEMENTS PROP_MOTIF_WM_HINTS_ELEMENTS

/* atom name for _MWM_HINTS property */
#define _XA_MOTIF_WM_HINTS "_MOTIF_WM_HINTS"
#define _XA_MWM_HINTS _XA_MOTIF_WM_HINTS
/*
 * Motif style hint definitions
 ******************************************************************************/

//-------------------------------------------------------------------------------------------------
template <int EventType>
int XEventTypeEquals(Display*, XEvent* event, XPointer winptr)
{
  return (event->type == EventType &&
    *(reinterpret_cast<Window*>(winptr)) == reinterpret_cast<XAnyEvent*>(event)->window);
}

//-------------------------------------------------------------------------------------------------
int vtkXEventErrorHandler(Display* dpy, XErrorEvent* error)
{
  char error_text[256];
  vtkXGetErrorText(dpy, error->error_code, error_text, sizeof(error_text));
  vtkGenericWarningMacro("X Error: " << error_text
                                     << " request_code: " << static_cast<int>(error->request_code)
                                     << " minor_code: " << static_cast<int>(error->minor_code)
                                     << " resourceid: " << error->resourceid);
  return 0;
}

//-------------------------------------------------------------------------------------------------
vtkStandardNewMacro(vtkXlibHardwareWindow);

//-------------------------------------------------------------------------------------------------
vtkXlibHardwareWindow::vtkXlibHardwareWindow()
{
  this->ParentId = static_cast<Window>(0);
  this->OwnDisplay = 0;
  this->CursorHidden = 0;
  this->DisplayId = static_cast<Display*>(nullptr);
  this->WindowId = static_cast<Window>(0);
  this->ColorMap = static_cast<Colormap>(0);
  this->OwnWindow = 0;
  this->Platform = "X11";

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

  vtkX11FunctionsInitialize();
}

//-------------------------------------------------------------------------------------------------
vtkXlibHardwareWindow::~vtkXlibHardwareWindow()
{
  if (this->WindowId && this->DisplayId && (this->OwnDisplay || this->OwnWindow))
  {
    this->Destroy();
  }

  vtkX11FunctionsFinalize();
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorMap: " << this->ColorMap << "\n";
  os << indent << "OwnWindow: " << (this->OwnWindow ? "Y" : "N") << "\n";
  os << indent << "OwnDisplay: " << (this->OwnDisplay ? "Y" : "N") << "\n";
}

//-------------------------------------------------------------------------------------------------
Display* vtkXlibHardwareWindow::GetDisplayId()
{
  return this->DisplayId;
}

//-------------------------------------------------------------------------------------------------
Window vtkXlibHardwareWindow::GetWindowId()
{
  return this->WindowId;
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::SetDisplayId(void* arg)
{
  this->DisplayId = (Display*)(arg);
}

//------------------------------------------------------------------------------------------------
// Set this HardwareWindow's X window id to a pre-existing window.
void vtkXlibHardwareWindow::SetWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << reinterpret_cast<void*>(arg) << "\n");
  this->WindowId = arg;

  if (this->CursorHidden)
  {
    this->CursorHidden = 0;
    this->HideCursor();
  }
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::SetWindowId(void* arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << reinterpret_cast<void*>(arg) << "\n");
  this->WindowId = reinterpret_cast<Window>(arg);
  if (this->CursorHidden)
  {
    this->CursorHidden = 0;
    this->HideCursor();
  }
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::SetParentId(void* arg)
{
  vtkDebugMacro(<< "Setting ParentId to " << reinterpret_cast<void*>(arg) << "\n");
  this->ParentId = reinterpret_cast<Window>(arg);
}

//-------------------------------------------------------------------------------------------------
void* vtkXlibHardwareWindow::GetGenericDisplayId()
{
  return reinterpret_cast<void*>(this->DisplayId);
}

//-------------------------------------------------------------------------------------------------
void* vtkXlibHardwareWindow::GetGenericWindowId()
{
  return reinterpret_cast<void*>(this->WindowId);
}

//-------------------------------------------------------------------------------------------------
void* vtkXlibHardwareWindow::GetGenericParentId()
{
  return reinterpret_cast<void*>(this->ParentId);
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::Create()
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
    xsh.x = this->Position[0];
    xsh.y = this->Position[1];
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
    abort();
  }

  attr.override_redirect = False;
  if (this->Borders == 0.0 && !this->Coverable)
  {
    // Removes borders, and makes the window appear on top of all other windows
    attr.override_redirect = True;
  }

  // create our own window ?
  this->OwnWindow = false;
  if (!this->WindowId)
  {
    v = this->GetDesiredVisualInfo();
    if (!v)
    {
      vtkErrorMacro(<< "Could not find a decent visual\n");
      abort();
    }
    this->ColorMap = vtkXCreateColormap(
      this->DisplayId, vtkXRootWindow(this->DisplayId, v->screen), v->visual, AllocNone);

    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = this->ColorMap;
    attr.event_mask = StructureNotifyMask | ExposureMask;

    // get a default parent if one has not been set.
    if (!this->ParentId)
    {
      this->ParentId = vtkXRootWindow(this->DisplayId, v->screen);
    }
    this->WindowId =
      vtkXCreateWindow(this->DisplayId, this->ParentId, x, y, static_cast<unsigned int>(width),
        static_cast<unsigned int>(height), 0, v->depth, InputOutput, v->visual,
        CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect | CWEventMask, &attr);
    if (this->Borders == 0.0 && this->Coverable)
    {
      // Removes borders, while still allowing other windows on top
      Atom mwmHintsProperty = vtkXInternAtom(this->DisplayId, _XA_MWM_HINTS, 0);
      MotifWmHints mwmHints;
      mwmHints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
      mwmHints.functions = MWM_FUNC_ALL;
      mwmHints.decorations = 0;
      vtkXChangeProperty(this->DisplayId, this->WindowId, mwmHintsProperty, XA_ATOM, 32,
        PropModeReplace, reinterpret_cast<unsigned char*>(&mwmHints), PROP_MWM_HINTS_ELEMENTS);
    }

    vtkXStoreName(this->DisplayId, this->WindowId, this->WindowName);
    vtkXSetNormalHints(this->DisplayId, this->WindowId, &xsh);

    char classStr[4] = "Vtk";
    char nameStr[4] = "vtk";
    xch.res_class = classStr;
    xch.res_name = nameStr;
    vtkXSetClassHint(this->DisplayId, this->WindowId, &xch);

    this->OwnWindow = true;
  }
  else
  {
    vtkXChangeWindowAttributes(this->DisplayId, this->WindowId, CWOverrideRedirect, &attr);
    vtkXGetWindowAttributes(this->DisplayId, this->WindowId, &winattr);
    matcher.visualid = vtkXVisualIDFromVisual(winattr.visual);
    matcher.screen = vtkXDefaultScreen(DisplayId);
    v = vtkXGetVisualInfo(this->DisplayId, VisualIDMask | VisualScreenMask, &matcher, &nItems);
  }

  if (this->OwnWindow)
  {
    // RESIZE THE WINDOW TO THE DESIRED SIZE
    vtkDebugMacro(<< "Resizing the xwindow\n");
    vtkXResizeWindow(this->DisplayId, this->WindowId,
      ((this->Size[0] > 0) ? static_cast<unsigned int>(this->Size[0]) : 300),
      ((this->Size[1] > 0) ? static_cast<unsigned int>(this->Size[1]) : 300));
    vtkXSync(this->DisplayId, False);
  }

  if (this->OwnWindow && this->ShowWindow)
  {
    vtkDebugMacro(" Mapping the xwindow\n");
    vtkXMapWindow(this->DisplayId, this->WindowId);
    vtkXSync(this->DisplayId, False);
    XEvent e;
    vtkXIfEvent(this->DisplayId, &e, XEventTypeEquals<MapNotify>,
      reinterpret_cast<XPointer>(&this->WindowId));
    vtkXGetWindowAttributes(this->DisplayId, this->WindowId, &winattr);
    // if the specified window size is bigger than the screen size,
    // we have to reset the window size to the screen size
    width = winattr.width;
    height = winattr.height;
    this->Mapped = 1;

    if (this->FullScreen)
    {
      vtkXGrabKeyboard(
        this->DisplayId, this->WindowId, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    }
  }
  this->Size[0] = width;
  this->Size[1] = height;
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::Destroy()
{
  if (this->DisplayId && this->WindowId)
  {
    // we will only have a cursor defined if a CurrentCursor has been
    // set > 0 or if the cursor has been hidden... if we undefine without
    // checking, bad things can happen (BadWindow)
    if (this->GetCurrentCursor() || this->CursorHidden)
    {
      vtkXUndefineCursor(this->DisplayId, this->WindowId);
    }
    if (this->XCArrow)
    {
      vtkXFreeCursor(this->DisplayId, this->XCArrow);
    }
    if (this->XCCrosshair)
    {
      vtkXFreeCursor(this->DisplayId, this->XCCrosshair);
    }
    if (this->XCSizeAll)
    {
      vtkXFreeCursor(this->DisplayId, this->XCSizeAll);
    }
    if (this->XCSizeNS)
    {
      vtkXFreeCursor(this->DisplayId, this->XCSizeNS);
    }
    if (this->XCSizeWE)
    {
      vtkXFreeCursor(this->DisplayId, this->XCSizeWE);
    }
    if (this->XCSizeNE)
    {
      vtkXFreeCursor(this->DisplayId, this->XCSizeNE);
    }
    if (this->XCSizeNW)
    {
      vtkXFreeCursor(this->DisplayId, this->XCSizeNW);
    }
    if (this->XCSizeSE)
    {
      vtkXFreeCursor(this->DisplayId, this->XCSizeSE);
    }
    if (this->XCSizeSW)
    {
      vtkXFreeCursor(this->DisplayId, this->XCSizeSW);
    }
    if (this->XCHand)
    {
      vtkXFreeCursor(this->DisplayId, this->XCHand);
    }
    if (this->XCCustom)
    {
      vtkXFreeCursor(this->DisplayId, this->XCCustom);
    }
    if (this->OwnWindow)
    {
      vtkXDestroyWindow(this->DisplayId, this->WindowId);
      this->WindowId = static_cast<Window>(0);
    }
    else
    {
      // if we don't own it, simply unmap the window
      vtkXUnmapWindow(this->DisplayId, this->WindowId);
    }
    this->Mapped = 0;
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

  this->CloseDisplay();

  // make sure all other code knows we're not mapped anymore
  this->Mapped = 0;
}

//-------------------------------------------------------------------------------------------------
// Specify the size of the rendering window.
void vtkXlibHardwareWindow::SetSize(int width, int height)
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
      vtkXResizeWindow(this->DisplayId, this->WindowId, static_cast<unsigned int>(width),
        static_cast<unsigned int>(height));

      // flush output queue and wait for X server to processes the request
      vtkXSync(this->DisplayId, False);

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
      vtkXGetWindowAttributes(this->DisplayId, this->WindowId, &attrs);
      if (!attrs.override_redirect && this->ParentId)
      {
        // check if parent has SubstructureRedirectMask
        XWindowAttributes parentAttrs;
        vtkXGetWindowAttributes(this->DisplayId, this->ParentId, &parentAttrs);
        if ((parentAttrs.all_event_masks & SubstructureRedirectMask) == SubstructureRedirectMask)
        {
          // set the wait timeout to be 2 seconds from now
          double maxtime = 2.0 + vtksys::SystemTools::GetTime();
          // look for a ConfigureNotify that came *after* XResizeWindow
          XEvent e;
          while (!vtkXCheckIfEvent(this->DisplayId, &e, XEventTypeEquals<ConfigureNotify>,
                   reinterpret_cast<XPointer>(&this->WindowId)) ||
            e.xconfigure.serial < serial)
          {
            // wait for 10 milliseconds and try again until time runs out
            vtksys::SystemTools::Delay(10);
            if (vtksys::SystemTools::GetTime() > maxtime)
            {
              vtkWarningMacro(<< "Timeout while waiting for response to vtkXResizeWindow.");
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
// Move the window to a new position on the display.
void vtkXlibHardwareWindow::SetPosition(int x, int y)
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

  vtkXMoveWindow(this->DisplayId, this->WindowId, x, y);
  vtkXSync(this->DisplayId, False);
}

//-------------------------------------------------------------------------------------------------
vtkXVisualInfo* vtkXlibHardwareWindow::GetDesiredVisualInfo()
{
  static XVisualInfo vinfo;
  if (!this->EnsureDisplay())
  {
    return nullptr;
  }

  int screenId = vtkXDefaultScreen(this->DisplayId);
  int nitems = 0;
  XVisualInfo vinfo_template = {};
  vinfo_template.screen = screenId;
  XVisualInfo* v = vtkXGetVisualInfo(this->DisplayId, VisualScreenMask, &vinfo_template, &nitems);

  bool haveVisual = false;
  // Accept either a TrueColor or DirectColor visual at any multiple-of-8 depth.
  for (int depth = 24; depth > 0 && !haveVisual; depth -= 8)
  {
    if (vtkXMatchVisualInfo(this->DisplayId, screenId, depth, /*class*/ TrueColor, &vinfo))
    {
      haveVisual = true;
      break;
    }
    if (vtkXMatchVisualInfo(this->DisplayId, screenId, depth, /*class*/ DirectColor, &vinfo))
    {
      haveVisual = true;
      break;
    }
  }
  if (v)
  {
    vtkXFree(v);
  }
  return haveVisual ? reinterpret_cast<vtkXVisualInfo*>(&vinfo) : nullptr;
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::CloseDisplay()
{
  // if we create the display, we'll delete it
  if (this->OwnDisplay && this->DisplayId)
  {
    XCloseDisplay(this->DisplayId);
    this->DisplayId = nullptr;
    this->OwnDisplay = false;
  }
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::HideCursor()
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

    vtkXDefineCursor(this->DisplayId, this->WindowId, blankCursor);

    vtkXFreePixmap(this->DisplayId, blankPixmap);

    this->CursorHidden = 1;
  }
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::ShowCursor()
{
  if (!this->DisplayId || !this->WindowId)
  {
    this->CursorHidden = 0;
  }
  else if (this->CursorHidden)
  {
    vtkXUndefineCursor(this->DisplayId, this->WindowId);
    this->CursorHidden = 0;
  }
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::SetCurrentCursor(int shape)
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
    vtkXUndefineCursor(this->DisplayId, this->WindowId);
    return;
  }

  switch (shape)
  {
    case VTK_CURSOR_CROSSHAIR:
      if (!this->XCCrosshair)
      {
        this->XCCrosshair = XCreateFontCursor(this->DisplayId, XC_crosshair);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCCrosshair);
      break;
    case VTK_CURSOR_ARROW:
      if (!this->XCArrow)
      {
        this->XCArrow = XCreateFontCursor(this->DisplayId, XC_top_left_arrow);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCArrow);
      break;
    case VTK_CURSOR_SIZEALL:
      if (!this->XCSizeAll)
      {
        this->XCSizeAll = XCreateFontCursor(this->DisplayId, XC_fleur);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCSizeAll);
      break;
    case VTK_CURSOR_SIZENS:
      if (!this->XCSizeNS)
      {
        this->XCSizeNS = XCreateFontCursor(this->DisplayId, XC_sb_v_double_arrow);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCSizeNS);
      break;
    case VTK_CURSOR_SIZEWE:
      if (!this->XCSizeWE)
      {
        this->XCSizeWE = XCreateFontCursor(this->DisplayId, XC_sb_h_double_arrow);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCSizeWE);
      break;
    case VTK_CURSOR_SIZENE:
      if (!this->XCSizeNE)
      {
        this->XCSizeNE = XCreateFontCursor(this->DisplayId, XC_top_right_corner);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCSizeNE);
      break;
    case VTK_CURSOR_SIZENW:
      if (!this->XCSizeNW)
      {
        this->XCSizeNW = XCreateFontCursor(this->DisplayId, XC_top_left_corner);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCSizeNW);
      break;
    case VTK_CURSOR_SIZESE:
      if (!this->XCSizeSE)
      {
        this->XCSizeSE = XCreateFontCursor(this->DisplayId, XC_bottom_right_corner);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCSizeSE);
      break;
    case VTK_CURSOR_SIZESW:
      if (!this->XCSizeSW)
      {
        this->XCSizeSW = XCreateFontCursor(this->DisplayId, XC_bottom_left_corner);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCSizeSW);
      break;
    case VTK_CURSOR_HAND:
      if (!this->XCHand)
      {
        this->XCHand = XCreateFontCursor(this->DisplayId, XC_hand1);
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCHand);
      break;
    case VTK_CURSOR_CUSTOM:
#if VTK_HAVE_XCURSOR
      this->XCCustom = vtkXcursorFilenameLoadCursor(this->DisplayId, this->GetCursorFileName());
      if (!this->XCCustom)
      {
        vtkErrorMacro(<< "Failed to load cursor from vtkXcursor file: "
                      << this->GetCursorFileName());
        break;
      }
      vtkXDefineCursor(this->DisplayId, this->WindowId, this->XCCustom);
#else
    {
      static bool once = false;
      if (!once)
      {
        once = true;
        vtkWarningMacro(
          "VTK built without vtkXcursor support; ignoring requests for custom cursors.");
      }
    }
#endif
      break;
  }
}

//------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::SetWindowName(const char* cname)
{
  char* name = new char[strlen(cname) + 1];
  strcpy(name, cname);
  XTextProperty win_name_text_prop;

  this->Superclass::SetWindowName(name);

  if (this->WindowId)
  {
    if (vtkXStringListToTextProperty(&name, 1, &win_name_text_prop) == 0)
    {
      vtkXFree(win_name_text_prop.value);
      vtkWarningMacro(<< "Can't rename window");
      delete[] name;
      return;
    }

    vtkXSetWMName(this->DisplayId, this->WindowId, &win_name_text_prop);
    vtkXSetWMIconName(this->DisplayId, this->WindowId, &win_name_text_prop);
    vtkXFree(win_name_text_prop.value);
  }
  delete[] name;
}

//------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::SetIcon(vtkImageData* img)
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

  Atom iconAtom = vtkXInternAtom(this->DisplayId, "_NET_WM_ICON", False);
  Atom typeAtom = vtkXInternAtom(this->DisplayId, "CARDINAL", False);
  vtkXChangeProperty(this->DisplayId, this->WindowId, iconAtom, typeAtom, 32, PropModeReplace,
    reinterpret_cast<unsigned char*>(pixels.data()), pixels.size());
}

//------------------------------------------------------------------------------------------------
// Set this HardwareWindow's X window id to a pre-existing window.
void vtkXlibHardwareWindow::SetWindowInfo(const char* info)
{
  int tmp;

  // get the default display connection
  if (!this->DisplayId)
  {
    this->DisplayId = vtkXOpenDisplay(static_cast<char*>(nullptr));
    if (this->DisplayId == nullptr)
    {
      vtkErrorMacro(<< "bad X server connection. DISPLAY=" << vtksys::SystemTools::GetEnv("DISPLAY")
                    << ". Aborting.\n");
      abort();
    }
    else
    {
      this->OwnDisplay = 1;
    }
  }

  tmp = vtk::scan_int<int>(std::string_view(info))->value();

  this->SetWindowId(static_cast<Window>(tmp));
}

//-------------------------------------------------------------------------------------------------
void vtkXlibHardwareWindow::SetCoverable(vtkTypeBool coverable)
{
  if (this->Coverable != coverable)
  {
    this->Coverable = coverable;
    this->Modified();
  }
}

//------------------------------------------------------------------------------------------------
bool vtkXlibHardwareWindow::EnsureDisplay()
{
  if (!this->DisplayId)
  {
    XInitThreads();
    this->DisplayId = vtkXOpenDisplay(static_cast<char*>(nullptr));
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
VTK_ABI_NAMESPACE_END
