/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuartzRenderWindow.mm
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
#include "vtkQuartzRenderWindow.h"
#include "vtkQuartzRenderWindowInteractor.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkObjectFactory.h"
//#include "vtkFloatArray.h"
#import "vtkQuartzWindow.h"
#import "vtkQuartzGLView.h"

#define id Id // since id is a reserved token in ObjC and is used a _lot_ in vtk


vtkCxxRevisionMacro(vtkQuartzRenderWindow, "1.14");
vtkStandardNewMacro(vtkQuartzRenderWindow);


#define VTK_MAX_LIGHTS 8


vtkQuartzRenderWindow::vtkQuartzRenderWindow()
{
  this->ApplicationInitialized = 0;
  this->ContextId = 0;
  this->MultiSamples = 8;
  this->WindowId = 0;
  this->DeviceContext = 0;		// hsr
  this->StereoType = 0;  
  this->SetWindowName("Visualization Toolkit - Quartz");
  this->TextureResourceIds = vtkIdList::New();
  this->CursorHidden = 0;
}

vtkQuartzRenderWindow::~vtkQuartzRenderWindow()
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
    
    [(vtkQuartzWindow *)this->WindowId close];
    }
}

void vtkQuartzRenderWindow::Clean()
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

    [(vtkQuartzWindow *)this->WindowId makeCurrentContext];
    //DOQUARTZwglDeleteContext(this->ContextId);
    this->ContextId = NULL;
    }
}

void vtkQuartzRenderWindow::SetWindowName( char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  if (this->WindowId)
    {
    //DOQUARTZSetWindowText(this->WindowId,this->WindowName);
    }
}

int vtkQuartzRenderWindow::GetEventPending()
{
  //MSG msg;
  
  //return PeekMessage(&msg,this->WindowId,WM_LBUTTONDOWN,WM_MBUTTONDOWN,PM_NOREMOVE);
  return 0;
}

// Begin the rendering process.
void vtkQuartzRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
    {
    this->Initialize();
    }

  // set the current window 
  this->MakeCurrent();
}

void vtkQuartzRenderWindow::MakeCurrent()
{
    [(vtkQuartzWindow *)this->WindowId makeCurrentContext];
}

void vtkQuartzRenderWindow::SetSize(int x, int y)
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
	[(vtkQuartzWindow *)this->WindowId setFrame:sizeRect display:YES];
        resizing = 0;
        }
      }
    }
}

void vtkQuartzRenderWindow::SetPosition(int x, int y)
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
        [(vtkQuartzWindow *)this->WindowId setFrame:sizeRect display:YES];
        resizing = 0;
        }
      }
    }
}

//static void vtkQuartzSwapBuffers(void *hdc)
//{
  //modify this to deal only with the particular context!
  //  glutSwapBuffers(); - getting called in drawrect
//  [[(vtkQuartzWindow *)this->WindowId getvtkQuartzGLView] display];
//    cout << "vtkQuartzSwapBuffers\n";
//}

// End the rendering process and display the image.
void vtkQuartzRenderWindow::Frame(void)
{
  [[(vtkQuartzWindow *)this->WindowId getvtkQuartzGLView] display];
  if (!this->AbortRender && this->DoubleBuffer)
    {
    //vtkQuartzSwapBuffers(this->DeviceContext);
    vtkDebugMacro(<< " SwapBuffers\n");
    }
}
 

// Update system if needed due to stereo rendering.
void vtkQuartzRenderWindow::StereoUpdate(void)
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
void vtkQuartzRenderWindow::WindowConfigure()
{
  // this is all handled by the desiredVisualInfo method
}

void vtkQuartzRenderWindow::SetupPixelFormat(void *hDC, void *dwFlags, 
						  int debug, int bpp, 
						  int zbpp)
{
    cout << "vtkQuartzRenderWindow::SetupPixelFormat - IMPLEMENT\n";
}

void vtkQuartzRenderWindow::SetupPalette(void *hDC)
{
cout << "vtkQuartzRenderWindow::SetupPalette - IMPLEMENT\n";
}

