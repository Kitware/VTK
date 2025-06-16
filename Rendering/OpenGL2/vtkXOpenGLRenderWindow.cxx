// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Must be included first to avoid conflicts with X11's `Status` define.
#include "vtksys/SystemTools.hxx"

#include "vtkOpenGLRenderer.h"
#include "vtkXOpenGLRenderWindow.h"

#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexBufferObjectCache.h"
#include "vtkRenderTimerLog.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkStringOutputWindow.h"

#include <sstream>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#if VTK_HAVE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "vtk_glad.h"
#include "vtkglad/include/glad/glx.h"

/*
 * Work-around to get forward declarations of C typedef of anonymous
 * structs working. We do not want to include XUtil.h in the header as
 * it populates the global namespace.
 */
VTK_ABI_NAMESPACE_BEGIN
struct vtkXVisualInfo : public XVisualInfo
{
};

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

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(
  Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef void (*glXSwapIntervalEXTProc)(Display* dpy, GLXDrawable drawable, int interval);

class vtkXOpenGLRenderWindow;
class vtkRenderWindow;
class vtkXOpenGLRenderWindowInternal
{
  friend class vtkXOpenGLRenderWindow;

private:
  vtkXOpenGLRenderWindowInternal(vtkRenderWindow*);

  GLXContext ContextId;
  GLXFBConfig FBConfig;
};

vtkXOpenGLRenderWindowInternal::vtkXOpenGLRenderWindowInternal(vtkRenderWindow*)
{
  this->ContextId = nullptr;
  this->FBConfig = None;
}

vtkStandardNewMacro(vtkXOpenGLRenderWindow);

#define MAX_LIGHTS 8

namespace
{
GLXFBConfig vtkXOpenGLRenderWindowTryForFBConfig(Display* DisplayId, int drawable_type,
  vtkTypeBool doublebuff, vtkTypeBool stereo, vtkTypeBool stencil, bool srgb)
{
  int index;
  static int attributes[50];

  // setup the default stuff we ask for
  index = 0;
  attributes[index++] = GLX_DRAWABLE_TYPE;
  attributes[index++] = drawable_type;
  attributes[index++] = GLX_RENDER_TYPE;
  attributes[index++] = GLX_RGBA_BIT;
  attributes[index++] = GLX_RED_SIZE;
  attributes[index++] = 1;
  attributes[index++] = GLX_GREEN_SIZE;
  attributes[index++] = 1;
  attributes[index++] = GLX_BLUE_SIZE;
  attributes[index++] = 1;
  attributes[index++] = GLX_DEPTH_SIZE;
  attributes[index++] = 1;
  attributes[index++] = GLX_ALPHA_SIZE;
  attributes[index++] = 1;
  if (doublebuff)
  {
    attributes[index++] = GLX_DOUBLEBUFFER;
    attributes[index++] = True;
  }
  if (stencil)
  {
    attributes[index++] = GLX_STENCIL_SIZE;
    attributes[index++] = 8;
  }
  if (stereo)
  {
    // also try for STEREO
    attributes[index++] = GLX_STEREO;
    attributes[index++] = True;
  }

  if (srgb)
  {
    attributes[index++] = 0x20B2;
    attributes[index++] = True;
  }

  attributes[index++] = None;

  // cout << "Trying config: " << endl
  //      << "         DisplayId : " << DisplayId << endl
  //      << "     drawable_type : " << drawable_type << endl
  //      << "        doublebuff : " << doublebuff << endl
  //      << "            stereo : " << stereo << endl
  //      << "    alphaBitPlanes : " << alphaBitPlanes << endl
  //      << "           stencil : " << stencil << endl;
  int tmp;
  GLXFBConfig* fb = glXChooseFBConfig(DisplayId, XDefaultScreen(DisplayId), attributes, &tmp);
  if (fb && tmp > 0)
  {
    // cout << "            STATUS : SUCCESS!!!" << endl;
    GLXFBConfig result = fb[0];
    XFree(fb);
    return result;
  }
  // cout << "            STATUS : FAILURE!!!" << endl;
  return None;
}

GLXFBConfig vtkXOpenGLRenderWindowGetDesiredFBConfig(Display* DisplayId, vtkTypeBool& win_stereo,
  vtkTypeBool& win_doublebuffer, int drawable_type, vtkTypeBool& stencil, bool srgb)
{
  GLXFBConfig fbc = None;
  int stereo = 0;

  // try every possibility stopping when we find one that works
  // start by adjusting stereo
  for (stereo = win_stereo; !fbc && stereo >= 0; stereo--)
  {
    fbc = vtkXOpenGLRenderWindowTryForFBConfig(
      DisplayId, drawable_type, win_doublebuffer, stereo, stencil, srgb);
    if (fbc)
    {
      // found a valid config
      win_stereo = stereo;
      return fbc;
    }
  }

  // OK adjusting stereo did not work
  // try flipping the double buffer requirement and try again
  for (stereo = win_stereo; !fbc && stereo >= 0; stereo--)
  {
    fbc = vtkXOpenGLRenderWindowTryForFBConfig(
      DisplayId, drawable_type, !win_doublebuffer, stereo, stencil, srgb);
    // we found a valid result
    if (fbc)
    {
      win_doublebuffer = !win_doublebuffer;
      win_stereo = stereo;
      return fbc;
    }
  }

  // we failed
  return (None);
}

template <int EventType>
int XEventTypeEquals(Display*, XEvent* event, XPointer winptr)
{
  return (event->type == EventType &&
    *(reinterpret_cast<Window*>(winptr)) == reinterpret_cast<XAnyEvent*>(event)->window);
}

// to manage the static GLADGLXLibraryDisplay correctly.
std::atomic<unsigned int> vtkGLXLibraryDisplayCleanupCounter{ 0 };

/**
 * The glX symbols are globally defined in glad sources, see glad/src/glx.c.
 * These symbols must be initialized exactly once in the lifetime of an
 * application. Symbol reassigment would corrupt existing render
 * windows.
 *
 * This setup is created to overcome an inadvertent memory leak
 * when `gladLoaderLoadGLX(display, 0)` is used. The memory leak
 * occurs when the passed in display is nullptr.
 * glad internally opens a display, discovers GLX extensions
 * supported by the display.
 *
 * Hence, if the code simply called `gladLoaderLoadGLX(nullptr, 0)`,
 * the application will leak the `Display*`, worst case, one
 * of the glX functions will crash with an error message
 * "Maximum number of clients reached". This was a real issue
 * in VTK 9.4.0. See https://gitlab.kitware.com/vtk/vtk/-/issues/19618
 */
// static Display used for loading GLX extensions with GLAD.
std::atomic<bool> GLADGLXLibraryDisplayOpened{ false };
int GLXVersion = 0;
Display* GLADGLXLibraryDisplay = nullptr;
}

vtkXVisualInfo* vtkXOpenGLRenderWindow::GetDesiredVisualInfo()
{
  XVisualInfo* v = nullptr;

  // get the default display connection
  if (!this->EnsureDisplay())
  {
    return nullptr;
  }
  // load GLX
  if (!this->EnsureGLX())
  {
    return nullptr;
  }

  this->Internal->FBConfig =
    vtkXOpenGLRenderWindowGetDesiredFBConfig(this->DisplayId, this->StereoCapableWindow,
      this->DoubleBuffer, GLX_WINDOW_BIT, this->StencilCapable, this->UseSRGBColorSpace);

  if (!this->Internal->FBConfig)
  {
    vtkErrorMacro(<< "Could not find a decent config\n");
  }
  else
  {
    v = glXGetVisualFromFBConfig(this->DisplayId, this->Internal->FBConfig);
    if (!v)
    {
      vtkErrorMacro(<< "Could not find a decent visual\n");
    }
  }
  return reinterpret_cast<vtkXVisualInfo*>(v);
}

vtkXOpenGLRenderWindow::vtkXOpenGLRenderWindow()
{
  this->ParentId = static_cast<Window>(0);
  this->OwnDisplay = 0;
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->UsingHardware = 0;
  this->DisplayId = static_cast<Display*>(nullptr);
  this->WindowId = static_cast<Window>(0);
  this->NextWindowId = static_cast<Window>(0);
  this->ColorMap = static_cast<Colormap>(0);
  this->OwnWindow = 0;

  this->Internal = new vtkXOpenGLRenderWindowInternal(this);

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

  auto loadFunc = [](void*, const char* name) -> VTKOpenGLAPIProc
  {
    if (name)
    {
      return glXGetProcAddress((const GLubyte*)name);
    }
    return nullptr;
  };
  this->SetOpenGLSymbolLoader(loadFunc, nullptr);
  // Increment static counter
  ++::vtkGLXLibraryDisplayCleanupCounter;
}

// free up memory & close the window
vtkXOpenGLRenderWindow::~vtkXOpenGLRenderWindow()
{
  // close-down all system-specific drawing resources
  this->Finalize();

  vtkRenderer* ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    ren->SetRenderWindow(nullptr);
  }
  if (--::vtkGLXLibraryDisplayCleanupCounter == 0)
  {
    if (::GLADGLXLibraryDisplay != nullptr)
    {
      XCloseDisplay(::GLADGLXLibraryDisplay);
      GLADGLXLibraryDisplayOpened = false;
    }
  }

  delete this->Internal;
}

