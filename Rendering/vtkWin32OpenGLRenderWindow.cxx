/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OpenGLRenderWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Horst Schreiber for developing this MFC code

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
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
#if defined(_MSC_VER) || defined (__BORLANDC__)
#include <GL/glaux.h>
#else
#include <GL/gl.h>
#endif
#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkWin32RenderWindowInteractor.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkWin32OpenGLRenderWindow* vtkWin32OpenGLRenderWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32OpenGLRenderWindow");
  if(ret)
    {
    return (vtkWin32OpenGLRenderWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32OpenGLRenderWindow;
}




#define VTK_MAX_LIGHTS 8


vtkWin32OpenGLRenderWindow::vtkWin32OpenGLRenderWindow()
{
  this->ApplicationInstance =  NULL;
  this->Palette = NULL;
  this->ContextId = 0;
  this->MultiSamples = 8;
  this->WindowId = 0;
  this->ParentId = 0;
  this->NextWindowId = 0;
  this->DeviceContext = (HDC)0;		// hsr
  this->MFChandledWindow = FALSE;	// hsr
  this->StereoType = VTK_STEREO_CRYSTAL_EYES;  
  this->CursorHidden = 0;
}

vtkWin32OpenGLRenderWindow::~vtkWin32OpenGLRenderWindow()
{
  if (this->CursorHidden)
    {
    this->ShowCursor();
    }

  if (this->WindowId && this->OwnWindow)
    {
    this->Clean();
    ReleaseDC(this->WindowId, this->DeviceContext);
    // can't set WindowId=NULL, needed for DestroyWindow
    this->DeviceContext = NULL;
    
    // clear the extra data before calling destroy
    SetWindowLong(this->WindowId,4,(LONG)0);
    DestroyWindow(this->WindowId);
    }
}

void vtkWin32OpenGLRenderWindow::Clean()
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
    
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(this->ContextId);
    this->ContextId = NULL;
    }
  if (this->Palette)
    {
    SelectPalette(this->DeviceContext, this->OldPalette, FALSE); // SVA delete the old palette
    DeleteObject(this->Palette);
    this->Palette = NULL;
    }
}

LRESULT APIENTRY vtkWin32OpenGLRenderWindow::WndProc(HWND hWnd, UINT message, 
						     WPARAM wParam, 
						     LPARAM lParam)
{
  vtkWin32OpenGLRenderWindow *me = 
    (vtkWin32OpenGLRenderWindow *)GetWindowLong(hWnd,4);

  // forward to actual object
  if (me)
    {
    return me->MessageProc(hWnd, message, wParam, lParam);
    }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void vtkWin32OpenGLRenderWindow::SetWindowName( char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  if (this->WindowId)
    {
    SetWindowText(this->WindowId,this->WindowName);
    }
}

int vtkWin32OpenGLRenderWindow::GetEventPending()
{
  MSG msg;
  
  return PeekMessage(&msg,this->WindowId,WM_LBUTTONDOWN,WM_MBUTTONDOWN,PM_NOREMOVE);
}

// this is a global because we really don't think you are doing
// multithreaded OpenGL rendering anyhow. Most current libraries
// don't support it.
HGLRC vtkWin32OpenGLGlobalContext = 0;

void vtkWin32OpenGLRenderWindow::MakeCurrent()
{
  if (this->ContextId != vtkWin32OpenGLGlobalContext)
    {
    // Try to avoid doing anything (for performance).
    if (this->ContextId)
      {
      wglMakeCurrent(this->DeviceContext, this->ContextId);
      }
    vtkWin32OpenGLGlobalContext = this->ContextId;
    }
}

void vtkWin32OpenGLRenderWindow::SetSize(int x, int y)
{
  static int resizing = 0;

  if ((this->Size[0] != x) || (this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    if (this->OffScreenRendering)
      {
      if (!resizing)
        {
        resizing = 1;
        this->CleanUpOffScreenRendering();
        HDC dc = CreateDC("DISPLAY", 0, 0, 0);
        this->CreateOffScreenDC(x, y, dc);
        DeleteDC(dc);
        resizing = 0;
        }
      }

    else if (this->Mapped)
      {
      if (!resizing)
        {
        resizing = 1;
   
        if (this->ParentId)
          {
          SetWindowExtEx(this->DeviceContext,x,y,NULL);
          SetViewportExtEx(this->DeviceContext,x,y,NULL);
          SetWindowPos(this->WindowId,HWND_TOP,0,0,
            x, y, SWP_NOMOVE | SWP_NOZORDER);
          }
        else
          {
          SetWindowPos(this->WindowId,HWND_TOP,0,0,
            x+2*GetSystemMetrics(SM_CXFRAME),
            y+2*GetSystemMetrics(SM_CYFRAME) +GetSystemMetrics(SM_CYCAPTION),
            SWP_NOMOVE | SWP_NOZORDER);
          }
        resizing = 0;
        }
      }
    }
}

void vtkWin32OpenGLRenderWindow::SetPosition(int x, int y)
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
   
        SetWindowPos(this->WindowId,HWND_TOP,x,y,
            0, 0, SWP_NOSIZE | SWP_NOZORDER);
        resizing = 0;
        }
      }
    }
}

