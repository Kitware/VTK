/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXOpenGLRenderDevice.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRect.h"

#include "vtk_glew.h"
#include <GL/glx.h>

#include "vtksys/SystemTools.hxx"

#include <sstream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <cassert>

namespace {
// Error handler for OpenGL context creation.
static bool ctxErrorOccurred = false;
static int ctxErrorHandler(Display*, XErrorEvent*)
{
  ctxErrorOccurred = true;
  return 0;
}

// Scan the extension string for extensions.
bool IsExtensionSupported(const std::string &list, const std::string &ext)
{
  if (list.find(ext) != std::string::npos)
    {
    return true;
    }
  return false;
}

}

class vtkXOpenGLRenderDevice::Private
{
public:
  Private() : ContextId(NULL) { }

  bool GetDesiredVisualInfo(Display *display, bool isDisplay, int drawableType,
                            bool alphaBitPlanes, bool doubleBuffer,
                            bool stencil, bool stereo);

  GLXFBConfig FBConfig;
  GLXContext ContextId;
};

vtkStandardNewMacro(vtkXOpenGLRenderDevice)

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig,
                                                     GLXContext, Bool,
                                                     const int*);

vtkXOpenGLRenderDevice::vtkXOpenGLRenderDevice()
  : ParentId(static_cast<Window>(NULL)), WindowId(static_cast<Window>(NULL)),
    DisplayId(NULL), ColorMap(static_cast<Colormap>(NULL)),
    OwnDisplay(true), OwnWindow(true),
    OffScreenRendering(false), Mapped(false),
    Internal(new vtkXOpenGLRenderDevice::Private)
{
}

vtkXOpenGLRenderDevice::~vtkXOpenGLRenderDevice()
{
  delete this->Internal;
  this->Internal = NULL;
}

bool vtkXOpenGLRenderDevice::Private::GetDesiredVisualInfo(Display *display,
    bool isDisplay, int drawableType, bool alphaBitPlanes, bool doubleBuffer,
    bool stencil, bool stereo)
{
  int index = 0;
  static int attributes[50];

  // Setup the default stuff we ask for.
  if (isDisplay)
    {
    attributes[index++] = GLX_X_RENDERABLE;
    attributes[index++] = True;
    }
  attributes[index++] = GLX_DRAWABLE_TYPE;
  attributes[index++] = drawableType;
  attributes[index++] = GLX_RENDER_TYPE;
  attributes[index++] = GLX_RGBA_BIT;
  attributes[index++] = GLX_RED_SIZE;
  attributes[index++] = 8;
  attributes[index++] = GLX_GREEN_SIZE;
  attributes[index++] = 8;
  attributes[index++] = GLX_BLUE_SIZE;
  attributes[index++] = 8;
  attributes[index++] = GLX_DEPTH_SIZE;
  attributes[index++] = 24;
  if (alphaBitPlanes)
    {
    attributes[index++] = GLX_ALPHA_SIZE;
    attributes[index++] = 8;
    }
  if (doubleBuffer)
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
    attributes[index++] = GLX_STEREO;
    attributes[index++] = True;
    }
  attributes[index++] = None;
  // Ensure we didn't exceed the maximum allocated size of 50 elements.
  assert(index < 50);

  int count;
  GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display),
                                       attributes, &count);
  if (!fbc || count < 1)
    {
    this->FBConfig = static_cast<GLXFBConfig>(0);
    return false;
    }
  this->FBConfig = fbc[0];
  XFree(fbc);
  return true;
}