// End the rendering process and display the image.
void vtkXOpenGLRenderWindow::Frame()
{
  if (!this->EnsureGLX())
  {
    return;
  }
  this->MakeCurrent();
  this->Superclass::Frame();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers && this->WindowId != 0)
  {
    this->RenderTimer->MarkStartEvent("glXSwapBuffers (may stall for VSync)");
    glXSwapBuffers(this->DisplayId, this->WindowId);
    this->RenderTimer->MarkEndEvent();

    vtkDebugMacro(<< " glXSwapBuffers\n");
  }
}

bool vtkXOpenGLRenderWindow::InitializeFromCurrentContext()
{
  GLXContext currentContext = glXGetCurrentContext();
  if (currentContext != nullptr)
  {
    this->SetDisplayId((void*)glXGetCurrentDisplay());
    this->SetWindowId((void*)glXGetCurrentDrawable());
    this->Internal->ContextId = currentContext;
    return this->Superclass::InitializeFromCurrentContext();
  }
  return false;
}

void vtkXOpenGLRenderWindow::SetCoverable(vtkTypeBool coverable)
{
  if (this->Coverable != coverable)
  {
    this->Coverable = coverable;
    this->Modified();
  }
}

//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkXOpenGLRenderWindow::SetStereoCapableWindow(vtkTypeBool capable)
{
  if (!this->Internal->ContextId)
  {
    vtkOpenGLRenderWindow::SetStereoCapableWindow(capable);
  }
  else
  {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
  }
}

