/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaRenderWindow.mm
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Yves Starreveld for developing this class

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "vtkCocoaRenderWindow.h"
#include "vtkCocoaRenderWindowInteractor.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
//#include "vtkFloatArray.h"
#import "vtkCocoaWindow.h"
#import "vtkCocoaGLView.h"

#define id Id // since id is a reserved token in ObjC and is used a _lot_ in vtk


vtkCxxRevisionMacro(vtkCocoaRenderWindow, "1.2");
vtkStandardNewMacro(vtkCocoaRenderWindow);


#define VTK_MAX_LIGHTS 8


vtkCocoaRenderWindow::vtkCocoaRenderWindow()
{
  this->ApplicationInitialized = 0;
  this->ContextId = 0;
  this->MultiSamples = 8;
  this->WindowId = 0;
  this->DeviceContext = 0;		// hsr
  this->StereoType = 0;  
  this->SetWindowName("Visualization Toolkit - Cocoa");
  this->TextureResourceIds = vtkIdList::New();
  this->CursorHidden = 0;
}

vtkCocoaRenderWindow::~vtkCocoaRenderWindow()
{
  if (this->CursorHidden)
  {
      this->ShowCursor();
  }
  if (this->WindowId && this->OwnWindow)
    {
    this->Clean();
    // can't set WindowId=NULL, needed for DestroyWindow
    this->DeviceContext = NULL;
    
    [(vtkCocoaWindow *)this->WindowId close];
    }
}

void vtkCocoaRenderWindow::Clean()
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
    }
}

void vtkCocoaRenderWindow::SetWindowName( char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  if (this->WindowId)
    {
    //DOCocoaSetWindowText(this->WindowId,this->WindowName);
    }
}

int vtkCocoaRenderWindow::GetEventPending()
{
  return 0;
}

// Begin the rendering process.
void vtkCocoaRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
    {
    this->Initialize();
    }

  // set the current window 
  this->MakeCurrent();
}

void vtkCocoaRenderWindow::MakeCurrent()
{
    [(vtkCocoaWindow *)this->WindowId makeCurrentContext];
}

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
				     this->Position[1],
				     this->Size[0],
				     this->Size[1]+22);
	[(vtkCocoaWindow *)this->WindowId setFrame:sizeRect display:YES];
        resizing = 0;
        }
      }
    }
}

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


// End the rendering process and display the image.
void vtkCocoaRenderWindow::Frame(void)
{
  [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] display];
  if (!this->AbortRender && this->DoubleBuffer)
    {
    //vtkCocoaSwapBuffers(this->DeviceContext);
    vtkDebugMacro(<< " SwapBuffers\n");
    }
}
 

// Update system if needed due to stereo rendering.
void vtkCocoaRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
        this->StereoStatus = 1;
	}
	break;
      case VTK_STEREO_RED_BLUE:
	{
        this->StereoStatus = 1;
	}
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
        this->StereoStatus = 0;
	}
	break;
      case VTK_STEREO_RED_BLUE:
	{
        this->StereoStatus = 0;
	}
      }
    }
}

// Specify various window parameters.
void vtkCocoaRenderWindow::WindowConfigure()
{
  // this is all handled by the desiredVisualInfo method
}

void vtkCocoaRenderWindow::SetupPixelFormat(void *hDC, void *dwFlags, 
						  int debug, int bpp, 
						  int zbpp)
{
    cout << "vtkCocoaRenderWindow::SetupPixelFormat - IMPLEMENT\n";
}

void vtkCocoaRenderWindow::SetupPalette(void *hDC)
{
cout << "vtkCocoaRenderWindow::SetupPalette - IMPLEMENT\n";
}

void vtkCocoaRenderWindow::OpenGLInit()
{
  glMatrixMode( GL_MODELVIEW );
  glDepthFunc( GL_LEQUAL );
  glEnable( GL_DEPTH_TEST );
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

  // initialize blending for transparency
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glEnable(GL_BLEND);

  if (this->PointSmoothing)
    {
    glEnable(GL_POINT_SMOOTH);
    }
  else
    {
    glDisable(GL_POINT_SMOOTH);
    }

  if (this->LineSmoothing)
    {
    glEnable(GL_LINE_SMOOTH);
    }
  else
    {
    glDisable(GL_LINE_SMOOTH);
    }

  if (this->PolygonSmoothing)
    {
    glEnable(GL_POLYGON_SMOOTH);
    }
  else
    {
    glDisable(GL_POLYGON_SMOOTH);
    }

  glEnable( GL_NORMALIZE );
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
}