void vtkQuartzRenderWindow::OpenGLInit()
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
void vtkQuartzRenderWindow::WindowInitialize (void)
{
  int x, y, width, height;
  static int count = 1;
  char *windowName;
  NSRect ctRect;
  NSRect glRect;
  vtkQuartzGLView *glView;
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
        int len = strlen( "Visualization Toolkit - Quartz #")
        + (int)ceil( (double) log10( (double)(count+1) ) )
        + 1;
        windowName = new char [ len ];
        sprintf(windowName,"Visualization Toolkit - Quartz #%i",count++);
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
        
	this->WindowId = (void *)[[[vtkQuartzWindow alloc] initWithContentRect:ctRect 									styleMask:NSTitledWindowMask|NSClosableWindowMask|
                NSMiniaturizableWindowMask|NSResizableWindowMask 
                backing:NSBackingStoreBuffered defer:NO] retain];
        if (!this->WindowId)
        {
            vtkErrorMacro("Could not create window, serious error!");
            return;
        }
        [(vtkQuartzWindow *)this->WindowId setFrame:ctRect display:YES];
        [(vtkQuartzWindow *)this->WindowId orderFrontRegardless];
        glView = [[[vtkQuartzGLView alloc] initWithFrame:glRect] retain];
        [(vtkQuartzWindow *)this->WindowId setvtkQuartzGLView:glView];
	[(vtkQuartzWindow *)this->WindowId setAcceptsMouseMovedEvents:YES];
	[(vtkQuartzWindow *)this->WindowId setVTKRenderWindow:this];
	[(vtkQuartzWindow *)this->WindowId setVTKRenderWindowInteractor:0];
	[glView setVTKRenderWindow:this];
	[glView setVTKRenderWindowInteractor:0];
	[[(vtkQuartzWindow *)this->WindowId getvtkQuartzGLView] display];
        this->OwnWindow = 1;
    }
    this->OpenGLInit();
    this->Mapped = 1;
}

// Initialize the rendering window.
void vtkQuartzRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized
  if (this->ContextId)
    {
    return;
    }

  // now initialize the window 
  this->WindowInitialize();
}


void vtkQuartzRenderWindow::UpdateSizeAndPosition(int xPos, int yPos, int xSize, int ySize)
{
    this->Size[0]=xSize;
    this->Size[1]=ySize;
    this->Position[0]=xPos;
    this->Position[1]=yPos;
    this->Modified();
}


// Get the current size of the window.
int *vtkQuartzRenderWindow::GetSize(void)
{
    // if we aren't mapped then just return the ivar
    if (!this->Mapped)
    {
        return this->Size;
    }

    //  Find the current window size
    this->Size[0] = (int) [[(vtkQuartzWindow *)this->WindowId getvtkQuartzGLView] frame].size.width;
    this->Size[1] = (int) [[(vtkQuartzWindow *)this->WindowId getvtkQuartzGLView] frame].size.height;
    return this->Size;
}

// Get the current size of the screen.
int *vtkQuartzRenderWindow::GetScreenSize(void)
{
cout << "Inside vtkQuartzRenderWindow::GetScreenSize - MUST IMPLEMENT\n";
  this->Size[0] = 0;
  this->Size[1] = 0;
  
  return this->Size;
}

// Get the position in screen coordinates of the window.
int *vtkQuartzRenderWindow::GetPosition(void)
{
    // if we aren't mapped then just return the ivar
    if (!this->Mapped)
    {
        return(this->Position);
    }

    //  Find the current window position
    this->Position[0] = (int)[(vtkQuartzWindow *)this->WindowId frame].origin.x;
    this->Position[1] = (int)[(vtkQuartzWindow *)this->WindowId frame].origin.y;
    return this->Position;
}

// Change the window to fill the entire screen.
void vtkQuartzRenderWindow::SetFullScreen(int arg)
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
void vtkQuartzRenderWindow::SetStereoCapableWindow(int capable)
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
void vtkQuartzRenderWindow::PrefFullScreen()
{
vtkWarningMacro(<< "Can't get full screen window.");
}

// Remap the window.
void vtkQuartzRenderWindow::WindowRemap()
{
vtkWarningMacro(<< "Can't remap the window.");
}

void vtkQuartzRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
}

int vtkQuartzRenderWindow::GetDepthBufferSize()
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
void *vtkQuartzRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n"); 

  return this->WindowId;
}

// Set the window id to a pre-existing window.
void vtkQuartzRenderWindow::SetWindowId(void *arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << arg << "\n"); 

  this->WindowId = arg;
}

// Set this RenderWindow's Quartz window id to a pre-existing window.
void vtkQuartzRenderWindow::SetWindowInfo(void *windowID)
{
  this->WindowId = windowID;
  vtkDebugMacro(<< "Setting WindowId to " << this->WindowId << "\n"); 
}


void vtkQuartzRenderWindow::SetContextId(void *arg)
{
  this->ContextId = arg;
}

void vtkQuartzRenderWindow::SetDeviceContext(void *arg)
{
  this->DeviceContext = arg;
}

void vtkQuartzRenderWindow::RegisterTextureResource (GLuint id)
{
  this->TextureResourceIds->InsertNextId ((int) id);
}


//----------------------------------------------------------------------------
void vtkQuartzRenderWindow::HideCursor()
{
  if (this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 1;

  //DOQUARTZ::ShowCursor(!this->CursorHidden);
}

//----------------------------------------------------------------------------
void vtkQuartzRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 0;

  //DOQUARTZ::ShowCursor(!this->CursorHidden);
}				   