static int PbufferAllocFail = 0;
#define vtkXOGLPbufferErrorHandler VTK_ABI_NAMESPACE_MANGLE(vtkXOGLPbufferErrorHandler)
extern "C"
{
  int VTK_ABI_NAMESPACE_MANGLE(vtkXOGLPbufferErrorHandler)(Display*, XErrorEvent*)
  {
    PbufferAllocFail = 1;
    return 1;
  }
}

static bool ctxErrorOccurred = false;
#define vtkXOGLContextCreationErrorHandler                                                         \
  VTK_ABI_NAMESPACE_MANGLE(vtkXOGLContextCreationErrorHandler)
extern "C"
{
  int VTK_ABI_NAMESPACE_MANGLE(vtkXOGLContextCreationErrorHandler)(Display*, XErrorEvent*)
  {
    ctxErrorOccurred = true;
    return 1;
  }
}

void vtkXOpenGLRenderWindow::SetShowWindow(bool val)
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

void vtkXOpenGLRenderWindow::CreateAWindow()
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

  // load GLX
  if (!this->EnsureGLX())
  {
    return;
  }

  // get the default display connection
  if (!this->EnsureDisplay())
  {
    return;
  }

  attr.override_redirect = False;
  if (this->Borders == 0.0 && !this->Coverable)
  {
    // Removes borders, and makes the window appear on top of all other windows
    attr.override_redirect = True;
  }

  // create our own window ?
  this->OwnWindow = 0;
  if (!this->WindowId)
  {
    v = this->GetDesiredVisualInfo();
    if (!v)
    {
      vtkWarningMacro(<< "Could not find a decent visual\n");
      return;
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
      this->ParentId = XRootWindow(this->DisplayId, v->screen);
    }
    this->WindowId =
      XCreateWindow(this->DisplayId, this->ParentId, x, y, static_cast<unsigned int>(width),
        static_cast<unsigned int>(height), 0, v->depth, InputOutput, v->visual,
        CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect | CWEventMask, &attr);

    if (this->Borders == 0.0 && this->Coverable)
    {
      // Removes borders, while still allowing other windows on top
      Atom mwmHintsProperty = XInternAtom(this->DisplayId, _XA_MWM_HINTS, 0);
      MotifWmHints mwmHints;
      mwmHints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
      mwmHints.functions = MWM_FUNC_ALL;
      mwmHints.decorations = 0;
      XChangeProperty(this->DisplayId, this->WindowId, mwmHintsProperty, XA_ATOM, 32,
        PropModeReplace, reinterpret_cast<unsigned char*>(&mwmHints), PROP_MWM_HINTS_ELEMENTS);
    }

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

    // if FBConfig is not set, try to find it based on the window
    if (!this->Internal->FBConfig)
    {
      int fbcount = 0;
      GLXFBConfig* fbc = glXGetFBConfigs(this->DisplayId, matcher.screen, &fbcount);
      if (fbc)
      {
        int i;
        for (i = 0; i < fbcount; ++i)
        {
          XVisualInfo* vi = glXGetVisualFromFBConfig(this->DisplayId, fbc[i]);
          if (vi && vi->visualid == matcher.visualid)
          {
            this->Internal->FBConfig = fbc[i];
          }
        }
        XFree(fbc);
      }
    }
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

  // is GLX extension is supported?
  if (!glXQueryExtension(this->DisplayId, nullptr, nullptr))
  {
    vtkWarningMacro("GLX not found.");
    if (this->HasObserver(vtkCommand::ExitEvent))
    {
      this->InvokeEvent(vtkCommand::ExitEvent, nullptr);
      return;
    }
    else
    {
      return;
    }
  }

  // try for 32 context
  if (this->Internal->FBConfig)
  {
    // NOTE: It is not necessary to create or make current to a context before
    // calling glXGetProcAddressARB
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB =
      (glXCreateContextAttribsARBProc)glXGetProcAddressARB(
        (const GLubyte*)"glXCreateContextAttribsARB");

    int context_attribs[] = { GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 2,
    // GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
#ifdef GL_ES_VERSION_3_0
      GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_ES_PROFILE_BIT_EXT,
#else
      GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
#endif
      0 };

    if (glXCreateContextAttribsARB)
    {
      // do we have a shared render window?
      GLXContext sharedContext = nullptr;
      vtkXOpenGLRenderWindow* renWin = nullptr;
      if (this->SharedRenderWindow)
      {
        renWin = vtkXOpenGLRenderWindow::SafeDownCast(this->SharedRenderWindow);
        if (renWin && renWin->Internal->ContextId)
        {
          sharedContext = renWin->Internal->ContextId;
        }
      }

      XErrorHandler previousHandler = XSetErrorHandler(vtkXOGLContextCreationErrorHandler);
      this->Internal->ContextId = nullptr;

      // we believe that these later versions are all compatible with
      // OpenGL 3.2 so get a more recent context if we can.
      // For GLES, version 3.0 is best supported by VTK shaders.
#ifdef GL_ES_VERSION_3_0
      int attemptedVersions[] = { 3, 0 };
#else
      int attemptedVersions[] = { 4, 5, 4, 4, 4, 3, 4, 2, 4, 1, 4, 0, 3, 3, 3, 2 };
#endif

      // try shared context first, the fallback to not shared
      bool done = false;
      while (!done)
      {
        for (int i = 0; i < 8 && !this->Internal->ContextId; i++)
        {
          context_attribs[1] = attemptedVersions[i * 2];
          context_attribs[3] = attemptedVersions[i * 2 + 1];
          this->Internal->ContextId = glXCreateContextAttribsARB(
            this->DisplayId, this->Internal->FBConfig, sharedContext, GL_TRUE, context_attribs);
          // Sync to ensure any errors generated are processed.
          XSync(this->DisplayId, False);
          if (ctxErrorOccurred)
          {
            this->Internal->ContextId = nullptr;
            ctxErrorOccurred = false;
          }
        }
        if (!this->Internal->ContextId && sharedContext)
        {
          sharedContext = nullptr;
        }
        else
        {
          done = true;
        }
      }
      XSetErrorHandler(previousHandler);
      if (this->Internal->ContextId)
      {
        if (sharedContext)
        {
          this->GetState()->SetVBOCache(renWin->GetState()->GetVBOCache());
        }
      }
    }
  }

  // old failsafe
  if (this->Internal->ContextId == nullptr)
  {
    // I suspect this will always return an unusable context
    // but leaving it in to be safe
    this->Internal->ContextId = glXCreateContext(this->DisplayId, v, nullptr, GL_TRUE);
  }

  if (!this->Internal->ContextId)
  {
    vtkWarningMacro("Cannot create GLX context.");
    if (this->HasObserver(vtkCommand::ExitEvent))
    {
      this->InvokeEvent(vtkCommand::ExitEvent, nullptr);
      return;
    }
    else
    {
      return;
    }
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

void vtkXOpenGLRenderWindow::DestroyWindow()
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

  if (this->OwnContext && this->Internal->ContextId)
  {
    this->MakeCurrent();
    this->ReleaseGraphicsResources(this);

    if (this->Internal->ContextId)
    {
      glFinish();
      // load GLX
      if (this->EnsureGLX())
      {
        glXDestroyContext(this->DisplayId, this->Internal->ContextId);
        glXMakeCurrent(this->DisplayId, None, nullptr);
      }
    }
  }
  else
  {
    // Assume the context is made current externally and release resources
    this->ReleaseGraphicsResources(this);
  }

  this->Internal->ContextId = nullptr;

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

// Initialize the window for rendering.
void vtkXOpenGLRenderWindow::WindowInitialize()
{
  this->CreateAWindow();
  if (!this->DisplayId || !this->WindowId)
  {
    return;
  }

  this->MakeCurrent();

  // tell our renderers about us
  vtkRenderer* ren;
  for (this->Renderers->InitTraversal(); (ren = this->Renderers->GetNextItem());)
  {
    ren->SetRenderWindow(nullptr);
    ren->SetRenderWindow(this);
  }

  this->OpenGLInit();
}

// Initialize the rendering window.
void vtkXOpenGLRenderWindow::Initialize()
{
  if (!this->Internal->ContextId)
  {
    // initialize the window
    this->WindowInitialize();
  }
}

void vtkXOpenGLRenderWindow::Finalize()
{
  if (!this->Initialized)
  {
    return;
  }
  // clean and destroy window
  this->DestroyWindow();
}

// Change the window to fill the entire screen.
void vtkXOpenGLRenderWindow::SetFullScreen(vtkTypeBool arg)
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

// Set the preferred window size to full screen.
void vtkXOpenGLRenderWindow::PrefFullScreen()
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

// Resize the window.
void vtkXOpenGLRenderWindow::WindowRemap()
{
  // shut everything down
  this->Finalize();

  // set the default windowid
  this->WindowId = this->NextWindowId;
  this->NextWindowId = static_cast<Window>(0);

  // set everything up again
  this->Initialize();
}

// Begin the rendering process.
void vtkXOpenGLRenderWindow::Start()
{
  this->Initialize();

  // When mixing on-screen render windows with offscreen render windows,
  // the active context state can easily get messed up. Ensuring that before we
  // start rendering we force making the context current is a reasonable
  // workaround for now.
  this->SetForceMakeCurrent();

  this->Superclass::Start();
}

// Specify the size of the rendering window.
void vtkXOpenGLRenderWindow::SetSize(int width, int height)
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
              return;
            }
          }
          XPutBackEvent(this->DisplayId, &e);
        }
      }
    }
  }
}

