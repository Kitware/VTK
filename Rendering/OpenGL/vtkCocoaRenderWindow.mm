/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCocoaRenderWindow.mm

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import <Cocoa/Cocoa.h>
#import "vtkCocoaMacOSXSDKCompatibility.h" // Needed to support old SDKs

#import "vtkCocoaRenderWindow.h"
#import "vtkCommand.h"
#import "vtkIdList.h"
#import "vtkObjectFactory.h"
#import "vtkRendererCollection.h"
#import "vtkCocoaGLView.h"

#import <vtksys/ios/sstream>

vtkStandardNewMacro(vtkCocoaRenderWindow);

//----------------------------------------------------------------------------
// For fullscreen, an NSWindow that captures key events even when borderless
@interface vtkCocoaFullScreenWindow : NSWindow
{
}
@end

@implementation vtkCocoaFullScreenWindow
- (BOOL)canBecomeKeyWindow
{
  return YES;
}
@end

//----------------------------------------------------------------------------
vtkCocoaRenderWindow::vtkCocoaRenderWindow()
{
  // First, create the cocoa objects manager. The dictionary is empty so
  // essentially all objects are initialized to NULL.
  NSMutableDictionary * cocoaManager = [NSMutableDictionary dictionary];

  // SetCocoaManager works like an Obj-C setter, so do like Obj-C and
  // init the ivar to null first.
  this->CocoaManager = NULL;
  this->SetCocoaManager(reinterpret_cast<void *>(cocoaManager));
  [cocoaManager self]; // prevent premature collection.

  this->WindowCreated = 0;
  this->ViewCreated = 0;
  this->SetWindowName("Visualization Toolkit - Cocoa");
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->Capabilities = 0;
  this->OnScreenInitialized = 0;
  this->OffScreenInitialized = 0;
}

