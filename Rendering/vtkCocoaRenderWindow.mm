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

#include "vtkCocoaRenderWindow.h"
#include "vtkCocoaRenderWindowInteractor.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#import "vtkCocoaWindow.h"
#import "vtkCocoaGLView.h"

#define id Id // since id is a reserved token in ObjC and is used a _lot_ in vtk


vtkCxxRevisionMacro(vtkCocoaRenderWindow, "1.26");
vtkStandardNewMacro(vtkCocoaRenderWindow);


#define VTK_MAX_LIGHTS 8

//----------------------------------------------------------------------------
vtkCocoaRenderWindow::vtkCocoaRenderWindow()
{
  this->ApplicationInitialized = 0;
  this->ContextId = 0;
  this->MultiSamples = 8;
  this->WindowId = 0;
  this->DeviceContext = 0;    // hsr
  this->SetWindowName("Visualization Toolkit - Cocoa");
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->Capabilities = 0;
}

//----------------------------------------------------------------------------
vtkCocoaRenderWindow::~vtkCocoaRenderWindow()
{
  if (this->CursorHidden)
    {
    this->ShowCursor();
    }
  this->Finalize();
  // can't set WindowId=NULL, needed for DestroyWindow
  this->DeviceContext = NULL;
  if (this->WindowId && this->OwnWindow)
    {  
    [(vtkCocoaWindow *)this->WindowId close];
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::Finalize()
{
  vtkRenderer *ren;
  GLuint id;
  
  /* finish OpenGL rendering */
  if (this->ContextId) 
    {
    this->MakeCurrent();

    /* now delete all textures */
    glDisable(GL_TEXTURE_2D);
    for (int i = 1; i < this->TextureResourceIds->GetNumberOfIds(); i++)
      {
      id = (GLuint) this->TextureResourceIds->GetId(i);
#ifdef GL_VERSION_1_1
      if (glIsTexture(id))
        {
        glDeleteTextures(1, &id);
        }
#else
      if (glIsList(id))
        {
        glDeleteLists(id,1);
        }
#endif
      }

    // tell each of the renderers that this render window/graphics context
    // is being removed (the RendererCollection is removed by vtkRenderWindow's
    // destructor)
    this->Renderers->InitTraversal();
    for ( ren = (vtkOpenGLRenderer *) this->Renderers->GetNextItemAsObject();
          ren != NULL;
          ren = (vtkOpenGLRenderer *) this->Renderers->GetNextItemAsObject() )
      {
      ren->SetRenderWindow(NULL);
      }

    [(vtkCocoaWindow *)this->WindowId makeCurrentContext];
    //DOCOCOAwglDeleteContext(this->ContextId);
    this->ContextId = NULL;

//temporary remove the autorelease pool until a better solution is found
//    NSAutoreleasePool *pool = (NSAutoreleasePool *)(this->AutoreleasePool);
//    [pool release];
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetWindowName( const char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  if (this->WindowId)
    {
    [(vtkCocoaWindow  *)this->WindowId setTitle:[NSString stringWithCString:_arg] ];
    }
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::GetEventPending()
{
  return 0;
}

//----------------------------------------------------------------------------
// Begin the rendering process.
void vtkCocoaRenderWindow::Start()
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
    {
    this->Initialize();
    }

  // set the current window 
  this->MakeCurrent();
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::MakeCurrent()
{
  [(vtkCocoaWindow *)this->WindowId makeCurrentContext];
}

//----------------------------------------------------------------------------
const char* vtkCocoaRenderWindow::ReportCapabilities()
{
  this->MakeCurrent();

  const char* glVendor = (const char*) glGetString(GL_VENDOR);
  const char* glRenderer = (const char*) glGetString(GL_RENDERER);
  const char* glVersion = (const char*) glGetString(GL_VERSION);
  const char* glExtensions = (const char*) glGetString(GL_EXTENSIONS);

  ostrstream strm;
  strm << "OpenGL vendor string:  " << glVendor
       << "\nOpenGL renderer string:  " << glRenderer
       << "\nOpenGL version string:  " << glVersion
       << "\nOpenGL extensions:  " << glExtensions << endl;

  // Obtain the OpenGL context in order to keep track of the current screen.
  NSOpenGLContext* context = [[(vtkCocoaWindow*)this->WindowId getvtkCocoaGLView] openGLContext];
  int currentScreen = [context currentVirtualScreen];

  // The NSOpenGLPixelFormat can only be queried for one particular
  // attribute at a time. Just make repeated queries to get the
  // pertinent settings.
  NSOpenGLPixelFormat* pixelFormat = [[(vtkCocoaWindow*)this->WindowId getvtkCocoaGLView] pixelFormat];
  strm << "PixelFormat Descriptor:" << endl;
  long pfd;
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
  strm  << "  double buffer:  " << (pfd == YES ? "Yes" : "No") << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAStereo forVirtualScreen: currentScreen];
  strm  << "  stereo:  " << (pfd == YES ? "Yes" : "No") << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAAccelerated forVirtualScreen: currentScreen];
  strm  << "  hardware acceleration::  " << (pfd == YES ? "Yes" : "No") << endl;

  strm << ends;
  delete[] this->Capabilities;
  this->Capabilities = new char[strlen(strm.str()) + 1];
  strcpy(this->Capabilities, strm.str());
  return this->Capabilities;
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::SupportsOpenGL()
{
  this->MakeCurrent();
  if (!this->ContextId)
    {
    return 0;
    }

  NSOpenGLContext* context = [[(vtkCocoaWindow*)this->WindowId getvtkCocoaGLView] openGLContext];
  int currentScreen = [context currentVirtualScreen];

  NSOpenGLPixelFormat* pixelFormat = [[(vtkCocoaWindow*)this->WindowId getvtkCocoaGLView] pixelFormat];
  long pfd;
  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFACompliant forVirtualScreen: currentScreen];

  return (pfd == YES ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::IsDirect()
{
  this->MakeCurrent();
  if (!this->ContextId)
    {
      return 0;
    }

  NSOpenGLContext* context = [[(vtkCocoaWindow*)this->WindowId getvtkCocoaGLView] openGLContext];
  int currentScreen = [context currentVirtualScreen];

  NSOpenGLPixelFormat* pixelFormat = [[(vtkCocoaWindow*)this->WindowId getvtkCocoaGLView] pixelFormat];
  long pfd;
  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAFullScreen forVirtualScreen: currentScreen];

  return (pfd == YES ? 1 : 0);
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

  if ((this->Size[0] != x) || (this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    if (this->Mapped)
      {
      if (!resizing)
        {
        resizing = 1;
        //22 added since that is the size of the title bar
        NSRect sizeRect = NSMakeRect(this->Position[0],
                                     this->Position[1], this->Size[0],
                                     this->Size[1] + 22);
        [(vtkCocoaWindow *)this->WindowId setFrame:sizeRect display:YES];
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
        NSRect sizeRect = NSMakeRect(this->Position[0],
                                     this->Position[1],
                                     this->Size[0],
                                     this->Size[1]+22);
        [(vtkCocoaWindow *)this->WindowId setFrame:sizeRect display:YES];
        resizing = 0;
        }
      }
    }
}

//----------------------------------------------------------------------------
// End the rendering process and display the image.
void vtkCocoaRenderWindow::Frame()
{
  [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] display];
  if (!this->AbortRender && this->DoubleBuffer)
    {
    //vtkCocoaSwapBuffers(this->DeviceContext);
    vtkDebugMacro(<< " SwapBuffers\n");
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
  cout << "vtkCocoaRenderWindow::SetupPixelFormat - IMPLEMENT\n";
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetupPalette(void*)
{
  cout << "vtkCocoaRenderWindow::SetupPalette - IMPLEMENT\n";
}

//----------------------------------------------------------------------------
// Initialize the window for rendering.
void vtkCocoaRenderWindow::WindowInitialize ()
{
  static int count = 1;
  char *windowName;
  NSRect ctRect;
  NSRect glRect;
  vtkCocoaGLView *glView;
#undef id
#define id Id
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  // create our own window if not already set
  this->OwnWindow = 0;
  // get the application instance if we don't have one already
  if (!this->ApplicationInitialized)
    {
    [NSApplication sharedApplication];
    this->ApplicationInitialized=1;
    }
  if (!this->WindowId)
    {
    int len = strlen( "Visualization Toolkit - Cocoa #")
              + (int)ceil( (double) log10( (double)(count+1) ) ) + 1;
    windowName = new char [ len ];
    sprintf(windowName, "Visualization Toolkit - Cocoa #%i", count++);
    this->SetWindowName(windowName);
    delete [] windowName;
    if ((this->Size[0]+this->Size[1])==0)
      {
      this->Size[0] = 300;
      this->Size[1] = 300;
      }
    if ((this->Position[0]+this->Position[1])==0)
      {
      this->Position[0] = 50;
      this->Position[1] = 50;
      }
    //22 added since that is the size of the title bar
    ctRect = NSMakeRect(this->Position[0],this->Position[1],
                        this->Size[0], this->Size[1]+22);
    glRect = NSMakeRect(0,0,this->Size[0],this->Size[1]);
    /* create window */
        
    this->WindowId = (void *)[[[vtkCocoaWindow alloc] initWithContentRect:ctRect 
                     styleMask:NSTitledWindowMask|NSClosableWindowMask|
                     NSMiniaturizableWindowMask|NSResizableWindowMask 
                     backing:NSBackingStoreBuffered defer:NO] retain];
    if (!this->WindowId)
      {
      vtkErrorMacro("Could not create window, serious error!");
      return;
      }
    [(vtkCocoaWindow *)this->WindowId setFrame:ctRect display:YES];
    [(vtkCocoaWindow *)this->WindowId orderFrontRegardless];
    glView = [[[vtkCocoaGLView alloc] initWithFrame:glRect] retain];
    [(vtkCocoaWindow *)this->WindowId setvtkCocoaGLView:glView];
    [(vtkCocoaWindow *)this->WindowId setAcceptsMouseMovedEvents:YES];
    [(vtkCocoaWindow  *)this->WindowId setTitle:[NSString
    stringWithCString: this->WindowName]];
    [(vtkCocoaWindow *)this->WindowId setVTKRenderWindow:this];
    [(vtkCocoaWindow *)this->WindowId setVTKRenderWindowInteractor:0];
    [glView setVTKRenderWindow:this];
    [glView setVTKRenderWindowInteractor:0];
    [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] display];
    this->OwnWindow = 1;
    this->ContextId = [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView ] getOpenGLContext];
    }

  this->OpenGLInit();
  this->Mapped = 1;
  [pool release];
}

//----------------------------------------------------------------------------
// Initialize the rendering window.
void vtkCocoaRenderWindow::Initialize ()
{
  // make sure we havent already been initialized
  if (this->ContextId)
    {
    return;
    }

  // now initialize the window 
  this->WindowInitialize();
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::UpdateSizeAndPosition(int xPos, int yPos, int xSize, int ySize)
{
  this->Size[0] = xSize;
  this->Size[1] = ySize;
  this->Position[0] = xPos;
  this->Position[1] = yPos;

  this->Modified();
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

  //  Find the current window size
  this->Size[0] = (int) [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] frame].size.width;
  this->Size[1] = (int) [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] frame].size.height;
  return this->Superclass::GetSize();
}

//----------------------------------------------------------------------------
// Get the current size of the screen.
int *vtkCocoaRenderWindow::GetScreenSize()
{
  NSOpenGLContext* context = [[(vtkCocoaWindow*)this->WindowId getvtkCocoaGLView] openGLContext];
  int currentScreen = [context currentVirtualScreen];

  NSScreen* screen = [[NSScreen screens] objectAtIndex: currentScreen];
  NSRect screenRect = [screen frame];
  this->Size[0] = (int)screenRect.size.width;
  this->Size[1] = (int)screenRect.size.height;
  return this->Size;
}

//----------------------------------------------------------------------------
// Get the position in screen coordinates of the window.
int *vtkCocoaRenderWindow::GetPosition()
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
    {
    return(this->Position);
    }

  //  Find the current window position
  this->Position[0] = (int)[(vtkCocoaWindow *)this->WindowId frame].origin.x;
  this->Position[1] = (int)[(vtkCocoaWindow *)this->WindowId frame].origin.y;
  return this->Position;
}

//----------------------------------------------------------------------------
// Change the window to fill the entire screen.
void vtkCocoaRenderWindow::SetFullScreen(int arg)
{
  int *pos;
  
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
      pos = this->GetPosition();      
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
  if (this->WindowId == 0)
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
  vtkWarningMacro(<< "Can't get full screen window of size "
      << size[0] << 'x' << size[1] << ".");
}

//----------------------------------------------------------------------------
// Remap the window.
void vtkCocoaRenderWindow::WindowRemap()
{
  vtkWarningMacro(<< "Can't remap the window.");
  // Aquire the display and capture the screen.
  // Create the full-screen window.
  // Add the context.
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::GetDepthBufferSize()
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

//----------------------------------------------------------------------------
// Get the window id.
void *vtkCocoaRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n"); 

  return this->WindowId;
}

//----------------------------------------------------------------------------
// Set the window id to a pre-existing window.
void vtkCocoaRenderWindow::SetWindowId(void *arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << arg << "\n"); 

  this->WindowId = arg;
}

//----------------------------------------------------------------------------
// Set this RenderWindow's Cocoa window id to a pre-existing window.
void vtkCocoaRenderWindow::SetWindowInfo(void *windowID)
{
  this->WindowId = windowID;
  vtkDebugMacro(<< "Setting WindowId to " << this->WindowId << "\n"); 
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetContextId(void *arg)
{
  this->ContextId = arg;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetDeviceContext(void *arg)
{
  this->DeviceContext = arg;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::RegisterTextureResource (GLuint id)
{
  this->TextureResourceIds->InsertNextId ((int) id);
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