bool vtkXOpenGLRenderWindow::SetSwapControl(int i)
{
  // load GLX
  if (!this->EnsureGLX())
  {
    return false;
  }

  glXSwapIntervalEXTProc glXSwapIntervalEXT =
    (glXSwapIntervalEXTProc)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");

  if (!glXSwapIntervalEXT)
  {
    return false;
  }

  // if (i < 0)
  // {
  //   if (glxewIsSupported("GLX_EXT_swap_control_tear"))
  //   {
  //     glXSwapIntervalEXT(i);
  //     return true;
  //   }
  //   return false;
  // }

  glXSwapIntervalEXT(this->DisplayId, this->WindowId, i);
  return true;
}

int vtkXOpenGLRenderWindow::GetDesiredDepth()
{
  XVisualInfo* v;
  int depth = 0;

  // get the default visual to use
  v = this->GetDesiredVisualInfo();

  if (v)
  {
    depth = v->depth;
    XFree(v);
  }

  return depth;
}

// Get a visual from the windowing system.
Visual* vtkXOpenGLRenderWindow::GetDesiredVisual()
{
  XVisualInfo* v;
  Visual* vis = nullptr;

  // get the default visual to use
  v = this->GetDesiredVisualInfo();

  if (v)
  {
    vis = v->visual;
    XFree(v);
  }

  return vis;
}

