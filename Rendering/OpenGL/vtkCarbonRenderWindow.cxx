/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCarbonRenderWindow.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCarbonRenderWindow.h"

#include "vtkCarbonRenderWindowInteractor.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLTexture.h"
#include "vtkRendererCollection.h"

#include <math.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCarbonRenderWindow);

//----------------------------------------------------------------------------
// Dump agl errors to string, return error code
OSStatus aglReportError ()
{
  GLenum err = aglGetError();
  if (AGL_NO_ERROR != err)
    cout << ((char *)aglErrorString(err));
  // ensure we are returning an OSStatus noErr if no error condition
  if (err == AGL_NO_ERROR)
    return noErr;
  else
    return (OSStatus) err;
}

//----------------------------------------------------------------------------
// if error dump gl errors, return error
OSStatus glReportError ()
{
  GLenum err = glGetError();
  switch (err)
    {
    case GL_NO_ERROR:
      break;
    case GL_INVALID_ENUM:
      cout << "GL Error: Invalid enumeration\n";
      break;
    case GL_INVALID_VALUE:
      cout << "GL Error: Invalid value\n";
      break;
    case GL_INVALID_OPERATION:
      cout << "GL Error: Invalid operation\n";
      break;
    case GL_STACK_OVERFLOW:
      cout << "GL Error: Stack overflow\n";
      break;
    case GL_STACK_UNDERFLOW:
      cout << "GL Error: Stack underflow\n";
      break;
    case GL_OUT_OF_MEMORY:
      cout << "GL Error: Out of memory\n";
      break;
    }
  // ensure we are returning an OSStatus noErr if no error condition
  if (err == GL_NO_ERROR)
    return noErr;
  else
    return (OSStatus) err;
}

//-----------------------------------------------------------------------------
static void* vtkCreateOSWindow(int width, int height, int pixel_size)
{
  return malloc(width*height*pixel_size);
}

//-----------------------------------------------------------------------------
static void vtkDestroyOSWindow(void* win)
{
  free(win);
}

class vtkCarbonRenderWindowInternal
{
public:
  vtkCarbonRenderWindowInternal(vtkRenderWindow* win)
    {
      this->OffScreenWindow = NULL;
      this->OffScreenContextId = NULL;
      this->ScreenMapped = win->GetMapped();
      this->ScreenDoubleBuffer = win->GetDoubleBuffer();
    }

  void* OffScreenWindow;
  AGLContext OffScreenContextId;
  AGLPixelFormat OffScreenPixelFmt;
  int ScreenMapped;
  int ScreenDoubleBuffer;

  AGLPixelFormat ChoosePixelFormat(int accel, int offscreen, int doublebuff, int stereo,
    int multisamples, int alphaBitPlanes, int stencil);

  AGLContext CreateContext(int offscreen, int& doublebuff, int& stereo,
    int& multisamples, int& alphaBitPlanes, int &stencil, const char*& error);

};

AGLPixelFormat vtkCarbonRenderWindowInternal::ChoosePixelFormat(int accel, int offscreen, int doublebuff,
  int stereo, int multisamples, int alphaBitPlanes, int stencil)
{
  int i = 0;
  GLint attr[64];

  if(offscreen)
    {
    attr[i++] = AGL_OFFSCREEN;
    }
  if(doublebuff)
    {
    attr[i++] = AGL_DOUBLEBUFFER;
    }
  attr[i++] = AGL_RGBA;
  attr[i++] = AGL_DEPTH_SIZE;
  attr[i++] = 32;
  attr[i++] = AGL_PIXEL_SIZE;
  attr[i++] = 32;
  if(accel)
    {
    attr[i++] = AGL_ACCELERATED;
    }
  if(multisamples)
    {
    attr[i++] = AGL_SAMPLE_BUFFERS_ARB;
    attr[i++] = 1;
    attr[i++] = AGL_SAMPLES_ARB;
    attr[i++] = multisamples;
    attr[i++] = AGL_MULTISAMPLE;
    }
  if (alphaBitPlanes)
    {
    attr[i++] = AGL_ALPHA_SIZE;
    attr[i++] = 8;
    }
  if(stereo)
    {
    attr[i++] = AGL_STEREO;
    attr[i++] = GL_TRUE;
    }
  if(stencil)
    {
    attr[i++] = AGL_STENCIL_SIZE;
    attr[i++] = 8;
    }
  attr[i++] = AGL_NO_RECOVERY;  // must choose the pixel format we want!
  attr[i++] = AGL_NONE;

  return aglChoosePixelFormat (NULL, 0, attr);
}