//----------------------------------------------------------------------------
vtkCocoaRenderWindow::~vtkCocoaRenderWindow()
{
  if (this->CursorHidden)
    {
    this->ShowCursor();
    }
  this->Finalize();

  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
    ren->SetRenderWindow(NULL);
    }

  delete[] this->Capabilities;
  this->Capabilities = 0;

  // Release the cocoa object manager.
  this->SetCocoaManager(NULL);
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::Finalize()
{
  if(this->OffScreenInitialized)
    {
    this->OffScreenInitialized = 0;
    this->DestroyOffScreenWindow();
    }
  if(this->OnScreenInitialized)
    {
    this->OnScreenInitialized = 0;
    this->DestroyWindow();
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::DestroyWindow()
{
  GLuint txId;

  // finish OpenGL rendering
  if (this->OwnContext && this->GetContextId())
    {
    this->MakeCurrent();

    // now delete all textures
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
  }
  this->SetContextId(NULL);
  this->SetPixelFormat(NULL);

  if (this->WindowCreated)
    {
    NSWindow* win = (NSWindow*)this->GetRootWindow();
    [win close];
    this->WindowCreated = 0;
    }

  this->SetWindowId(NULL);
  this->SetParentId(NULL);
  this->SetRootWindow(NULL);
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetWindowName( const char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  if (this->GetRootWindow())
    {
    NSString* winTitleStr = [NSString stringWithCString:_arg
                                               encoding:NSASCIIStringEncoding];

    [(NSWindow*)this->GetRootWindow() setTitle:winTitleStr];
    }
}

//----------------------------------------------------------------------------
bool vtkCocoaRenderWindow::InitializeFromCurrentContext()
{
  NSOpenGLContext *currentContext = [NSOpenGLContext currentContext];
  if (currentContext != NULL)
    {
    NSView *currentView = [currentContext view];
    if (currentView != NULL)
      {
      NSWindow *window = [currentView window];
      this->SetWindowId(currentView);
      this->SetRootWindow(window);
      this->SetContextId((void*)currentContext);
      this->OpenGLInit();
      this->OwnContext = 0;
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::GetEventPending()
{
  return 0;
}

//----------------------------------------------------------------------------
// Initialize the rendering process.
void vtkCocoaRenderWindow::Start()
{
  this->Initialize();

  // set the current window
  this->MakeCurrent();
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::MakeCurrent()
{
  if (this->GetContextId())
    {
    [(NSOpenGLContext*)this->GetContextId() makeCurrentContext];
    }
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkCocoaRenderWindow::IsCurrent()
{
  bool result=false;
  if(this->GetContextId()!=0)
    {
    result=static_cast<NSOpenGLContext *>(this->GetContextId())==
      [NSOpenGLContext currentContext];
    }
  return result;
}

//----------------------------------------------------------------------------
bool vtkCocoaRenderWindow::IsDrawable()
{
  // you must initialize it first
  // else it always evaluates false
  this->Initialize();

  // first check that window is valid
  NSView *theView = (NSView*)this->GetWindowId();
  bool win =[[theView window] windowNumber]>0;

  // then check that the drawable is valid
  NSOpenGLContext *context = (NSOpenGLContext *)this->GetContextId();
  bool ok  = [context view] != nil;
  return win && ok;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::UpdateContext()
{
  if (this->GetContextId())
    {
    [(NSOpenGLContext*)this->GetContextId() update];
    }
}

//----------------------------------------------------------------------------
const char* vtkCocoaRenderWindow::ReportCapabilities()
{
  this->MakeCurrent();

  const char* glVendor = (const char*) glGetString(GL_VENDOR);
  const char* glRenderer = (const char*) glGetString(GL_RENDERER);
  const char* glVersion = (const char*) glGetString(GL_VERSION);
  const char* glExtensions = (const char*) glGetString(GL_EXTENSIONS);

  vtksys_ios::ostringstream strm;
  strm << "OpenGL vendor string:  " << glVendor
       << "\nOpenGL renderer string:  " << glRenderer
       << "\nOpenGL version string:  " << glVersion
       << "\nOpenGL extensions:  " << glExtensions << endl;

  // Obtain the OpenGL context in order to keep track of the current screen.
  NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
  GLint currentScreen = [context currentVirtualScreen];

  // The NSOpenGLPixelFormat can only be queried for one particular
  // attribute at a time. Just make repeated queries to get the
  // pertinent settings.
  NSOpenGLPixelFormat* pixelFormat = (NSOpenGLPixelFormat*)this->GetPixelFormat();
  strm << "PixelFormat Descriptor:" << endl;
  GLint pfd = 0;
  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAColorSize forVirtualScreen: currentScreen];
  strm  << "  colorSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAAlphaSize forVirtualScreen: currentScreen];
  strm  << "  alphaSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAStencilSize forVirtualScreen: currentScreen];
  strm  << "  stencilSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFADepthSize forVirtualScreen: currentScreen];
  strm  << "  depthSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAAccumSize forVirtualScreen: currentScreen];
  strm  << "  accumSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFADoubleBuffer forVirtualScreen: currentScreen];
  strm  << "  double buffer:  " << (pfd == 0 ? "No" : "Yes") << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAStereo forVirtualScreen: currentScreen];
  strm  << "  stereo:  " << (pfd == 0 ? "No" : "Yes") << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAStencilSize forVirtualScreen: currentScreen];
  strm  << "  stencil:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAAccelerated forVirtualScreen: currentScreen];
  strm  << "  hardware acceleration::  " << (pfd == 0 ? "No" : "Yes") << endl;

  delete[] this->Capabilities;

  size_t len = strm.str().length() + 1;
  this->Capabilities = new char[len];
  strlcpy(this->Capabilities, strm.str().c_str(), len);

  return this->Capabilities;
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::SupportsOpenGL()
{
  this->MakeCurrent();
  if (!this->GetContextId() || !this->GetPixelFormat())
    {
    return 0;
    }

  NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
  GLint currentScreen = [context currentVirtualScreen];

  NSOpenGLPixelFormat* pixelFormat = (NSOpenGLPixelFormat*)this->GetPixelFormat();
  GLint pfd = 0;
  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFACompliant forVirtualScreen: currentScreen];

  int supportsOpenGL = (pfd == 0) ? 0 : 1;
  return supportsOpenGL;
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::IsDirect()
{
  this->MakeCurrent();
  if (!this->GetContextId() || !this->GetPixelFormat())
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetSize(int* a)
{
  this->SetSize( a[0], a[1] );
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetSize(int x, int y)
{
  static int resizing = 0;

  if ((this->Size[0] != x) || (this->Size[1] != y) || (this->GetParentId()))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    if (this->GetParentId() && this->GetWindowId() && this->Mapped)
      {
      // Set the NSView size, not the window size.
      if (!resizing)
        {
        resizing = 1;
        NSView *theView = (NSView*)this->GetWindowId();
        NSRect viewRect = [theView frame];
        CGFloat oldHeight = NSHeight(viewRect);
        CGFloat height = (CGFloat)y;
        CGFloat width = (CGFloat)x;
        CGFloat xpos = NSMinX(viewRect);
        CGFloat ypos = NSMinY(viewRect) - (height - oldHeight);
        NSRect theRect = NSMakeRect(xpos, ypos, width, height);
        [theView setFrame:theRect];
        [theView setNeedsDisplay:YES];
        resizing = 0;
        }
      }
    else if (this->GetRootWindow() && this->Mapped)
      {
      if (!resizing)
        {
        resizing = 1;
        NSSize theSize = NSMakeSize((CGFloat)x, (CGFloat)y);
        [(NSWindow*)this->GetRootWindow() setContentSize:theSize];
        resizing = 0;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetPosition(int* a)
{
  this->SetPosition( a[0], a[1] );
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetPosition(int x, int y)
{
  static int resizing = 0;

  if ((this->Position[0] != x) || (this->Position[1] != y)
      || (this->GetParentId()))
    {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    if (this->GetParentId() && this->GetWindowId() && this->Mapped)
      {
      // Set the NSView position relative to the parent
      if (!resizing)
        {
        resizing = 1;
        NSRect parentRect = [(NSView*)this->GetParentId() frame];
        NSView *theView = (NSView*)this->GetWindowId();
        NSRect viewRect = [theView frame];
        CGFloat parentHeight = NSHeight(parentRect);
        CGFloat height = NSHeight(viewRect);
        CGFloat xpos = (CGFloat)x;
        CGFloat ypos = parentHeight - height - (CGFloat)y;
        NSPoint origin = NSMakePoint(xpos,ypos);
        [theView setFrameOrigin:origin];
        [theView setNeedsDisplay:YES];
        resizing = 0;
        }
      }
    else if (this->GetRootWindow() && this->Mapped)
      {
      if (!resizing)
        {
        resizing = 1;
        NSPoint origin = NSMakePoint((CGFloat)x, (CGFloat)y);
        [(NSWindow*)this->GetRootWindow() setFrameOrigin:origin];
        resizing = 0;
        }
      }
    }
}

//----------------------------------------------------------------------------
// End the rendering process and display the image.
void vtkCocoaRenderWindow::Frame()
{
  this->MakeCurrent();

  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
    {
    [(NSOpenGLContext*)this->GetContextId() flushBuffer];
    }
   else
    {
    glFlush();
    }
}

//----------------------------------------------------------------------------
// Update system if needed due to stereo rendering.
void vtkCocoaRenderWindow::StereoUpdate()
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType)
      {
      case VTK_STEREO_CRYSTAL_EYES:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_CHECKERBOARD:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
        this->StereoStatus = 1;
        break;
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType)
      {
      case VTK_STEREO_CRYSTAL_EYES:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_CHECKERBOARD:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
        this->StereoStatus = 0;
        break;
      }
    }
}

//----------------------------------------------------------------------------
// Specify various window parameters.
void vtkCocoaRenderWindow::WindowConfigure()
{
  // this is all handled by the desiredVisualInfo method
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetupPixelFormat(void*, void*, int, int, int)
{
  vtkErrorMacro(<< "vtkCocoaRenderWindow::SetupPixelFormat - IMPLEMENT");
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetupPalette(void*)
{
  vtkErrorMacro(<< "vtkCocoaRenderWindow::SetupPalette - IMPLEMENT");
}

//----------------------------------------------------------------------------
// Initialize the window for rendering.
void vtkCocoaRenderWindow::CreateAWindow()
{
  static unsigned count = 1;

  // As vtk is both crossplatform and a library, we don't know if it is being
  // used in a 'regular Cocoa application' or as a 'pure vtk application'.
  // By the former I mean a regular Cocoa application that happens to have
  // a vtkCocoaGLView, by the latter I mean an application that only uses
  // vtk APIs (which happen to use Cocoa as an implementation detail).
  // Specifically, we can't know if NSApplicationMain() was ever called
  // (which is usually done in main()), nor whether the NSApplication exists.
  //
  // So here we call +sharedApplication which will create the NSApplication
  // if it does not exist.  If it does exist, this does nothing.
  // We are not actually interested in the return value.
  // This call is intentionally delayed until this CreateAWindow call
  // to prevent Cocoa-window related stuff from happening in scenarios
  // where vtkRenderWindows are created but never shown.
  (void)[NSApplication sharedApplication];

  // create an NSWindow only if neither an NSView nor an NSWindow have
  // been specified already.  This is the case for a 'pure vtk application'.
  // If you are using vtk in a 'regular Mac application' you should call
  // SetRootWindow() and SetWindowId() so that a window is not created here.
  if (!this->GetRootWindow() && !this->GetWindowId() && !this->GetParentId())
    {
    NSWindow* theWindow = nil;

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
    NSScreen *screen = [NSScreen mainScreen];
    if (this->FullScreen && screen)
      {
      NSRect ctRect = [screen frame];
      this->Size[0] = (int)round(NSWidth(ctRect));
      this->Size[1] = (int)round(NSHeight(ctRect));

      theWindow = [[[vtkCocoaFullScreenWindow alloc]
                    initWithContentRect:ctRect
                              styleMask:NSBorderlessWindowMask
                                backing:NSBackingStoreBuffered
                                  defer:NO] autorelease];

      // This will hide the menu and the dock
      [theWindow setLevel:NSMainMenuWindowLevel+1];
      // This will show the menu and the dock
      //[theWindow setLevel:NSFloatingWindowLevel];
      }
    else
#endif
      {
      if ((this->Size[0]+this->Size[1]) == 0)
        {
        this->Size[0] = 300;
        this->Size[1] = 300;
        }
      if ((this->Position[0]+this->Position[1]) == 0)
        {
        this->Position[0] = 50;
        this->Position[1] = 50;
        }

      NSRect ctRect = NSMakeRect((CGFloat)this->Position[0],
                                 (CGFloat)this->Position[1],
                                 (CGFloat)this->Size[0],
                                 (CGFloat)this->Size[1]);

      theWindow = [[[NSWindow alloc]
                    initWithContentRect:ctRect
                              styleMask:NSTitledWindowMask |
                                        NSClosableWindowMask |
                                        NSMiniaturizableWindowMask |
                                        NSResizableWindowMask
                                backing:NSBackingStoreBuffered
                                  defer:NO] autorelease];
      }

    if (!theWindow)
      {
      vtkErrorMacro("Could not create window, serious error!");
      return;
      }

    this->SetRootWindow(theWindow);
    this->WindowCreated = 1;

    // make key and order front will show the window
    // we don't want this if offscreen was requested
    if(!this->OffScreenRendering)
      {
      [theWindow makeKeyAndOrderFront:nil];
      [theWindow setAcceptsMouseMovedEvents:YES];
      }
  }
  // create a view if one has not been specified
  if (!this->GetWindowId())
    {
    if (this->GetParentId())
      {
      NSView *parent = (NSView*)this->GetParentId();
      NSRect parentRect = [parent frame];
      CGFloat parentHeight = NSHeight(parentRect);
      CGFloat parentWidth = NSWidth(parentRect);
      CGFloat width = (CGFloat)this->Size[0];
      CGFloat height = (CGFloat)this->Size[1];
      CGFloat x = (CGFloat)this->Position[0];
      CGFloat y = parentHeight - height - (CGFloat)this->Position[1];

      // A whole bunch of sanity checks: frame must be inside parent
      if (x > parentWidth - 1) { x = parentWidth - 1; };
      if (y > parentHeight - 1) { y = parentHeight - 1; };
      if (x < 0.0) { x = 0.0; }
      if (y < 0.0) { y = 0.0; }
      if (x + width > parentWidth) { width = parentWidth - x; }
      if (y + height > parentWidth) { height = parentHeight - y; }

      // Don't use vtkCocoaGLView, because if we are in Tk (which is what
      // SetParentId() was added for) then the Tk superview handles the events.
      NSRect glRect = NSMakeRect(x, y, width, height);
      NSView *glView = [[[NSView alloc] initWithFrame:glRect] autorelease];
      [parent addSubview:glView];
      this->SetWindowId(glView);
      this->ViewCreated = 1;
      }
    else
      {
      NSRect glRect = NSMakeRect(0.0, 0.0,
                                 (CGFloat)this->Size[0],
                                 (CGFloat)this->Size[1]);

      // Create a vtkCocoaGLView.
      vtkCocoaGLView *glView =
        [[[vtkCocoaGLView alloc] initWithFrame:glRect] autorelease];
      [(NSWindow*)this->GetRootWindow() setContentView:glView];
      this->SetWindowId(glView);
      this->ViewCreated = 1;
      [glView setVTKRenderWindow:this];
      }
    }

  this->CreateGLContext();

  // Change the window title, but only if it was created by vtk
  if (this->WindowCreated)
    {
    NSString * winName = [NSString stringWithFormat:@"Visualization Toolkit - Cocoa #%u", count++];
    this->SetWindowName([winName cStringUsingEncoding:NSASCIIStringEncoding]);
    }

  // the error "invalid drawable" in the console from this call can appear
  // but only early in the app's lifetime (ie sometime during launch)
  // IMPORTANT: this is necessary to update the context here in case of
  // hardware offscreen rendering
  NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
  [context setView:(NSView*)this->GetWindowId()];

  [context update];

  this->MakeCurrent();

  // wipe out any existing display lists
  vtkRenderer *renderer;
  vtkCollectionSimpleIterator rsit;

  for ( this->Renderers->InitTraversal(rsit);
        (renderer = this->Renderers->GetNextRenderer(rsit));)
    {
    renderer->SetRenderWindow(0);
    renderer->SetRenderWindow(this);
    }
  this->OpenGLInit();
  this->Mapped = 1;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::CreateGLContext()
{
  // keep trying to get different pixelFormats until successful
  NSOpenGLPixelFormat* pixelFormat = nil;
  while (pixelFormat == nil)
    {
    int i = 0;
    NSOpenGLPixelFormatAttribute attribs[20];

    attribs[i++] = NSOpenGLPFAAccelerated;
    attribs[i++] = NSOpenGLPFADepthSize;
    attribs[i++] = (NSOpenGLPixelFormatAttribute)32;

    if (this->MultiSamples != 0)
      {
      attribs[i++] = NSOpenGLPFASampleBuffers;
      attribs[i++] = (NSOpenGLPixelFormatAttribute)1;
      attribs[i++] = NSOpenGLPFASamples;
      attribs[i++] = (NSOpenGLPixelFormatAttribute)(this->MultiSamples);
      attribs[i++] = NSOpenGLPFAMultisample;
      }

    if (this->DoubleBuffer != 0)
      {
      attribs[i++] = NSOpenGLPFADoubleBuffer;
      }

    attribs[i++] = (NSOpenGLPixelFormatAttribute)0;

    // make sure that size of array was not exceeded
    assert(sizeof(NSOpenGLPixelFormatAttribute)*i < sizeof(attribs));

    pixelFormat = [[[NSOpenGLPixelFormat alloc]
                   initWithAttributes:attribs] autorelease];

    if (pixelFormat == nil)
      {
      if (this->MultiSamples == 0)
        {
        // after trying with no multisamples, we are done
        break;
        }
      else if (this->MultiSamples < 4)
        {
        // next time try with no multisamples
        this->MultiSamples = 0;
        }
      else
        {
        this->MultiSamples /= 2;
        }
      }
    }

  NSOpenGLContext* context = [[[NSOpenGLContext alloc]
                              initWithFormat:pixelFormat
                                shareContext:nil] autorelease];

  // This syncs the OpenGL context to the VBL to prevent tearing
  GLint one = 1;
  [context setValues:&one forParameter:NSOpenGLCPSwapInterval];

  this->SetPixelFormat((void*)pixelFormat);
  this->SetContextId((void*)context);

  [pixelFormat self]; // prevent premature collection.
  [context self]; // prevent premature collection.
}

//----------------------------------------------------------------------------
// Initialize the rendering window.
void vtkCocoaRenderWindow::Initialize ()
{
  if(this->OffScreenRendering)
    {
    // destroy on screen
    if(this->OnScreenInitialized)
      {
      this->DestroyWindow();
      this->OnScreenInitialized = 0;
      }
    // create off screen
    if(!this->OffScreenInitialized)
      {
      int width=((this->Size[0]>0) ? this->Size[0] : 300);
      int height=((this->Size[1]>0) ? this->Size[1] : 300);
      if(!this->CreateHardwareOffScreenWindow(width,height))
        {
        // no other offscreen mode available, do on screen rendering
        this->CreateAWindow();
        }
      this->OffScreenInitialized = 1;
      }
    }
  else
    {
    // destroy off screen
    if(this->OffScreenInitialized)
      {
      this->DestroyOffScreenWindow();
      }
    // create on screen
    if(!this->OnScreenInitialized)
      {
      this->OnScreenInitialized = 1;
      this->CreateAWindow();
      }
    }
  if((this->OnScreenInitialized) && (this->Mapped))
    {
    // the error "invalid drawable" in the console from this call can appear
    // but only early in the app's lifetime (ie sometime during launch)
    // IMPORTANT: this is necessary to update the context here in case of
    // onscreen rendering
    NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
    [context setView:(NSView*)this->GetWindowId()];

    [context update];
    }
}

//-----------------------------------------------------------------------------
void vtkCocoaRenderWindow::DestroyOffScreenWindow()
{
  if(this->OffScreenUseFrameBuffer)
    {
    this->DestroyHardwareOffScreenWindow();
    }
  else
    {
    // on screen
    this->DestroyWindow();
    }
}

//----------------------------------------------------------------------------
// Get the current size of the window.
int *vtkCocoaRenderWindow::GetSize()
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
    {
    return this->Superclass::GetSize();
    }

  // We want to return the size of 'the window'.  But the term 'window'
  // is overloaded. It's really the NSView that vtk draws into, so we
  // return its size.
  NSView* view = (NSView*)this->GetWindowId();
  if (view)
    {
    NSRect frameRect = [view frame];
    this->Size[0] = (int)round(NSWidth(frameRect));
    this->Size[1] = (int)round(NSHeight(frameRect));
    }
  return this->Superclass::GetSize();
}

//----------------------------------------------------------------------------
// Get the current size of the screen in pixels.
int *vtkCocoaRenderWindow::GetScreenSize()
{
  NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
  GLint currentScreen = [context currentVirtualScreen];

  NSScreen* screen = [[NSScreen screens] objectAtIndex:currentScreen];
  NSRect screenRect = [screen frame];

  this->Size[0] = (int)round(NSWidth(screenRect));
  this->Size[1] = (int)round(NSHeight(screenRect));

  return this->Size;
}

//----------------------------------------------------------------------------
// Get the position in screen coordinates of the window.
int *vtkCocoaRenderWindow::GetPosition()
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
    {
    return this->Position;
    }

  if (this->GetParentId() && this->GetWindowId())
    {
    // Get display position of the NSView within its parent
    NSRect parentRect = [(NSView*)this->GetParentId() frame];
    NSRect viewFrameRect = [(NSView*)this->GetWindowId() frame];
    this->Position[0] = int(round(NSMinX(viewFrameRect)));
    this->Position[1] = int(round((NSHeight(parentRect)
                                   - NSHeight(viewFrameRect)
                                   - NSMinY(viewFrameRect))));
    }
  else
    {
    // We want to return the position of 'the window'.  But the term 'window'
    // is overloaded. In this case, it's the position of the NSWindow itself
    // on the screen that we return. We don't much care where the NSView is
    // within the NSWindow.
    NSWindow *window = (NSWindow*)this->GetRootWindow();
    if (window)
      {
      NSRect winFrameRect = [window frame];
      this->Position[0] = (int)NSMinX(winFrameRect);
      this->Position[1] = (int)NSMinY(winFrameRect);
      }
    }

  return this->Position;
}

//----------------------------------------------------------------------------
// Change the window to fill the entire screen.
void vtkCocoaRenderWindow::SetFullScreen(int arg)
{
  if (this->FullScreen == arg)
    {
    return;
    }

  if (!this->Mapped)
    {
    this->FullScreen = arg;
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
    if (this->GetRootWindow())
      {
      int* pos = this->GetPosition();
      this->OldScreen[0] = pos[0];
      this->OldScreen[1] = pos[1];

      this->OldScreen[4] = this->Borders;
      this->PrefFullScreen();
      }
    }

  // remap the window
  this->WindowRemap();

  this->Modified();
}

//----------------------------------------------------------------------------
//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkCocoaRenderWindow::SetStereoCapableWindow(int capable)
{
  if (this->GetContextId() == 0)
    {
    vtkRenderWindow::SetStereoCapableWindow(capable);
    }
  else
    {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
    }
}

//----------------------------------------------------------------------------
// Set the preferred window size to full screen.
void vtkCocoaRenderWindow::PrefFullScreen()
{
  int *size = this->GetScreenSize();
  vtkWarningMacro(<< "Can only set FullScreen before showing window: "
                  << size[0] << 'x' << size[1] << ".");
}

//----------------------------------------------------------------------------
// Remap the window.
void vtkCocoaRenderWindow::WindowRemap()
{
  vtkWarningMacro(<< "Can't remap the window.");
  // Acquire the display and capture the screen.
  // Create the full-screen window.
  // Add the context.
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MultiSamples: " << this->MultiSamples << endl;
  os << indent << "CocoaManager: " << this->GetCocoaManager() << endl;
  os << indent << "RootWindow (NSWindow): " << this->GetRootWindow() << endl;
  os << indent << "WindowId (NSView): " << this->GetWindowId() << endl;
  os << indent << "ParentId: " << this->GetParentId() << endl;
  os << indent << "ContextId: " << this->GetContextId() << endl;
  os << indent << "PixelFormat: " << this->GetPixelFormat() << endl;
  os << indent << "WindowCreated: " << (this->WindowCreated ? "Yes" : "No") << endl;
  os << indent << "ViewCreated: " << (this->ViewCreated ? "Yes" : "No") << endl;
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::GetDepthBufferSize()
{
  if ( this->Mapped )
    {
    GLint size = 0;
    glGetIntegerv( GL_DEPTH_BITS, &size );
    return (int) size;
    }
  else
    {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    return 24;
    }
}

//----------------------------------------------------------------------------
// Returns the NSWindow* associated with this vtkRenderWindow.
void *vtkCocoaRenderWindow::GetRootWindow()
{
  NSMutableDictionary* manager =
    reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
  return reinterpret_cast<void *>([manager objectForKey:@"RootWindow"]);
}

//----------------------------------------------------------------------------
// Sets the NSWindow* associated with this vtkRenderWindow.
void vtkCocoaRenderWindow::SetRootWindow(void *arg)
{
  if (arg != NULL)
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(arg)
                forKey:@"RootWindow"];
    }
  else
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager removeObjectForKey:@"RootWindow"];
    }
}

//----------------------------------------------------------------------------
// Returns the NSView* associated with this vtkRenderWindow.
void *vtkCocoaRenderWindow::GetWindowId()
{
  NSMutableDictionary* manager =
    reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
  return reinterpret_cast<void *>([manager objectForKey:@"WindowId"]);
}

//----------------------------------------------------------------------------
// Sets the NSView* associated with this vtkRenderWindow.
void vtkCocoaRenderWindow::SetWindowId(void *arg)
{
  if (arg != NULL)
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(arg)
                forKey:@"WindowId"];
    }
  else
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager removeObjectForKey:@"WindowId"];
    }
}

//----------------------------------------------------------------------------
// Returns the NSView* that is the parent of this vtkRenderWindow.
void *vtkCocoaRenderWindow::GetParentId()
{
  NSMutableDictionary* manager =
    reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
  return reinterpret_cast<void *>([manager objectForKey:@"ParentId"]);
}

//----------------------------------------------------------------------------
// Sets the NSView* that this vtkRenderWindow should use as a parent.
void vtkCocoaRenderWindow::SetParentId(void *arg)
{
  if (arg != NULL)
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(arg)
                forKey:@"ParentId"];
    }
  else
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager removeObjectForKey:@"ParentId"];
    }
}