// this function is needed because SwapBuffers is an ivar,
// as well as a Win32 API
inline static void vtkWin32OpenGLSwapBuffers(HDC hdc)
{
  SwapBuffers(hdc);
}

// End the rendering process and display the image.
void vtkWin32OpenGLRenderWindow::Frame(void)
{
  this->MakeCurrent();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
    {
    vtkWin32OpenGLSwapBuffers(this->DeviceContext);
    vtkDebugMacro(<< " SwapBuffers\n");
    }
  else
    {
    glFlush();
    }
}
 
void vtkWin32OpenGLRenderWindow::SetupPixelFormat(HDC hDC, DWORD dwFlags, 
						  int debug, int bpp, 
						  int zbpp)
{
  PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),  /* size */
    1,                              /* version */
    dwFlags         ,               /* support double-buffering */
    PFD_TYPE_RGBA,                  /* color type */
    bpp,                             /* prefered color depth */
    0, 0, 0, 0, 0, 0,               /* color bits (ignored) */
    0,                              /* no alpha buffer */
    0,                              /* alpha bits (ignored) */
    0,                              /* no accumulation buffer */
    0, 0, 0, 0,                     /* accum bits (ignored) */
    zbpp,                           /* depth buffer */
    0,                              /* no stencil buffer */
    0,                              /* no auxiliary buffers */
    PFD_MAIN_PLANE,                 /* main layer */
    0,                              /* reserved */
    0, 0, 0,                        /* no layer, visible, damage masks */
  };
  int pixelFormat;
  // Only try to set pixel format if we do not currently have one
  int currentPixelFormat = GetPixelFormat(hDC);
  // if there is a current pixel format, then make sure it
  // supports OpenGL
  if (currentPixelFormat != 0)
    {
    DescribePixelFormat(hDC, currentPixelFormat,sizeof(pfd), &pfd);
    if (!(pfd.dwFlags & PFD_SUPPORT_OPENGL))
      {
      MessageBox(WindowFromDC(hDC), 
		 "Invalid pixel format, no OpenGL support",
		 "Error",
		 MB_ICONERROR | MB_OK);
      exit(1);
      }         
    }
  else
    {
    // hDC has no current PixelFormat, so 
    pixelFormat = ChoosePixelFormat(hDC, &pfd);
    if (pixelFormat == 0)
      {
      MessageBox(WindowFromDC(hDC), "ChoosePixelFormat failed.", "Error",
		 MB_ICONERROR | MB_OK);
      exit(1);
      }
    DescribePixelFormat(hDC, pixelFormat,sizeof(pfd), &pfd); 
    if (SetPixelFormat(hDC, pixelFormat, &pfd) != TRUE) 
      {
      int err = GetLastError();
      MessageBox(WindowFromDC(hDC), "SetPixelFormat failed.", "Error",
		 MB_ICONERROR | MB_OK);
      exit(1);
      }
    }
  if (debug && (dwFlags & PFD_STEREO) && !(pfd.dwFlags & PFD_STEREO))
    {
    vtkGenericWarningMacro("No Stereo Available!");
    this->StereoCapableWindow = 0;
    }
}