bool vtkXOpenGLRenderDevice::CreateNewWindow(const vtkRecti &geometry,
                                             const std::string &name)
{
  XVisualInfo  *v, matcher;
  XSetWindowAttributes  attr;
  int nItems;
  XWindowAttributes winattr;
  XSizeHints xsh;

  xsh.flags = USSize;
  xsh.flags |= USPosition;
  xsh.x =      geometry.GetX();
  xsh.y =      geometry.GetY();
  xsh.width  = geometry.GetWidth();
  xsh.height = geometry.GetHeight();

  // get the default display connection
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay(static_cast<char *>(NULL));
    if (this->DisplayId == NULL)
      {
      vtkErrorMacro(<< "bad X server connection. DISPLAY="
        << vtksys::SystemTools::GetEnv("DISPLAY") << ". Aborting.\n");
      abort();
      }
    int glxMajor, glxMinor;
    if (!glXQueryVersion(this->DisplayId, &glxMajor, &glxMinor) ||
        (glxMajor == 1 && glxMinor < 3) || glxMajor < 1)
      {
      vtkErrorMacro(<< "Invalid GLX version: " << glxMajor << "." << glxMinor);
      abort();
      }
    this->OwnDisplay = true;
    }

  attr.override_redirect = False;
  if (this->Borders == 0.0)
    {
    //attr.override_redirect = True;
    }

  // Create our own window?
  this->OwnWindow = 0;
  if (!this->WindowId)
    {
    bool res = this->Internal->GetDesiredVisualInfo(this->DisplayId, true,
                                                    GLX_WINDOW_BIT, true, true,
                                                    true, false);
    if (!res)
      {
      vtkErrorMacro(<< "Failed to retrieve a framebuffer config.");
      return false;
      }
    v = glXGetVisualFromFBConfig(this->DisplayId, this->Internal->FBConfig);
    this->ColorMap = XCreateColormap(this->DisplayId,
                                     XRootWindow(this->DisplayId, v->screen),
                                     v->visual, AllocNone);

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
      XCreateWindow(this->DisplayId,
                    this->ParentId,
                    geometry.GetX(), geometry.GetY(),
                    static_cast<unsigned int>(geometry.GetWidth()),
                    static_cast<unsigned int>(geometry.GetHeight()), 0,
                    v->depth,
                    InputOutput, v->visual,
                    CWBackPixel | CWBorderPixel | CWColormap |
                    CWOverrideRedirect | CWEventMask,
                    &attr);
    XStoreName(this->DisplayId, this->WindowId, name.c_str());
    XSetNormalHints(this->DisplayId, this->WindowId, &xsh);
    this->OwnWindow = 1;
    }
  else
    {
    XChangeWindowAttributes(this->DisplayId,this->WindowId,
                            CWOverrideRedirect, &attr);
    XGetWindowAttributes(this->DisplayId, this->WindowId, &winattr);
    matcher.visualid = XVisualIDFromVisual(winattr.visual);
    matcher.screen = XDefaultScreen(DisplayId);
    v = XGetVisualInfo(this->DisplayId, VisualIDMask | VisualScreenMask,
                       &matcher, &nItems);
    }

  if (this->OwnWindow)
    {
    // Resize the window to the requested size.
    vtkDebugMacro(<< "Resizing the xwindow\n");
    XResizeWindow(this->DisplayId, this->WindowId,
                  static_cast<unsigned int>(geometry.GetWidth()),
                  static_cast<unsigned int>(geometry.GetHeight()));
    XSync(this->DisplayId, False);
    }

  // Now to create an OpenGL context!
  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
        glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
  // We need to install an error handler in order to avoid exiting on failure
  // to create a GL 3/4 context. The error handler is global, so care must be
  // taken to nesure no other threads issue X commands while this is running.
  ctxErrorOccurred = false;
  // FIXME: Restore old handler?
  /* int (*oldHandler)(Display*, XErrorEvent*) = */ XSetErrorHandler(&ctxErrorHandler);

  // Get the default screen's GLX extension list
  const char *glxExts = glXQueryExtensionsString(this->DisplayId,
                                                 DefaultScreen(this->DisplayId));
  cout << "glxExtensions:\n" << glxExts << endl;
  if (!IsExtensionSupported(glxExts, "GLX_ARB_create_context") ||
      !glXCreateContextAttribsARB)
    {
    vtkErrorMacro(<< "Cannot find GLX_ARB_create_context extension, GL 2.1.");
    this->Internal->ContextId =
      glXCreateNewContext(this->DisplayId, this->Internal->FBConfig,
                          GLX_RGBA_TYPE, 0, True);
    }
  else
    {
    cout << "Creating a GL 3/4 context!!!" << endl;
    int attributes[] =
      {
      GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
      GLX_CONTEXT_MINOR_VERSION_ARB, 4,
      None
      };
    this->Internal->ContextId = glXCreateContextAttribsARB(this->DisplayId,
                                                           this->Internal->FBConfig,
                                                           0, True, attributes);
    }

  if (!this->Internal->ContextId)
    {
    vtkErrorMacro("Cannot create GLX context.  Aborting.");
    if (this->HasObserver(vtkCommand::ExitEvent))
      {
      this->InvokeEvent(vtkCommand::ExitEvent, NULL);
      return false;
      }
    else
      {
      abort();
      }
    }

  if (this->OwnWindow && !this->OffScreenRendering)
    {
    vtkDebugMacro(" Mapping the xwindow\n");
    XMapWindow(this->DisplayId, this->WindowId);
    XSync(this->DisplayId, False);
    XGetWindowAttributes(this->DisplayId,
                         this->WindowId, &winattr);
    // Guarantee that the window is mapped before the program continues
    // on to do the OpenGL rendering.
    while (winattr.map_state == IsUnmapped)
      {
      XGetWindowAttributes(this->DisplayId, this->WindowId, &winattr);
      }
    }
  // Free the visual info
  if (v)
    {
    XFree(v);
    }
  this->Mapped = true;

  glXMakeCurrent(this->DisplayId, this->WindowId, this->Internal->ContextId);