// Get a colormap from the windowing system.
Colormap vtkXOpenGLRenderWindow::GetDesiredColormap()
{
  XVisualInfo* v;

  if (this->ColorMap)
    return this->ColorMap;

  // get the default visual to use
  v = this->GetDesiredVisualInfo();

  if (v)
  {
    this->ColorMap = XCreateColormap(
      this->DisplayId, XRootWindow(this->DisplayId, v->screen), v->visual, AllocNone);
    XFree(v);
  }

  return this->ColorMap;
}

void vtkXOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ContextId: " << this->Internal->ContextId << "\n";
  os << indent << "Color Map: " << this->ColorMap << "\n";
  os << indent << "Display Id: " << this->GetDisplayId() << "\n";
  os << indent << "Next Window Id: " << this->NextWindowId << "\n";
  os << indent << "Window Id: " << this->GetWindowId() << "\n";
}

// the following can be useful for debugging XErrors
// When uncommented (along with the lines in MakeCurrent)
// it will cause a segfault upon an XError instead of
// the normal XError handler
// extern "C" {int vtkXError(Display *display, XErrorEvent *err)
// {
// // cause a segfault
//   *(float *)(0x01) = 1.0;
//   return 1;
// }}

void vtkXOpenGLRenderWindow::MakeCurrent()
{
  // load GLX
  if (!this->EnsureGLX())
  {
    return;
  }
  // when debugging XErrors uncomment the following lines
  // if (this->DisplayId)
  //   {
  //     XSynchronize(this->DisplayId,1);
  //   }
  //  XSetErrorHandler(vtkXError);
  if (this->Internal->ContextId &&
    ((this->Internal->ContextId != glXGetCurrentContext()) || this->ForceMakeCurrent))
  {
    glXMakeCurrent(this->DisplayId, this->WindowId, this->Internal->ContextId);
    this->ForceMakeCurrent = 0;
  }
}