void vtkWin32OpenGLRenderWindow::SetupPalette(HDC hDC)
{
    int pixelFormat = GetPixelFormat(hDC);
    PIXELFORMATDESCRIPTOR pfd;
    LOGPALETTE* pPal;
    int paletteSize;

    DescribePixelFormat(hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    if (pfd.dwFlags & PFD_NEED_PALETTE) {
        paletteSize = 1 << pfd.cColorBits;
    } else {
        return;
    }

    pPal = (LOGPALETTE*)
        malloc(sizeof(LOGPALETTE) + paletteSize * sizeof(PALETTEENTRY));
    pPal->palVersion = 0x300;
    pPal->palNumEntries = paletteSize;

    /* build a simple RGB color palette */
    {
        int redMask = (1 << pfd.cRedBits) - 1;
        int greenMask = (1 << pfd.cGreenBits) - 1;
        int blueMask = (1 << pfd.cBlueBits) - 1;
        int i;

        for (i=0; i<paletteSize; ++i) {
            pPal->palPalEntry[i].peRed =
                    (((i >> pfd.cRedShift) & redMask) * 255) / redMask;
            pPal->palPalEntry[i].peGreen =
                    (((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask;
            pPal->palPalEntry[i].peBlue =
                    (((i >> pfd.cBlueShift) & blueMask) * 255) / blueMask;
            pPal->palPalEntry[i].peFlags = 0;
        }
    }

    this->Palette = CreatePalette(pPal);
    free(pPal);

    if (this->Palette) {
        this->OldPalette = SelectPalette(hDC, this->Palette, FALSE);
        RealizePalette(hDC);
    }
}


LRESULT vtkWin32OpenGLRenderWindow::MessageProc(HWND hWnd, UINT message, 
						WPARAM wParam, LPARAM lParam)
{
  switch (message) 
    {
    case WM_CREATE:
      {
      // nothing to be done here, opengl is initilized after the call to
      // create now
      return 0;
      }
    case WM_DESTROY:
      this->Clean();
      if (this->DeviceContext)
	{
        ReleaseDC(this->WindowId, this->DeviceContext);
	this->DeviceContext = NULL;
	this->WindowId = NULL;
	}
      return 0;
    case WM_SIZE:
        /* track window size changes */
        if (this->ContextId) 
          {
          this->SetSize((int) LOWORD(lParam),(int) HIWORD(lParam));
          return 0;
          }
    case WM_PALETTECHANGED:
        /* realize palette if this is *not* the current window */
        if (this->ContextId && this->Palette && (HWND) wParam != hWnd) 
          {
	  SelectPalette(this->DeviceContext, this->OldPalette, FALSE);
          UnrealizeObject(this->Palette);
          this->OldPalette = SelectPalette(this->DeviceContext, 
					   this->Palette, FALSE);
          RealizePalette(this->DeviceContext);
          this->Render();
          }
        break;
    case WM_QUERYNEWPALETTE:
        /* realize palette if this is the current window */
        if (this->ContextId && this->Palette) 
          {
	  SelectPalette(this->DeviceContext, this->OldPalette, FALSE);
          UnrealizeObject(this->Palette);
          this->OldPalette = SelectPalette(this->DeviceContext, 
					   this->Palette, FALSE);
          RealizePalette(this->DeviceContext);
          this->Render();
          return TRUE;
          }
        break;
    case WM_PAINT:
        {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        if (this->ContextId) 
          {
          this->Render();
          }
        EndPaint(hWnd, &ps);
        return 0;
        }
        break;
    case WM_ERASEBKGND:
      return TRUE;
    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}


void vtkWin32OpenGLRenderWindow::InitializeApplication()
{
  // get the applicaiton instance if we don't have one already
  if (!this->ApplicationInstance)
    {
    // if we have a parent window get the app instance from it
    if (this->ParentId)
      {
      this->ApplicationInstance = (HINSTANCE)GetWindowLong(this->ParentId,GWL_HINSTANCE);
      }
    else
      {
      this->ApplicationInstance = GetModuleHandle(NULL); /*AfxGetInstanceHandle();*/
      }
    }
}

void vtkWin32OpenGLRenderWindow::CreateAWindow(int x, int y, int width,
                                              int height)
{
  static int count=1;
  char *windowName;

  if (!this->WindowId)
    {
    WNDCLASS wndClass;
      
    int len = strlen( "Visualization Toolkit - Win32OpenGL #") 
      + (int)ceil( (double) log10( (double)(count+1) ) )
      + 1; 
    windowName = new char [ len ];
    sprintf(windowName,"Visualization Toolkit - Win32OpenGL #%i",count++);
    this->SetWindowName(windowName);
    delete [] windowName;
      
    // has the class been registered ?
    if (!GetClassInfo(this->ApplicationInstance,"vtkOpenGL",&wndClass))
      {
      wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
      wndClass.lpfnWndProc = vtkWin32OpenGLRenderWindow::WndProc;
      wndClass.cbClsExtra = 0;
      wndClass.hInstance = this->ApplicationInstance;
      wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
      wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
      wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
      wndClass.lpszMenuName = NULL;
      wndClass.lpszClassName = "vtkOpenGL";
      // vtk doesn't use the first extra 4 bytes, but app writers
      // may want them, so we provide them. VTK does use the second 
      // four bytes of extra space.
      wndClass.cbWndExtra = 8;
      RegisterClass(&wndClass);
      }
      
    /* create window */
    if (this->ParentId)
      {
      this->WindowId = CreateWindow(
        "vtkOpenGL", this->WindowName,
        WS_CHILD | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/,
        x, y, width, height,
        this->ParentId, NULL, this->ApplicationInstance, NULL);
      }
    else
      {
      DWORD style;
      if (this->Borders)
        {
        style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/;
        }
      else
        {
        style = WS_POPUP | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/;
        }
      this->WindowId = CreateWindow(
        "vtkOpenGL", this->WindowName, style,
        x,y, width+2*GetSystemMetrics(SM_CXFRAME),
        height+2*GetSystemMetrics(SM_CYFRAME) +GetSystemMetrics(SM_CYCAPTION),
        NULL, NULL, this->ApplicationInstance, NULL);
      }
    if (!this->WindowId)
      {
      vtkErrorMacro("Could not create window, error:  " << GetLastError());
      return;
      }
    // extract the create info
      
    /* display window */
    ShowWindow(this->WindowId, SW_SHOW);
    //UpdateWindow(this->WindowId);
    this->OwnWindow = 1;
    SetWindowLong(this->WindowId,4,(LONG)this);
    }
  this->DeviceContext = GetDC(this->WindowId);
  if (this->StereoCapableWindow)
    {
    this->SetupPixelFormat(this->DeviceContext, PFD_SUPPORT_OPENGL |
                           PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER |
                           PFD_STEREO, this->GetDebug(), 32, 32);
    }
  else
    {
    this->SetupPixelFormat(this->DeviceContext, PFD_SUPPORT_OPENGL |
                           PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
                           this->GetDebug(), 32, 32);
    }
  this->SetupPalette(this->DeviceContext);
  this->ContextId = wglCreateContext(this->DeviceContext);
  wglMakeCurrent(this->DeviceContext, this->ContextId);
  this->OpenGLInit();
  this->Mapped = 1;
}

// Initialize the window for rendering.
void vtkWin32OpenGLRenderWindow::WindowInitialize()
{
  int x, y, width, height;
  x = ((this->Position[0] >= 0) ? this->Position[0] : 5);
  y = ((this->Position[1] >= 0) ? this->Position[1] : 5);
  height = ((this->Size[1] > 0) ? this->Size[1] : 300);
  width = ((this->Size[0] > 0) ? this->Size[0] : 300);

  
  // create our own window if not already set
  this->OwnWindow = 0;
  if (!this->MFChandledWindow)
    {
    this->InitializeApplication();
    this->CreateAWindow(x,y,width,height);
    }	
  else 
    {
    wglMakeCurrent(this->DeviceContext, this->ContextId); // hsr
    this->OpenGLInit();
    }
  
  // set the DPI
  this->SetDPI(GetDeviceCaps(this->DeviceContext, LOGPIXELSY));
}

// Initialize the rendering window.
void vtkWin32OpenGLRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized 
  if (this->ContextId)
    {
    return;
    }

  // now initialize the window
  if (this->OffScreenRendering)
    {
    this->InitializeApplication();
    }
  else
    {
    this->WindowInitialize();
    }
}


// Get the current size of the window.
int *vtkWin32OpenGLRenderWindow::GetSize(void)
{
  RECT rect;

  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return(this->Size);
    }

  //  Find the current window size 
  GetClientRect(this->WindowId, &rect);

  this->Size[0] = rect.right;
  this->Size[1] = rect.bottom;
  
  return this->Size;
}

// Get the current size of the window.
int *vtkWin32OpenGLRenderWindow::GetScreenSize(void)
{
  RECT rect;

  SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
                            
  this->Size[0] = rect.right - rect.left;
  this->Size[1] = rect.bottom - rect.top;
  
  return this->Size;
}

// Get the position in screen coordinates of the window.
int *vtkWin32OpenGLRenderWindow::GetPosition(void)
{
  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return(this->Position);
    }

  //  Find the current window position 
//  x,y,&this->Position[0],&this->Position[1],&child);

  return this->Position;
}

// Change the window to fill the entire screen.
void vtkWin32OpenGLRenderWindow::SetFullScreen(int arg)
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
void vtkWin32OpenGLRenderWindow::SetStereoCapableWindow(int capable)
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
void vtkWin32OpenGLRenderWindow::PrefFullScreen()
{
  int *size;

  size = this->GetScreenSize();

  // use full screen 
  this->Position[0] = 0;
  this->Position[1] = 0;
  this->Size[0] = size[0] - 2*GetSystemMetrics(SM_CXFRAME);
  this->Size[1] = size[1] - 
    2*GetSystemMetrics(SM_CYFRAME) - GetSystemMetrics(SM_CYCAPTION);

  // don't show borders 
  this->Borders = 0;
}

// Remap the window.
void vtkWin32OpenGLRenderWindow::WindowRemap()
{
  short cur_light;

  /* first delete all the old lights */
  for (cur_light = GL_LIGHT0; cur_light < GL_LIGHT0+VTK_MAX_LIGHTS; cur_light++)
    {
    glDisable(cur_light);
    }
  
  // then close the old window 
  if (this->OwnWindow)
    {
    SendMessage(this->WindowId, WM_CLOSE, 0, 0L );
    }
  
  // set the default windowid 
  this->WindowId = this->NextWindowId;
  this->NextWindowId = 0;

  // configure the window 
  this->WindowInitialize();
}

void vtkWin32OpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkOpenGLRenderWindow::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "Next Window Id: " << this->NextWindowId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
}

// Get the window id.
HWND vtkWin32OpenGLRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n"); 

  return this->WindowId;
}