AGLContext vtkCarbonRenderWindowInternal::CreateContext(int offscreen, int& doublebuff,
  int& stereo, int& multisamples, int& alphaBitPlanes, int& stencil, const char*& error)
{
  error = NULL;
  AGLContext ctx = 0;
  AGLPixelFormat fmt = 0;
  int noSoftwareRendering = 1;  // flip to zero if you're willing to do software
                                // rendering to get more features.

  int _db, _a, _s, _m, _stencil;

  for(_stencil = stencil; !fmt && _stencil >= 0; _stencil--)
    {
    for(_db = doublebuff; !fmt && _db >= 0; _db--)
      {
      for(_a = alphaBitPlanes; !fmt && _a >= 0; _a--)
        {
        for(_s = stereo; !fmt && _s >= 0; _s--)
          {
          for(_m = multisamples; !fmt && _m >= 0; _m--)
            {
            for(int accel = 1; !fmt && accel >= noSoftwareRendering; accel--)
              {
              fmt = this->ChoosePixelFormat(accel, offscreen, _db, _s, _m, _a, _stencil);
              if(fmt)
                {
                doublebuff = _db;
                stereo = _s;
                multisamples = _m;
                alphaBitPlanes = _a;
                stencil = _stencil;
                }
              }
            }
          }
        }
      }
    }
  aglReportError (); // cough up any errors encountered
  if (NULL == fmt)
    {
    error = "Could not find valid pixel format";
    return NULL;
    }

  ctx = aglCreateContext (fmt, 0); // create without sharing
  aglDestroyPixelFormat(fmt);
  aglReportError (); // cough up errors
  if (NULL == ctx)
    {
    error = "Could not create context";
    return NULL;
    }
  return ctx;
}


//--------------------------------------------------------------------------
vtkCarbonRenderWindow::vtkCarbonRenderWindow()
{
  this->Internal = new vtkCarbonRenderWindowInternal(this);
  this->ApplicationInitialized = 0;
  this->ContextId = 0;
  this->WindowId = 0;
  this->ParentId = 0;
  this->RootWindow = 0;
  this->OwnWindow = 0; // Keep before SetWindowName.
  this->SetWindowName("Visualization Toolkit - Carbon");
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->RegionEventHandlerUPP = 0;
  this->RegionEventHandler = 0;
}