void vtkXOpenGLRenderWindow::ReleaseCurrent()
{
  // load GLX
  if (!this->EnsureGLX())
  {
    return;
  }
  if (this->Internal->ContextId && (this->Internal->ContextId == glXGetCurrentContext()) &&
    this->DisplayId)
  {
    glXMakeCurrent(this->DisplayId, None, nullptr);
  }
}

//------------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkXOpenGLRenderWindow::IsCurrent()
{
  // load GLX
  if (!this->EnsureGLX())
  {
    return false;
  }
  bool result = false;
  if (this->Internal->ContextId)
  {
    result = this->Internal->ContextId == glXGetCurrentContext();
  }
  return result;
}

void vtkXOpenGLRenderWindow::PushContext()
{
  // load GLX
  if (!this->EnsureGLX())
  {
    return;
  }
  GLXContext current = glXGetCurrentContext();
  this->ContextStack.push(current);
  this->DisplayStack.push(glXGetCurrentDisplay());
  this->DrawableStack.push(glXGetCurrentDrawable());
  if (this->Internal->ContextId != current)
  {
    this->MakeCurrent();
  }
}

void vtkXOpenGLRenderWindow::PopContext()
{
  // load GLX
  if (!this->EnsureGLX())
  {
    return;
  }
  GLXContext current = glXGetCurrentContext();
  GLXContext target = static_cast<GLXContext>(this->ContextStack.top());
  this->ContextStack.pop();
  if (target && target != current)
  {
    glXMakeCurrent(this->DisplayStack.top(), this->DrawableStack.top(), target);
  }
  this->DisplayStack.pop();
  this->DrawableStack.pop();
}

void vtkXOpenGLRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

vtkTypeBool vtkXOpenGLRenderWindowFoundMatch;

#define vtkXOpenGLRenderWindowPredProc VTK_ABI_NAMESPACE_MANGLE(vtkXOpenGLRenderWindowPredProc)
extern "C"
{
  Bool VTK_ABI_NAMESPACE_MANGLE(vtkXOpenGLRenderWindowPredProc)(
    Display* vtkNotUsed(disp), XEvent* event, char* arg)
  {
    Window win = (Window)arg;

    if (((reinterpret_cast<XAnyEvent*>(event))->window == win) && ((event->type == ButtonPress)))
    {
      vtkXOpenGLRenderWindowFoundMatch = 1;
    }

    return 0;
  }
}

void* vtkXOpenGLRenderWindow::GetGenericContext()
{
  static GC gc = static_cast<GC>(nullptr);
  if (!gc)
  {
    gc = XCreateGC(this->DisplayId, this->WindowId, 0, nullptr);
  }
  return static_cast<void*>(gc);
}

void* vtkXOpenGLRenderWindow::GetGenericFBConfig()
{
  return reinterpret_cast<void*>(&(this->Internal->FBConfig));
}

vtkTypeBool vtkXOpenGLRenderWindow::GetEventPending()
{
  XEvent report;

  vtkXOpenGLRenderWindowFoundMatch = 0;
  if (!this->ShowWindow)
  {
    return vtkXOpenGLRenderWindowFoundMatch;
  }
  XCheckIfEvent(this->DisplayId, &report, vtkXOpenGLRenderWindowPredProc,
    reinterpret_cast<char*>(this->WindowId));
  return vtkXOpenGLRenderWindowFoundMatch;
}

// Get the size of the screen in pixels
int* vtkXOpenGLRenderWindow::GetScreenSize()
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