// Set the window id to a pre-existing window.
void vtkWin32OpenGLRenderWindow::SetWindowId(HWND arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << arg << "\n"); 

  this->WindowId = arg;
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkWin32OpenGLRenderWindow::SetWindowInfo(char *info)
{
  int tmp;
  
  sscanf(info,"%i",&tmp);
 
  this->WindowId = (HWND)tmp;
  vtkDebugMacro(<< "Setting WindowId to " << this->WindowId << "\n"); 
}

// Sets the HWND id of the window that WILL BE created.
void vtkWin32OpenGLRenderWindow::SetParentInfo(char *info)
{
  int tmp;
  
  sscanf(info,"%i",&tmp);
 
  this->ParentId = (HWND)tmp;
  vtkDebugMacro(<< "Setting ParentId to " << this->ParentId << "\n"); 
}

// Set the window id to a pre-existing window.
void vtkWin32OpenGLRenderWindow::SetParentId(HWND arg)
{
  vtkDebugMacro(<< "Setting ParentId to " << arg << "\n"); 

  this->ParentId = arg;
}

// Set the window id of the new window once a WindowRemap is done.
void vtkWin32OpenGLRenderWindow::SetNextWindowId(HWND arg)
{
  vtkDebugMacro(<< "Setting NextWindowId to " << arg << "\n"); 

  this->NextWindowId = arg;
}