//  /* event loop */
//  XEvent event;
//  XSelectInput(this->DisplayId, this->WindowId, ExposureMask | KeyPressMask);
//  int s = DefaultScreen(this->DisplayId);
//      for (;;)
//      {
//          XNextEvent(this->DisplayId, &event);
//          cout << "New event..." << endl;

//          /* draw or redraw the window */
//          if (event.type == Expose)
//          {
//            cout << "Expose\n";
//              XFillRectangle(this->DisplayId, this->WindowId, DefaultGC(this->DisplayId, s), 20, 20, 10, 10);
//              //XDrawString(display, this->WindowId, DefaultGC(this->DisplayId, s), 50, 50, msg, strlen(msg));
//              XFlush(this->DisplayId);
//          }
//          if (event.type == MapNotify)
//            cout << "MapNotify\n";
//          /* exit on key press */
//          if (event.type == KeyPress)
//          {
//            cout << "KeyPress\n";
//              break;
//          }
//      }

//      /* close connection to server */
////      XCloseDisplay(this->DisplayId);


  // Initialize GLEW.
  GLenum result = glewInit();
  if (result != GLEW_OK)
    {
    vtkErrorMacro(<< "GLEW could not be initialized, aborting.\n"
                  << (const char *)glewGetErrorString(result));
    return false;
    }
  if (!GLEW_VERSION_2_1)
    {
    vtkErrorMacro(<< "GL version 2.1 is not supported by your graphics driver.");
    return false;
    }
  if (!GLEW_VERSION_3_0)
    {
    vtkErrorMacro(<< "GL version 3.0 is not supported by your graphics driver.");
    return false;
    }
  if (!GLEW_VERSION_4_0)
    {
    vtkErrorMacro(<< "GL version 4.4 is not supported by your graphics driver.");
    return false;
    }

  return true;
}

void vtkXOpenGLRenderDevice::MakeCurrent()
{
  glXMakeCurrent(this->DisplayId, this->WindowId, this->Internal->ContextId);
}

void vtkXOpenGLRenderDevice::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