//----------------------------------------------------------------------------
// Sets the NSOpenGLContext* associated with this vtkRenderWindow.
void vtkCocoaRenderWindow::SetContextId(void *contextId)
{
  if (contextId != NULL)
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(contextId)
                forKey:@"ContextId"];
    }
  else
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager removeObjectForKey:@"ContextId"];
    }
}

//----------------------------------------------------------------------------
// Returns the NSOpenGLContext* associated with this vtkRenderWindow.
void *vtkCocoaRenderWindow::GetContextId()
{
  NSMutableDictionary* manager =
    reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
  return reinterpret_cast<void *>([manager objectForKey:@"ContextId"]);
}

//----------------------------------------------------------------------------
// Sets the NSOpenGLPixelFormat* associated with this vtkRenderWindow.
void vtkCocoaRenderWindow::SetPixelFormat(void *pixelFormat)
{
  if (pixelFormat != NULL)
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager setObject:reinterpret_cast<id>(pixelFormat)
                forKey:@"PixelFormat"];
    }
  else
    {
    NSMutableDictionary* manager =
      reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
    [manager removeObjectForKey:@"PixelFormat"];
    }
}

//----------------------------------------------------------------------------
// Returns the NSOpenGLPixelFormat* associated with this vtkRenderWindow.
void *vtkCocoaRenderWindow::GetPixelFormat()
{
  NSMutableDictionary* manager =
    reinterpret_cast<NSMutableDictionary *>(this->GetCocoaManager());
  return reinterpret_cast<void *>([manager objectForKey:@"PixelFormat"]);
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetCocoaManager(void *manager)
{
  NSMutableDictionary* currentCocoaManager =
    reinterpret_cast<NSMutableDictionary *>(this->CocoaManager);
  NSMutableDictionary* newCocoaManager =
    reinterpret_cast<NSMutableDictionary *>(manager);

  if (currentCocoaManager != newCocoaManager)
    {
    // Why not use Cocoa's retain and release?  Without garbage collection
    // (GC), the two are equivalent anyway because of 'toll free bridging',
    // so no problem there.  With GC, retain and release do nothing, but
    // CFRetain and CFRelease still manipulate the internal reference count.
    // We need that, since we are not using strong references (we don't want
    // it collected out from under us!).
    if (currentCocoaManager)
      {
      CFRelease(currentCocoaManager);
      }
    if (newCocoaManager)
      {
      this->CocoaManager = const_cast<void *>(CFRetain (newCocoaManager));
      }
    else
      {
      this->CocoaManager = NULL;
      }
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetWindowInfo(char *info)
{
  // The paramater is an ASCII string of a decimal number representing
  // a pointer to the window. Convert it back to a pointer.
  ptrdiff_t tmp = 0;
  if (info)
    {
    (void)sscanf(info, "%tu", &tmp);
    }

  this->SetWindowId (reinterpret_cast<void *>(tmp));
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetParentInfo(char *info)
{
  // The paramater is an ASCII string of a decimal number representing
  // a pointer to the window. Convert it back to a pointer.
  ptrdiff_t tmp = 0;
  if (info)
    {
    (void)sscanf(info, "%tu", &tmp);
    }

  this->SetParentId (reinterpret_cast<void *>(tmp));
}

//----------------------------------------------------------------------------
void *vtkCocoaRenderWindow::GetCocoaManager()
{
  return this->CocoaManager;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::HideCursor()
{
  if (this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 1;

  [NSCursor hide];
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 0;

  [NSCursor unhide];
}

// ---------------------------------------------------------------------------
int vtkCocoaRenderWindow::GetWindowCreated()
{
  return this->WindowCreated;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetCursorPosition(int x, int y)
{
  // The given coordinates are from the bottom left of the view.
  NSPoint newViewPoint = NSMakePoint (x, y);

  // Convert to screen coordinates.
  NSView* view = (NSView*)this->GetWindowId();
  if (view)
    {
    NSPoint screenPoint = [view convertPoint:newViewPoint toView:nil];
    CGPoint newCursorPosition = NSPointToCGPoint(screenPoint);

    // Move the cursor there.
    (void)CGWarpMouseCursorPosition (newCursorPosition);
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetCurrentCursor(int shape)
{
  if ( this->InvokeEvent(vtkCommand::CursorChangedEvent,&shape) )
    {
    return;
    }
  this->Superclass::SetCurrentCursor(shape);
  NSCursor* cursor = nil;
  switch (shape)
    {
    case VTK_CURSOR_DEFAULT:
    case VTK_CURSOR_ARROW:
      cursor = [NSCursor arrowCursor];
      break;
    case VTK_CURSOR_SIZENS:
      cursor = [NSCursor resizeUpDownCursor];
      break;
    case VTK_CURSOR_SIZEWE:
      cursor = [NSCursor resizeLeftRightCursor];
      break;
    case VTK_CURSOR_HAND:
      cursor = [NSCursor pointingHandCursor];
      break;
    case VTK_CURSOR_CROSSHAIR:
      cursor = [NSCursor crosshairCursor];
      break;

    // NSCursor does not have cursors for these.
    case VTK_CURSOR_SIZENE:
    case VTK_CURSOR_SIZESW:
    case VTK_CURSOR_SIZENW:
    case VTK_CURSOR_SIZESE:
    case VTK_CURSOR_SIZEALL:
      cursor = [NSCursor arrowCursor];
      break;
    }

  [cursor set];
}