// Begin the rendering process.
void vtkWin32OpenGLRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
    {
    this->Initialize();
    }

  // set the current window 
  this->MakeCurrent();
}


void vtkWin32OpenGLRenderWindow::SetOffScreenRendering(int offscreen)
{
  if (offscreen == this->OffScreenRendering)
    {
    return;
    }

  this->vtkRenderWindow::SetOffScreenRendering(offscreen);

  if (offscreen)
    {
    int size[2];
    size[0] = (this->Size[0] > 0) ? this->Size[0] : 300;
    size[1] = (this->Size[1] > 0) ? this->Size[1] : 300;

    HDC dc = CreateDC("DISPLAY", 0, 0, 0);
    this->SetupMemoryRendering(size[0], size[1], dc);
    DeleteDC(dc); 
    }
  else
    {
    if (!this->WindowId)
      {
      vtkRenderer* ren;
      this->CleanUpOffScreenRendering();
      this->WindowInitialize();
      for (this->Renderers->InitTraversal(); 
           (ren = this->Renderers->GetNextItem());)
        {
        ren->SetRenderWindow(this);
        }
      this->OpenGLInit();
      if (this->Interactor)
        {
        this->Interactor->ReInitialize();
        }
      this->DoubleBuffer = 1;
      }
    else
      {
      this->ResumeScreenRendering();
      }
    }
}