// Initialize the window for rendering.
void vtkCocoaRenderWindow::WindowInitialize (void)
{
  int x, y, width, height;
  static int count = 1;
  char *windowName;
  NSRect ctRect;
  NSRect glRect;
  vtkCocoaGLView *glView;
#undef id
  id item;
#define id Id
  // create our own window if not already set
  this->OwnWindow = 0;
    // get the application instance if we don't have one already
    if (!this->ApplicationInitialized)
        {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[NSApplication sharedApplication];
        this->ApplicationInitialized=1;
        }
    if (!this->WindowId)
    {
        int len = strlen( "Visualization Toolkit - Cocoa #")
        + (int)ceil( (double) log10( (double)(count+1) ) )
        + 1;
        windowName = new char [ len ];
        sprintf(windowName,"Visualization Toolkit - Cocoa #%i",count++);
        this->SetWindowName(windowName);
        delete [] windowName;
        if ((this->Size[0]+this->Size[1])==0)
            {
            this->Size[0]=300;
            this->Size[1]=300;
            }
        if ((this->Position[0]+this->Position[1])==0)
            {
            this->Position[0]=50;
            this->Position[1]=50;
            }
        //22 added since that is the size of the title bar
        ctRect = NSMakeRect(this->Position[0],this->Position[1],
                            this->Size[0], this->Size[1]+22);
        glRect = NSMakeRect(0,0,this->Size[0],this->Size[1]);
        /* create window */
        
	this->WindowId = (void *)[[[vtkCocoaWindow alloc] initWithContentRect:ctRect 									styleMask:NSTitledWindowMask|NSClosableWindowMask|
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
	[(vtkCocoaWindow *)this->WindowId setVTKRenderWindow:this];
	[(vtkCocoaWindow *)this->WindowId setVTKRenderWindowInteractor:0];
	[glView setVTKRenderWindow:this];
	[glView setVTKRenderWindowInteractor:0];
	[[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] display];
        this->OwnWindow = 1;
    }
    this->OpenGLInit();
    this->Mapped = 1;
}

// Initialize the rendering window.
void vtkCocoaRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized
  if (this->ContextId)
    {
    return;
    }

  // now initialize the window 
  this->WindowInitialize();
}


void vtkCocoaRenderWindow::UpdateSizeAndPosition(int xPos, int yPos, int xSize, int ySize)
{
    this->Size[0]=xSize;
    this->Size[1]=ySize;
    this->Position[0]=xPos;
    this->Position[1]=yPos;
    this->Modified();
}


// Get the current size of the window.
int *vtkCocoaRenderWindow::GetSize(void)
{
    // if we aren't mapped then just return the ivar
    if (!this->Mapped)
    {
        return this->Size;
    }

    //  Find the current window size
    this->Size[0] = (int) [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] frame].size.width;
    this->Size[1] = (int) [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] frame].size.height;
    return this->Size;
}

// Get the current size of the screen.
int *vtkCocoaRenderWindow::GetScreenSize(void)
{
cout << "Inside vtkCocoaRenderWindow::GetScreenSize - MUST IMPLEMENT\n";
  this->Size[0] = 0;
  this->Size[1] = 0;
  
  return this->Size;
}

// Get the position in screen coordinates of the window.
int *vtkCocoaRenderWindow::GetPosition(void)
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

// Change the window to fill the entire screen.
void vtkCocoaRenderWindow::SetFullScreen(int arg)
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


// Set the preferred window size to full screen.
void vtkCocoaRenderWindow::PrefFullScreen()
{
vtkWarningMacro(<< "Can't get full screen window.");
}

// Remap the window.
void vtkCocoaRenderWindow::WindowRemap()
{
vtkWarningMacro(<< "Can't remap the window.");
}

void vtkCocoaRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
}

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


// Get the window id.
void *vtkCocoaRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n"); 

  return this->WindowId;
}

// Set the window id to a pre-existing window.
void vtkCocoaRenderWindow::SetWindowId(void *arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << arg << "\n"); 

  this->WindowId = arg;
}

// Set this RenderWindow's Cocoa window id to a pre-existing window.
void vtkCocoaRenderWindow::SetWindowInfo(void *windowID)
{
  this->WindowId = windowID;
  vtkDebugMacro(<< "Setting WindowId to " << this->WindowId << "\n"); 
}


void vtkCocoaRenderWindow::SetContextId(void *arg)
{
  this->ContextId = arg;
}

void vtkCocoaRenderWindow::SetDeviceContext(void *arg)
{
  this->DeviceContext = arg;
}

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

  //DOCocoa::ShowCursor(!this->CursorHidden);
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 0;

  //DOCocoa::ShowCursor(!this->CursorHidden);
}				   