// --------------------------------------------------------------------------
vtkCarbonRenderWindow::~vtkCarbonRenderWindow()
{
  this->Finalize();

  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
    ren->SetRenderWindow(NULL);
    }

  delete this->Internal;
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::DestroyWindow()
{
  GLuint txId;

  this->MakeCurrent();

  // tell each of the renderers that this render window/graphics context
  // is being removed (the RendererCollection is removed by vtkRenderWindow's
  // destructor)
  vtkCollectionSimpleIterator rsit;
  vtkRenderer *ren;
  for ( this->Renderers->InitTraversal(rsit);
        (ren = this->Renderers->GetNextRenderer(rsit));)
    {
    ren->SetRenderWindow(NULL);
    ren->SetRenderWindow(this);
    }

  /* finish OpenGL rendering */
  if (this->ContextId)
    {

    /* now delete all textures */
    glDisable(GL_TEXTURE_2D);
    for (int i = 1; i < this->TextureResourceIds->GetNumberOfIds(); i++)
      {
      txId = (GLuint) this->TextureResourceIds->GetId(i);
#ifdef GL_VERSION_1_1
      if (glIsTexture(txId))
        {
        glDeleteTextures(1, &txId);
        }
#else
      if (glIsList(txId))
        {
        glDeleteLists(txId,1);
        }
#endif
      }

    aglSetCurrentContext(this->ContextId);
    aglDestroyContext(this->ContextId);
    this->ContextId = NULL;
    }

  // remove event filters if we have them
  if(this->RegionEventHandler)
    {
    RemoveEventHandler(this->RegionEventHandler);
    DisposeEventHandlerUPP(this->RegionEventHandlerUPP);
    this->RegionEventHandler = 0;
    this->RegionEventHandlerUPP = 0;
    }

  if (this->RootWindow && this->OwnWindow)
    {
    DisposeWindow(this->RootWindow);
    this->RootWindow = 0;
    this->WindowId = 0;
    }

  this->Mapped = 0;
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetWindowName( const char * _arg )
{
  vtkWindow::SetWindowName(_arg);

  if(this->OwnWindow)
    {
    CFStringRef newTitle =
      CFStringCreateWithCString(kCFAllocatorDefault, _arg,
                                kCFStringEncodingASCII);
    SetWindowTitleWithCFString(this->RootWindow, newTitle);
    CFRelease(newTitle);
    }
}

//--------------------------------------------------------------------------
int vtkCarbonRenderWindow::GetEventPending()
{
  return 0;
}

//--------------------------------------------------------------------------
// Set the window id to a pre-existing window.
void vtkCarbonRenderWindow::SetParentId(HIViewRef arg)
{
  vtkDebugMacro(<< "Setting ParentId to " << arg << "\n");

  this->ParentId = arg;
}

//--------------------------------------------------------------------------
// Begin the rendering process.
void vtkCarbonRenderWindow::Start()
{
  // if the renderer has not been initialized, do so now
  this->Initialize();

  // set the current window
  this->MakeCurrent();
}

// --------------------------------------------------------------------------
void vtkCarbonRenderWindow::MakeCurrent()
{
  if(this->OffScreenRendering && this->Internal->OffScreenContextId)
    {
    if((this->Internal->OffScreenContextId != aglGetCurrentContext())
       || this->ForceMakeCurrent)
      {
      aglSetCurrentContext(this->Internal->OffScreenContextId);
      this->ForceMakeCurrent = 0;
      }
    }
  else if (this->ContextId || this->ForceMakeCurrent)
    {
    if((this->ContextId != aglGetCurrentContext()) || this->ForceMakeCurrent)
      {
      aglSetCurrentContext(this->ContextId);
      this->ForceMakeCurrent = 0;
      }
    }
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkCarbonRenderWindow::IsCurrent()
{
  bool result;

  if(this->OffScreenRendering && this->Internal->OffScreenContextId)
    {
    result=this->Internal->OffScreenContextId==aglGetCurrentContext();
    }
  else
    {
    result=this->ContextId!=0 && this->ContextId==aglGetCurrentContext();
    }
  return result;
}

// --------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

// --------------------------------------------------------------------------
int vtkCarbonRenderWindow::IsDirect()
{
  return 1;
}

// --------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetSize(int a[2])
{
  this->SetSize(a[0], a[1]);
}

void vtkCarbonRenderWindow::UpdateGLRegion()
{
  // if WindowId is a child window
  if(this->WindowId)
    {
    // Determine the AGL_BUFFER_RECT for the view. The coordinate
    // system for this rectangle is relative to the owning window, with
    // the origin at the bottom left corner and the y-axis inverted.
    HIRect viewBounds, winBounds;
    HIViewGetBounds(this->WindowId, &viewBounds);
    HIViewRef root = HIViewGetRoot(this->GetRootWindow());
    HIViewRef content_root;
    HIViewFindByID(root, kHIViewWindowContentID, &content_root);

    HIViewGetBounds(content_root, &winBounds);
    HIViewConvertRect(&viewBounds, this->WindowId, content_root);
    GLint bufferRect[4] = { GLint(viewBounds.origin.x),
                            GLint((winBounds.size.height) - (viewBounds.origin.y + viewBounds.size.height)),
                            GLint(viewBounds.size.width),
                            GLint(viewBounds.size.height) };
    if(!HIViewIsVisible(this->WindowId))
      {
      bufferRect[0] = 0;
      bufferRect[1] = 0;
      bufferRect[2] = 0;
      bufferRect[3] = 0;
      }

    // Associate the OpenGL context with the control's window, and establish the buffer rect.
#if 0
    aglSetWindowRef(this->ContextId, this->GetRootWindow());
#else
    aglSetDrawable(this->ContextId, GetWindowPort(this->GetRootWindow()));
#endif
    aglSetInteger(this->ContextId, AGL_BUFFER_RECT, bufferRect);
    aglEnable(this->ContextId, AGL_BUFFER_RECT);

    // Establish the clipping region for the OpenGL context. To properly handle clipping
    // within the view hierarchy, walk the hierarchy to determine the intersection
    // of this view's bounds with its children, siblings, and parents also taking into
    // account z-ordering of the views
    RgnHandle rgn = NewRgn();
    RgnHandle tmp_rgn = NewRgn();

    GetControlRegion(this->WindowId, kControlStructureMetaPart, rgn);
    HIViewConvertRegion(rgn, this->WindowId, content_root);

    HIViewRef current_view = NULL;
    HIViewRef child = NULL;
    HIViewRef last = NULL;

    for(current_view = this->WindowId;
        (current_view != NULL);
        current_view = HIViewGetSuperview(current_view))
      {
      if(last)
        {
        // clip view within parent bounds
        GetControlRegion(current_view, kControlStructureMetaPart, tmp_rgn);
        HIViewConvertRegion(tmp_rgn, current_view, content_root);
        DiffRgn(rgn, tmp_rgn, tmp_rgn);
        DiffRgn(rgn, tmp_rgn, rgn);
        }
      for(child = HIViewGetFirstSubview(current_view);
          (child != last) && (child != NULL);
          child = HIViewGetNextView(child))
        {
        if(child != last && HIViewIsVisible(child))
          {
          GetControlRegion(child, kControlStructureMetaPart, tmp_rgn);
          HIViewConvertRegion(tmp_rgn, child, content_root);
          DiffRgn(rgn, tmp_rgn, rgn);
          }
        }
      last = current_view;
      }

    GetControlRegion(this->WindowId, kControlStructureMetaPart, tmp_rgn);

    if(EqualRgn(rgn,tmp_rgn))
      {
      if(aglIsEnabled(this->ContextId, AGL_CLIP_REGION))
        {
        aglDisable(this->ContextId, AGL_CLIP_REGION);
        }
      }
    else
      {
      if(!aglIsEnabled(this->ContextId, AGL_CLIP_REGION))
        {
        aglEnable(this->ContextId, AGL_CLIP_REGION);
        }
      aglSetInteger(this->ContextId, AGL_CLIP_REGION, reinterpret_cast<const GLint*>(rgn));
      }

    DisposeRgn(rgn);
    DisposeRgn(tmp_rgn);

    }
  // this is provided for backwards compatibility
  else if(this->WindowId == 0 && this->RootWindow && this->ParentId)
    {
    GLint bufRect[4];
    Rect windowRect;
    GetWindowBounds(this->RootWindow, kWindowContentRgn, &windowRect);
    bufRect[0] = this->Position[0];
    bufRect[1] = (int) (windowRect.bottom-windowRect.top)
      - (this->Position[1]+this->Size[1]);
    bufRect[2] = this->Size[0];
    bufRect[3] = this->Size[1];
    aglEnable(this->ContextId, AGL_BUFFER_RECT);
    aglSetInteger(this->ContextId, AGL_BUFFER_RECT, bufRect);
    }

  aglUpdateContext(this->ContextId);

}

// --------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetSize(int x, int y)
{
  static int resizing = 0;

  if ((this->Size[0] != x) || (this->Size[1] != y))
    {
    this->Size[0] = x;
    this->Size[1] = y;

    if(this->OffScreenRendering &&
       (this->Internal->OffScreenWindow
        || this->OffScreenUseFrameBuffer))
      {
      this->ResizeOffScreenWindow(x,y);
      }
    else
      {
      if (this->Mapped)
        {
        if (!resizing)
          {
          resizing = 1;

          if(this->ParentId && this->RootWindow && !this->WindowId)
            {
            // backwards compatibility with Tk and who else?
            UpdateGLRegion();
            }
          else if(this->OwnWindow || !this->WindowId)
            {
            SizeWindow(this->RootWindow, x, y, TRUE);
            }
          resizing = 0;
          }
        }
      }

    this->Modified();
    }
}

// --------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetPosition(int a[2])
{
  this->SetPosition(a[0], a[1]);
}

// --------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetPosition(int x, int y)
{
  static int resizing = 0;

  if ((this->Position[0] != x) || (this->Position[1] != y))
    {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    if (this->Mapped)
      {
      if (!resizing)
        {
        resizing = 1;

        if(this->ParentId && this->RootWindow && !this->WindowId)
          {
          // backwards compatibility with Tk and who else?
          UpdateGLRegion();
          }
        else if(this->OwnWindow || !this->WindowId)
          {
          MoveWindow(this->RootWindow, x, y, FALSE);
          }

        resizing = 0;
        }
      }
    }
}


//--------------------------------------------------------------------------
// End the rendering process and display the image.
void vtkCarbonRenderWindow::Frame()
{
  this->MakeCurrent();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
    {
    glFinish();
    aglSwapBuffers(this->ContextId);
    vtkDebugMacro(<< " aglSwapBuffers\n");
    }
  else
    {
    glFlush();
    }
}

//--------------------------------------------------------------------------
AGLContext vtkCarbonRenderWindow::GetContextId()
{
  if(this->OffScreenRendering)
    {
    return this->Internal->OffScreenContextId;
    }
  return this->ContextId;
}

//--------------------------------------------------------------------------
// Specify various window parameters.
void vtkCarbonRenderWindow::WindowConfigure()
{
  // this is all handled by the desiredVisualInfo method
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::InitializeApplication()
{
  if (!this->ApplicationInitialized)
    {
    if (this->OwnWindow)
      {
      // Initialize the Toolbox managers if we are running the show
      DrawMenuBar();
      this->ApplicationInitialized=1;
      }
    }
}

//--------------------------------------------------------------------------
// Initialize the window for rendering.
void vtkCarbonRenderWindow::CreateAWindow()
{
  static int count = 1;
  char *windowName;

  // if a Window and HIView wasn't given, make a Window and HIView
  if (!this->WindowId && !this->RootWindow)
    {
    this->Position[0] = ((this->Position[0] >= 0) ? this->Position[0] : 5);
    this->Position[1] = ((this->Position[1] >= 0) ? this->Position[1] : 5);
    this->Size[0] = ((this->Size[0] > 0) ? this->Size[0] : 300);
    this->Size[1] = ((this->Size[1] > 0) ? this->Size[1] : 300);

    // Rect is defined as {top, left, bottom, right} (really)
    Rect rectWin = {this->Position[1], this->Position[0],
                    this->Position[1]+this->Size[1],
                    this->Position[0]+this->Size[0]};

    WindowAttributes windowAttrs = (kWindowStandardDocumentAttributes |
                                    kWindowLiveResizeAttribute |
                                    kWindowStandardHandlerAttribute |
                                    kWindowCompositingAttribute);

    if (noErr != CreateNewWindow (kDocumentWindowClass,
                                  windowAttrs,
                                  &rectWin, &(this->RootWindow)))
      {
      vtkErrorMacro("Could not create window, serious error!");
      return;
      }

    // get the content view
    HIViewFindByID(HIViewGetRoot(this->RootWindow),
                   kHIViewWindowContentID,
                   &this->WindowId);

    int len = (strlen("Visualization Toolkit - Carbon #")
               + (int) ceil((double)log10((double)(count+1)))
               + 1);
    windowName = new char [ len ];
    sprintf(windowName,"Visualization Toolkit - Carbon #%i",count++);
    this->OwnWindow = 1;
    this->SetWindowName(windowName);
    delete [] windowName;

    ShowWindow(this->RootWindow);
    }


  // install event handler for updating gl region
  // this works for a supplied HIView and an HIView made here
  if(this->WindowId && !this->RegionEventHandler)
    {
    EventTypeSpec region_events [] =
      {
        { kEventClassControl, kEventControlOwningWindowChanged},
        { kEventClassControl, kEventControlVisibilityChanged },
        { kEventClassControl, kEventControlBoundsChanged }
      };
    this->RegionEventHandlerUPP = NewEventHandlerUPP(vtkCarbonRenderWindow::RegionEventProcessor);
    InstallControlEventHandler(this->WindowId, this->RegionEventHandlerUPP,
                               GetEventTypeCount(region_events), region_events,
                               reinterpret_cast<void*>(this), &this->RegionEventHandler);
    }

  SetPortWindowPort(this->GetRootWindow());

  const char* error = NULL;
  this->ContextId = this->Internal->CreateContext(0, this->DoubleBuffer,
                                this->StereoCapableWindow, this->MultiSamples,
                                this->AlphaBitPlanes, this->StencilCapable,
                                error);
  if (NULL == this->ContextId)
    {
    if(error)
      {
      vtkErrorMacro(<<error);
      }
    return;
    }

  // This syncs the OpenGL context to the VBL to prevent tearing
  GLint one = 1;
  GLboolean res = aglSetInteger (this->ContextId, AGL_SWAP_INTERVAL, &one);
  if (GL_FALSE == res)
    {
    vtkErrorMacro ("Could not set context option");
    return;
    }

#if 0
  // attach the WindowRef to the context
  if (!aglSetWindowRef (this->ContextId, this->GetRootWindow()))
#else
  // attach the CGrafPtr to the context
  if (!aglSetDrawable (this->ContextId, GetWindowPort (this->GetRootWindow())))
#endif
    {
    aglReportError();
    return;
    }

  if(!aglSetCurrentContext (this->ContextId))
    // make the context the current context
    {
    aglReportError();
    return;
    }

  this->OpenGLInit();
  this->Mapped = 1;
  UpdateGLRegion();
}

//--------------------------------------------------------------------------
// Initialize the window for rendering.
void vtkCarbonRenderWindow::WindowInitialize()
{
  // create our own window if not already set
  this->InitializeApplication();
  this->OwnWindow = 0;
  this->CreateAWindow();

  // tell our renderers about us
  vtkRenderer* ren;
  for (this->Renderers->InitTraversal();
       (ren = this->Renderers->GetNextItem());)
    {
    ren->SetRenderWindow(0);
    ren->SetRenderWindow(this);
    }

  // set the DPI
  this->SetDPI(72); // this may need to be more clever some day
}

//-----------------------------------------------------------------------------
// Initialize the rendering window.
void vtkCarbonRenderWindow::Initialize ()
{
  // make sure we havent already been initialized

  if(!this->OffScreenRendering && !this->ContextId)
    {
    this->WindowInitialize();
    }
  else if(this->OffScreenRendering &&
          !(this->Internal->OffScreenContextId
            || this->OffScreenUseFrameBuffer))
    {
    // initialize offscreen window
    int width = ((this->Size[0] > 0) ? this->Size[0] : 300);
    int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
    this->CreateOffScreenWindow(width, height);
    }
}

//-----------------------------------------------------------------------------
void vtkCarbonRenderWindow::Finalize(void)
{
  if (this->CursorHidden)
    {
    this->ShowCursor();
    }

  this->SetOffScreenRendering(0);

  this->DestroyWindow();

}

//-----------------------------------------------------------------------------
void vtkCarbonRenderWindow::SetOffScreenRendering(int i)
{
  if (this->OffScreenRendering == i)
    {
    return;
    }

  Superclass::SetOffScreenRendering(i);

  // setup the offscreen area
  if(i)
    {
    this->Internal->ScreenDoubleBuffer = this->DoubleBuffer;
    this->DoubleBuffer = 0;
    this->Internal->ScreenMapped = this->Mapped;
    this->Mapped = 0;
    }
  else
    {
    this->DestroyOffScreenWindow();

    this->DoubleBuffer = this->Internal->ScreenDoubleBuffer;
    this->Mapped = this->Internal->ScreenMapped;
    // reset the size based on the screen window
    this->GetSize();
    }
}

//-----------------------------------------------------------------------------
void vtkCarbonRenderWindow::CreateOffScreenWindow(int width, int height)
{
  if(!this->CreateHardwareOffScreenWindow(width,height))
    {
    const char* error = NULL;
    int doubleBuf = 0;
    this->Internal->OffScreenContextId =
      this->Internal->CreateContext(1, doubleBuf,
         this->StereoCapableWindow, this->MultiSamples,
         this->AlphaBitPlanes, this->StencilCapable, error);

    this->Internal->OffScreenWindow = vtkCreateOSWindow(width, height, 4);
    this->Size[0] = width;
    this->Size[1] = height;

    aglSetOffScreen(this->Internal->OffScreenContextId,
                    width, height, width*4,
                    this->Internal->OffScreenWindow);

    aglSetCurrentContext(this->Internal->OffScreenContextId);
    } // if not hardware
  this->Mapped = 0;

  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
    ren->SetRenderWindow(NULL);
    ren->SetRenderWindow(this);
    }

  this->OpenGLInit();
}

//-----------------------------------------------------------------------------
void vtkCarbonRenderWindow::DestroyOffScreenWindow()
{
  // release graphic resources.
  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
    ren->SetRenderWindow(NULL);
    ren->SetRenderWindow(this);
    }

  if(this->OffScreenUseFrameBuffer)
    {
    this->DestroyHardwareOffScreenWindow();
    }
  else
    {
    if(this->Internal->OffScreenContextId)
      {
      aglDestroyContext(this->Internal->OffScreenContextId);
      this->Internal->OffScreenContextId = NULL;
      vtkDestroyOSWindow(this->Internal->OffScreenWindow);
      this->Internal->OffScreenWindow = NULL;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkCarbonRenderWindow::ResizeOffScreenWindow(int width, int height)
{
  if(!this->OffScreenRendering)
    {
    return;
    }

  if(this->OffScreenUseFrameBuffer || this->Internal->OffScreenContextId)
    {
    this->DestroyOffScreenWindow();
    this->CreateOffScreenWindow(width, height);
    }
}


//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::UpdateSizeAndPosition(int xPos, int yPos,
                                                  int xSize, int ySize)
{
  this->Size[0]=xSize;
  this->Size[1]=ySize;
  this->Position[0]=xPos;
  this->Position[1]=yPos;
  this->Modified();
}

//--------------------------------------------------------------------------
// Get the current size of the window.
int *vtkCarbonRenderWindow::GetSize()
{
  if(this->WindowId && this->Mapped)
    {
    HIRect viewBounds;
    HIViewGetBounds(this->WindowId, &viewBounds);
    this->Size[0] = (int)viewBounds.size.width;
    this->Size[1] = (int)viewBounds.size.height;
    }

  return this->Superclass::GetSize();
}

//--------------------------------------------------------------------------
// Get the current size of the screen.
int *vtkCarbonRenderWindow::GetScreenSize()
{
  CGRect r = CGDisplayBounds(CGMainDisplayID());
  this->Size[0] = (int)r.size.width;
  this->Size[1] = (int)r.size.height;
  return this->Size;
}

//--------------------------------------------------------------------------
// Get the position in screen coordinates of the window.
int *vtkCarbonRenderWindow::GetPosition()
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
    {
    return this->Position;
    }

  if(!this->WindowId && !this->ParentId)
    {
    //  Find the current window position
    Rect windowRect;
    GetWindowBounds(this->GetRootWindow(), kWindowContentRgn, &windowRect);
    this->Position[0] = windowRect.left;
    this->Position[1] = windowRect.top;
    }
  else
    {
    HIRect viewBounds;
    HIViewGetBounds(this->WindowId, &viewBounds);
    Rect windowRect;
    GetWindowBounds(this->GetRootWindow(), kWindowContentRgn, &windowRect);
    this->Position[0] = ((int)viewBounds.origin.x) + windowRect.left;
    this->Position[1] = ((int)viewBounds.origin.y) + windowRect.top;
    }

  return this->Position;
}

//--------------------------------------------------------------------------
// Change the window to fill the entire screen.
void vtkCarbonRenderWindow::SetFullScreen(int arg)
{
  int *temp;

  if (this->FullScreen == arg)
    {
    return;
    }

  if (!this->Mapped)
    {
    this->PrefFullScreen();
    return;
    }

  // set the mode
  this->FullScreen = arg;
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

//--------------------------------------------------------------------------
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
void vtkCarbonRenderWindow::SetStereoCapableWindow(int capable)
{
  if (!this->ContextId && !this->Internal->OffScreenContextId)
    {
    vtkRenderWindow::SetStereoCapableWindow(capable);
    }
  else
    {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
    }
}

//--------------------------------------------------------------------------
// Set the preferred window size to full screen.
void vtkCarbonRenderWindow::PrefFullScreen()
{
  vtkWarningMacro(<< "Can't get full screen window.");
}

//--------------------------------------------------------------------------
// Remap the window.
void vtkCarbonRenderWindow::WindowRemap()
{
  vtkWarningMacro(<< "Can't remap the window.");
}

//--------------------------------------------------------------------------
void vtkCarbonRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
  os << indent << "WindowId: " << this->WindowId << "\n";
  os << indent << "ParentId: " << this->ParentId << "\n";
  os << indent << "RootWindow: " << this->RootWindow << "\n";
}

//--------------------------------------------------------------------------
int vtkCarbonRenderWindow::GetDepthBufferSize()
{
  GLint size;

  if ( this->Mapped )
    {
    size = 0;
    glGetIntegerv( GL_DEPTH_BITS, &size );
    return (int) size;
    }
  else
    {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    return 24;
    }
}

//--------------------------------------------------------------------------
// Get the window id.
HIViewRef vtkCarbonRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n");
  return this->WindowId;
}

//--------------------------------------------------------------------------
// Set the window id to a pre-existing window.
void vtkCarbonRenderWindow::SetWindowId(HIViewRef theWindow)
{
  vtkDebugMacro(<< "Setting WindowId to " << theWindow << "\n");
  this->WindowId = theWindow;
}

//--------------------------------------------------------------------------
// Set this RenderWindow's Carbon window id to a pre-existing window.
void vtkCarbonRenderWindow::SetWindowInfo(char *info)
{
  long tmp;

  sscanf(info,"%ld",&tmp);

  this->WindowId = (HIViewRef)tmp;
  vtkDebugMacro(<< "Setting WindowId to " << this->WindowId << "\n");
}

void vtkCarbonRenderWindow::SetRootWindow(WindowPtr win)
{
  vtkDebugMacro(<< "Setting RootWindow to " << win << "\n");
  this->RootWindow = win;
}

WindowPtr vtkCarbonRenderWindow::GetRootWindow()
{
  // take into account whether the user set the root window or not.
  // if not, then WindowId is set and we're using HIViews.
  // Instead of storing the RootWindow, we ask for it in case of a dynamic
  // GUI where the root window can change.
  if(HIViewGetWindow != NULL && !this->RootWindow)
    {
    return HIViewGetWindow(this->WindowId);
    }
  return this->RootWindow;
}

//----------------------------------------------------------------------------
void vtkCarbonRenderWindow::HideCursor()
{
  if (this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 1;
  CGDisplayHideCursor(CGMainDisplayID());
}

//----------------------------------------------------------------------------
void vtkCarbonRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 0;
  CGDisplayShowCursor(CGMainDisplayID());
}

OSStatus vtkCarbonRenderWindow::RegionEventProcessor(EventHandlerCallRef,
                                                     EventRef event, void* win)
{
  vtkCarbonRenderWindow *vtk_win=reinterpret_cast<vtkCarbonRenderWindow*>(win);
  UInt32 event_kind = GetEventKind(event);
  UInt32 event_class = GetEventClass(event);

  switch(event_class)
    {
    case kEventClassControl:
    {
    switch (event_kind)
      {
      case kEventControlVisibilityChanged:
      case kEventControlOwningWindowChanged:
      case kEventControlBoundsChanged:
        vtk_win->UpdateGLRegion();
        break;
      default:
        break;
      }
    }
    break;
    default:
      break;
    }

  return eventNotHandledErr;
}