void vtkWin32OpenGLRenderWindow::CreateOffScreenDC(int xsize, int ysize,
                                                   HDC aHdc)
{
  int dataWidth = ((xsize*3+3)/4)*4;
  
  this->MemoryDataHeader.bmiHeader.biSize = 40;
  this->MemoryDataHeader.bmiHeader.biWidth = xsize;
  this->MemoryDataHeader.bmiHeader.biHeight = ysize;
  this->MemoryDataHeader.bmiHeader.biPlanes = 1;
  this->MemoryDataHeader.bmiHeader.biBitCount = 24;
  this->MemoryDataHeader.bmiHeader.biCompression = BI_RGB;
  this->MemoryDataHeader.bmiHeader.biClrUsed = 0;
  this->MemoryDataHeader.bmiHeader.biClrImportant = 0;
  this->MemoryDataHeader.bmiHeader.biSizeImage = dataWidth*ysize;
	
  // try using a DIBsection
  this->CreateOffScreenDC(
    CreateDIBSection(aHdc,
                     &this->MemoryDataHeader, DIB_RGB_COLORS, 
                     (void **)(&(this->MemoryData)),  NULL, 0), aHdc);
}

void vtkWin32OpenGLRenderWindow::CreateOffScreenDC(HBITMAP hbmp, HDC aHdc)
{
  SIZE sz;
  GetBitmapDimensionEx(hbmp,&sz);

  this->MemoryBuffer = hbmp;
  
  // Create a compatible device context
  this->MemoryHdc = (HDC)CreateCompatibleDC(aHdc);
  int cxPage = GetDeviceCaps(aHdc,LOGPIXELSX);
  int mxPage = GetDeviceCaps(this->MemoryHdc,LOGPIXELSX);
  
  // Put the bitmap into the device context
  SelectObject(this->MemoryHdc, this->MemoryBuffer);
  
  // we need to release resources
  vtkRenderer *ren;
  for (this->Renderers->InitTraversal(); (ren = this->Renderers->GetNextItem());)
    {
    ren->SetRenderWindow(NULL);
    }

  // adjust settings for renderwindow
  this->Mapped =0;
  this->Size[0] = sz.cx;
  this->Size[1] = sz.cy;
  
  this->DeviceContext = this->MemoryHdc;
  this->DoubleBuffer = 0;
  this->SetupPixelFormat(this->DeviceContext, 
			 PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI | 
                         PFD_DRAW_TO_BITMAP, this->GetDebug(), 24, 32);
  this->SetupPalette(this->DeviceContext);
  this->ContextId = wglCreateContext(this->DeviceContext);
  wglMakeCurrent(this->DeviceContext, this->ContextId);
  
  // we need to release resources
  for (this->Renderers->InitTraversal(); 
       (ren = this->Renderers->GetNextItem());)
    {
    ren->SetRenderWindow(this);
    }
  
  this->OpenGLInit();
}