// Get the position in screen coordinates (pixels) of the window.
int* vtkXOpenGLRenderWindow::GetPosition()
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

// Get this RenderWindow's X display id.
Display* vtkXOpenGLRenderWindow::GetDisplayId()
{
  vtkDebugMacro(<< "Returning DisplayId of " << static_cast<void*>(this->DisplayId) << "\n");

  return this->DisplayId;
}

bool vtkXOpenGLRenderWindow::EnsureDisplay()
{
  if (!this->DisplayId)
  {
    this->DisplayId = XOpenDisplay(static_cast<char*>(nullptr));
    if (this->DisplayId == nullptr)
    {
      // Only warn about DISPLAY if on-screen rendering is selected,
      // this helps with automatic detection of best window backend.
      if (this->ShowWindow)
      {
        vtkWarningMacro(<< "bad X server connection. DISPLAY="
                        << vtksys::SystemTools::GetEnv("DISPLAY"));
      }
    }
    else
    {
      this->OwnDisplay = 1;
    }
  }
  return this->DisplayId != nullptr;
}

bool vtkXOpenGLRenderWindow::EnsureGLX()
{
  if (!::GLADGLXLibraryDisplayOpened.exchange(true))
  {
    ::GLADGLXLibraryDisplay = XOpenDisplay(static_cast<char*>(nullptr));
    if (::GLADGLXLibraryDisplay)
    {
      ::GLXVersion = gladLoaderLoadGLX(::GLADGLXLibraryDisplay, 0);
      if (::GLXVersion > 0)
      {
        return true;
      }
      else
      {
        vtkGenericWarningMacro(<< "Failed to load GLX. Please install OpenGL drivers.");
        return false;
      }
    }
    else
    {
      vtkGenericWarningMacro(<< "bad X server connection. DISPLAY="
                             << vtksys::SystemTools::GetEnv("DISPLAY"));
      return false;
    }
  }
  else
  {
    return (::GLXVersion > 0);
  }
}

// Get this RenderWindow's parent X window id.
Window vtkXOpenGLRenderWindow::GetParentId()
{
  vtkDebugMacro(<< "Returning ParentId of " << reinterpret_cast<void*>(this->ParentId) << "\n");
  return this->ParentId;
}

// Get this RenderWindow's X window id.
Window vtkXOpenGLRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << reinterpret_cast<void*>(this->WindowId) << "\n");
  return this->WindowId;
}

// Move the window to a new position on the display.
void vtkXOpenGLRenderWindow::SetPosition(int x, int y)
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