void vtkWin32OpenGLRenderWindow::SetupMemoryRendering(int xsize, int ysize,
						      HDC aHdc)
{
  // save the current state
  this->ScreenMapped = this->Mapped;
  this->ScreenWindowSize[0] = this->Size[0];
  this->ScreenWindowSize[1] = this->Size[1];
  this->ScreenDeviceContext = this->DeviceContext;
  this->ScreenDoubleBuffer = this->DoubleBuffer;
  this->ScreenContextId = this->ContextId;

  this->CreateOffScreenDC(xsize, ysize, aHdc);
}

void vtkWin32OpenGLRenderWindow::SetupMemoryRendering(HBITMAP hbmp)
{
  HDC dc = CreateDC("DISPLAY", 0, 0, 0);

  // save the current state
  this->ScreenMapped = this->Mapped;
  this->ScreenWindowSize[0] = this->Size[0];
  this->ScreenWindowSize[1] = this->Size[1];
  this->ScreenDeviceContext = this->DeviceContext;
  this->ScreenDoubleBuffer = this->DoubleBuffer;
  this->ScreenContextId = this->ContextId;

  this->CreateOffScreenDC(hbmp, dc);
  DeleteDC(dc); 
}

HDC vtkWin32OpenGLRenderWindow::GetMemoryDC()
{
  return this->MemoryHdc;
}

void vtkWin32OpenGLRenderWindow::CleanUpOffScreenRendering()
{
  GdiFlush();
  DeleteDC(this->MemoryHdc); 
  DeleteObject(this->MemoryBuffer);
  
  // we need to release resources
  vtkRenderer *ren;
  for (this->Renderers->InitTraversal(); 
       (ren = this->Renderers->GetNextItem());)
    {
    ren->SetRenderWindow(NULL);
    }
  wglDeleteContext(this->ContextId);
}

void vtkWin32OpenGLRenderWindow::ResumeScreenRendering()
{
  this->CleanUpOffScreenRendering();
  this->Mapped = this->ScreenMapped;
  this->Size[0] = this->ScreenWindowSize[0];
  this->Size[1] = this->ScreenWindowSize[1];
  this->DeviceContext = this->ScreenDeviceContext;
  this->DoubleBuffer = this->ScreenDoubleBuffer;
  this->ContextId = this->ScreenContextId;
  wglMakeCurrent(this->DeviceContext, this->ContextId);

  vtkRenderer* ren;
  for (this->Renderers->InitTraversal(); 
       (ren = this->Renderers->GetNextItem());)
    {
    ren->SetRenderWindow(this);
    }
}

void vtkWin32OpenGLRenderWindow::SetContextId(HGLRC arg) // hsr
{													   // hsr	
  this->ContextId = arg;							   // hsr
}													   // hsr

void vtkWin32OpenGLRenderWindow::SetDeviceContext(HDC arg) // hsr
{														 // hsr
  this->DeviceContext = arg;							 // hsr
  this->MFChandledWindow = TRUE;						 // hsr
}														 // hsr

//----------------------------------------------------------------------------
void vtkWin32OpenGLRenderWindow::HideCursor()
{
  if (this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 1;

  ::ShowCursor(!this->CursorHidden);
}

//----------------------------------------------------------------------------
void vtkWin32OpenGLRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 0;

  ::ShowCursor(!this->CursorHidden);
}				   