// Sets the parent of the window that WILL BE created.
void vtkXOpenGLRenderWindow::SetParentId(Window arg)
{
  //   if (this->ParentId)
  //     {
  //     vtkErrorMacro("ParentId is already set.");
  //     return;
  //     }

  vtkDebugMacro(<< "Setting ParentId to " << reinterpret_cast<void*>(arg) << "\n");

  this->ParentId = arg;
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkXOpenGLRenderWindow::SetWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << reinterpret_cast<void*>(arg) << "\n");

  this->WindowId = arg;

  if (this->CursorHidden)
  {
    this->CursorHidden = 0;
    this->HideCursor();
  }
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkXOpenGLRenderWindow::SetWindowInfo(const char* info)
{
  // note: potential Display/Window mismatch here
  this->EnsureDisplay();

  int tmp;
  sscanf(info, "%i", &tmp);
  this->SetWindowId(static_cast<Window>(tmp));
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkXOpenGLRenderWindow::SetNextWindowInfo(const char* info)
{
  int tmp;
  sscanf(info, "%i", &tmp);
  this->SetNextWindowId(static_cast<Window>(tmp));
}

// Sets the X window id of the window that WILL BE created.
void vtkXOpenGLRenderWindow::SetParentInfo(const char* info)
{
  // note: potential Display/Window mismatch here
  this->EnsureDisplay();

  int tmp;
  sscanf(info, "%i", &tmp);
  this->SetParentId(static_cast<Window>(tmp));
}

void vtkXOpenGLRenderWindow::SetWindowId(void* arg)
{
  this->SetWindowId(reinterpret_cast<Window>(arg));
}
void vtkXOpenGLRenderWindow::SetParentId(void* arg)
{
  this->SetParentId(reinterpret_cast<Window>(arg));
}

const char* vtkXOpenGLRenderWindow::ReportCapabilities()
{
  MakeCurrent();

  if (!this->DisplayId)
  {
    return "display id not set";
  }
  // load GLX
  if (!this->EnsureGLX())
  {
    return "GLX not available.";
  }

  int scrnum = XDefaultScreen(this->DisplayId);
  const char* serverVendor = glXQueryServerString(this->DisplayId, scrnum, GLX_VENDOR);
  const char* serverVersion = glXQueryServerString(this->DisplayId, scrnum, GLX_VERSION);
  const char* serverExtensions = glXQueryServerString(this->DisplayId, scrnum, GLX_EXTENSIONS);
  const char* clientVendor = glXGetClientString(this->DisplayId, GLX_VENDOR);
  const char* clientVersion = glXGetClientString(this->DisplayId, GLX_VERSION);
  const char* glxExtensions = glXQueryExtensionsString(this->DisplayId, scrnum);
  const char* glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
  const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

  std::ostringstream strm;
  strm << "server glx vendor string:  " << serverVendor << endl;
  strm << "server glx version string:  " << serverVersion << endl;
  strm << "server glx extensions:  " << serverExtensions << endl;
  strm << "client glx vendor string:  " << clientVendor << endl;
  strm << "client glx version string:  " << clientVersion << endl;
  strm << "glx extensions:  " << glxExtensions << endl;
  strm << "OpenGL vendor string:  " << glVendor << endl;
  strm << "OpenGL renderer string:  " << glRenderer << endl;
  strm << "OpenGL version string:  " << glVersion << endl;
  strm << "OpenGL extensions:  " << endl;
  int n = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &n);
  for (int i = 0; i < n; i++)
  {
    const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
    strm << "  " << ext << endl;
  }

  strm << "X Extensions:  ";

  char** extlist = XListExtensions(this->DisplayId, &n);

  for (int i = 0; i < n; i++)
  {
    if (i != n - 1)
    {
      strm << extlist[i] << ", ";
    }
    else
    {
      strm << extlist[i] << endl;
    }
  }

  XFreeExtensionList(extlist);
  delete[] this->Capabilities;

  size_t len = strm.str().length();
  this->Capabilities = new char[len + 1];
  strncpy(this->Capabilities, strm.str().c_str(), len);
  this->Capabilities[len] = 0;

  return this->Capabilities;
}

void vtkXOpenGLRenderWindow::CloseDisplay()
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

vtkTypeBool vtkXOpenGLRenderWindow::IsDirect()
{
  this->MakeCurrent();
  // load GLX
  if (!this->EnsureGLX())
  {
    return false;
  }
  this->UsingHardware = 0;
  if (this->DisplayId && this->Internal->ContextId)
  {
    this->UsingHardware = glXIsDirect(this->DisplayId, this->Internal->ContextId) ? 1 : 0;
  }
  return this->UsingHardware;
}

void vtkXOpenGLRenderWindow::SetWindowName(const char* cname)
{
  char* name = new char[strlen(cname) + 1];
  strcpy(name, cname);
  XTextProperty win_name_text_prop;

  this->vtkOpenGLRenderWindow::SetWindowName(name);

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

void vtkXOpenGLRenderWindow::SetIcon(vtkImageData* img)
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

// Specify the X window id to use if a WindowRemap is done.
void vtkXOpenGLRenderWindow::SetNextWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting NextWindowId to " << reinterpret_cast<void*>(arg) << "\n");

  this->NextWindowId = arg;
}

void vtkXOpenGLRenderWindow::SetNextWindowId(void* arg)
{
  this->SetNextWindowId(reinterpret_cast<Window>(arg));
}

// Set the X display id for this RenderWindow to use to a pre-existing
// X display id.
void vtkXOpenGLRenderWindow::SetDisplayId(Display* arg)
{
  vtkDebugMacro(<< "Setting DisplayId to " << static_cast<void*>(arg) << "\n");

  this->DisplayId = arg;
  this->OwnDisplay = 0;
}
void vtkXOpenGLRenderWindow::SetDisplayId(void* arg)
{
  this->SetDisplayId(static_cast<Display*>(arg));
  this->OwnDisplay = 0;
}

void vtkXOpenGLRenderWindow::Render()
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
  this->vtkOpenGLRenderWindow::Render();
}

//------------------------------------------------------------------------------
void vtkXOpenGLRenderWindow::HideCursor()
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

//------------------------------------------------------------------------------
void vtkXOpenGLRenderWindow::ShowCursor()
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

// This probably has been moved to superclass.
void* vtkXOpenGLRenderWindow::GetGenericWindowId()
{
  return reinterpret_cast<void*>(this->WindowId);
}

void vtkXOpenGLRenderWindow::SetCurrentCursor(int shape)
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
VTK_ABI_NAMESPACE_END
