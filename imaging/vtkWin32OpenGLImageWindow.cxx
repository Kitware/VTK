/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OpenGLImageWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkWin32OpenGLImageWindow.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkWin32OpenGLImageWindow* vtkWin32OpenGLImageWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32OpenGLImageWindow");
  if(ret)
    {
    return (vtkWin32OpenGLImageWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32OpenGLImageWindow;
}




vtkWin32OpenGLImageWindow::vtkWin32OpenGLImageWindow()
{
  this->ApplicationInstance = NULL;
  this->Palette = NULL;
  this->ContextId = 0;
  this->WindowId = 0;
  this->ParentId = 0;
  this->NextWindowId = 0;
  this->DeviceContext = (HDC)0;		// hsr
  this->SetWindowName("Visualization Toolkit - Win32OpenGL");
  // we default to double buffered in contrast to other classes
  // mostly because in OpenGL double buffering should be free
  this->DoubleBuffer = 1;
  this->Erase = 1;
}

vtkWin32OpenGLImageWindow::~vtkWin32OpenGLImageWindow()
{
  if (this->WindowId && this->OwnWindow)
    {
    DestroyWindow(this->WindowId);
    }
}

void vtkWin32OpenGLImageWindow::Render()
{
  if (this->WindowCreated)
    {
    this->MakeCurrent();
    }
  this->vtkImageWindow::Render();
}

void vtkWin32OpenGLImageWindow::Clean()
{
  /* finish OpenGL rendering */
  if (this->ContextId) 
    {
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

LRESULT APIENTRY vtkWin32OpenGLImageWindow::WndProc(HWND hWnd, UINT message, 
						     WPARAM wParam, 
						     LPARAM lParam)
{
  vtkWin32OpenGLImageWindow *me = 
    (vtkWin32OpenGLImageWindow *)GetWindowLong(hWnd,GWL_USERDATA);

  // forward to actual object
  if (me)
    {
    return me->MessageProc(hWnd, message, wParam, lParam);
    }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void vtkWin32OpenGLImageWindow::SetWindowName( char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  if (this->WindowId)
    {
    SetWindowText(this->WindowId,this->WindowName);
    }
}

// Set this ImageWindow's X window id to a pre-existing window.
void vtkWin32OpenGLImageWindow::SetWindowInfo(char *info)
{
  int tmp;
  
  sscanf(info,"%i",&tmp);
 
  this->WindowId = (HWND)tmp;
  vtkDebugMacro(<< "Setting WindowId to " << this->WindowId << "\n"); 
}

// Sets the HWND id of the window that WILL BE created.
void vtkWin32OpenGLImageWindow::SetParentInfo(char *info)
{
  int tmp;
  
  sscanf(info,"%i",&tmp);
 
  this->ParentId = (HWND)tmp;
  vtkDebugMacro(<< "Setting ParentId to " << this->ParentId << "\n"); 
}

void vtkWin32OpenGLImageWindow::MakeCurrent()
{
  wglMakeCurrent(this->DeviceContext, this->ContextId);
}

void vtkWin32OpenGLImageWindow::SetSize(int x, int y)
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

void vtkWin32OpenGLImageWindow::SetPosition(int x, int y)
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

static void vtkWin32OpenGLSwapBuffers(HDC hdc)
{
  SwapBuffers(hdc);
}

// End the rendering process and display the image.
void vtkWin32OpenGLImageWindow::SwapBuffers()
{
  glFlush();
  if (this->DoubleBuffer)
    {
    vtkWin32OpenGLSwapBuffers(this->DeviceContext);
    vtkDebugMacro(<< " SwapBuffers\n");
    }
}

// End the rendering process and display the image.
void vtkWin32OpenGLImageWindow::Frame()
{
  glFlush();
  vtkDebugMacro(<< "Frame\n");
  if (this->DoubleBuffer)
    {
    vtkWin32OpenGLSwapBuffers(this->DeviceContext);
    }
}


void vtkWin32OpenGLImageWindow::SetupPixelFormat(HDC hDC, DWORD dwFlags, 
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
    0,                              /* depth buffer */
    0,                              /* no stencil buffer */
    0,                              /* no auxiliary buffers */
    PFD_MAIN_PLANE,                 /* main layer */
    0,                              /* reserved */
    0, 0, 0,                        /* no layer, visible, damage masks */
  };
  int pixelFormat;
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
}

void vtkWin32OpenGLImageWindow::SetupPalette(HDC hDC)
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

void vtkWin32OpenGLImageWindow::OpenGLInit()
{
  glMatrixMode( GL_MODELVIEW );
  glClearColor(0,0,0,1);
  glDisable(GL_DEPTH_TEST);
}


LRESULT vtkWin32OpenGLImageWindow::MessageProc(HWND hWnd, UINT message, 
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
        ReleaseDC(this->WindowId, this->DeviceContext);
		this->WindowId = NULL;
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



// Initialize the window for rendering.
void vtkWin32OpenGLImageWindow::MakeDefaultWindow()
{
  int x, y, width, height;
  static int count = 1;
  char *windowName;
  
  x = ((this->Position[0] >= 0) ? this->Position[0] : 5);
  y = ((this->Position[1] >= 0) ? this->Position[1] : 5);
  width = ((this->Size[0] > 0) ? this->Size[0] : 256);
  height = ((this->Size[1] > 0) ? this->Size[1] : 256);

  // create our own window if not already set
  this->OwnWindow = 0;

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
  if (!this->WindowId)
    {
    WNDCLASS wndClass;
    
    int len = strlen( "Visualization Toolkit - Win32OpenGLImage #") 
      + (int)ceil( (double) log10( (double)(count+1) ) )
      + 1; 
    windowName = new char [ len ];
    sprintf(windowName,"Visualization Toolkit - Win32OpenGLImage #%i",count++);
    this->SetWindowName(windowName);
    delete [] windowName;
    
    // has the class been registered ?
    if (!GetClassInfo(this->ApplicationInstance,"vtkOpenGLImage",&wndClass))
      {
      wndClass.style = CS_HREDRAW | CS_VREDRAW;
      wndClass.lpfnWndProc = vtkWin32OpenGLImageWindow::WndProc;
      wndClass.cbClsExtra = 0;
      wndClass.cbWndExtra = 0;
      wndClass.hInstance = this->ApplicationInstance;
      wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
      wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
      wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
      wndClass.lpszMenuName = NULL;
      wndClass.lpszClassName = "vtkOpenGLImage";
      RegisterClass(&wndClass);
      }
    

    /* create window */
    if (this->ParentId)
      {
      this->WindowId = CreateWindow(
	"vtkOpenGLImage", this->WindowName,
	WS_CHILD | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/,
	x, y, width, height,
	this->ParentId, NULL, this->ApplicationInstance, NULL);
      }
    else
      {
      this->WindowId = CreateWindow(
	"vtkOpenGLImage", this->WindowName,
	WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/,
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
    }
  SetWindowLong(this->WindowId,GWL_USERDATA,(LONG)this);
  this->DeviceContext = GetDC(this->WindowId);
  this->SetupPixelFormat(this->DeviceContext, PFD_SUPPORT_OPENGL |
			 PFD_DRAW_TO_WINDOW | 
			 (this->DoubleBuffer*PFD_DOUBLEBUFFER), 
			 this->GetDebug(), 32, 32);
  this->SetupPalette(this->DeviceContext);
  this->ContextId = wglCreateContext(this->DeviceContext);
  wglMakeCurrent(this->DeviceContext, this->ContextId);
  this->OpenGLInit();
  this->Mapped = 1;

  // set the DPI
  this->SetDPI(GetDeviceCaps(this->DeviceContext, LOGPIXELSY));
}

// Get the current size of the window.
int *vtkWin32OpenGLImageWindow::GetSize(void)
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

// Get the position in screen coordinates of the window.
int *vtkWin32OpenGLImageWindow::GetPosition(void)
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

void vtkWin32OpenGLImageWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageWindow::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "Next Window Id: " << this->NextWindowId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
}


unsigned char *vtkWin32OpenGLImageWindow::GetPixelData(int x1, int y1, int x2,
							int y2, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  unsigned char   *data = NULL;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  if (front)
    {
    glReadBuffer(GL_FRONT);
    }
  else
    {
    glReadBuffer(GL_BACK);
    }

  data = new unsigned char[(x_hi - x_low + 1)*(y_hi - y_low + 1)*3];

  // Calling pack alignment ensures that we can grab the any size window
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadPixels(x_low, y_low, x_hi-x_low+1, y_hi-y_low+1, GL_RGB,
               GL_UNSIGNED_BYTE, data);
  return data;
}

void vtkWin32OpenGLImageWindow::SetPixelData(int x1, int y1, int x2, int y2,
					    unsigned char *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  // set the current window
  this->MakeCurrent();

  if (front)
    {
    glDrawBuffer(GL_FRONT);
    }
  else
    {
    glDrawBuffer(GL_BACK);
    }

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }
  
  // now write the binary info
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1), 
                 (2.0 * (GLfloat)(y_low) / this->Size[1] - 1),
                 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();


  glDisable(GL_BLEND);
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  glDrawPixels((x_hi-x_low+1), (y_hi - y_low + 1),
               GL_RGB, GL_UNSIGNED_BYTE, data);
  glEnable(GL_BLEND);
}

// Get the window id.
HWND vtkWin32OpenGLImageWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n"); 

  return this->WindowId;
}

// Set the window id to a pre-existing window.
void vtkWin32OpenGLImageWindow::SetWindowId(HWND arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << arg << "\n"); 

  this->WindowId = arg;
}

// Set the window id to a pre-existing window.
void vtkWin32OpenGLImageWindow::SetParentId(HWND arg)
{
  vtkDebugMacro(<< "Setting ParentId to " << arg << "\n"); 

  this->ParentId = arg;
}

// Set the window id of the new window once a WindowRemap is done.
void vtkWin32OpenGLImageWindow::SetNextWindowId(HWND arg)
{
  vtkDebugMacro(<< "Setting NextWindowId to " << arg << "\n"); 

  this->NextWindowId = arg;
}

void vtkWin32OpenGLImageWindow::SetupMemoryRendering(int xsize, int ysize,
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
  this->MemoryBuffer = CreateDIBSection(aHdc,
				&this->MemoryDataHeader, DIB_RGB_COLORS, 
				(void **)(&(this->MemoryData)),  NULL, 0);
  
  // Create a compatible device context
  this->MemoryHdc = (HDC)CreateCompatibleDC(aHdc);
  int cxPage = GetDeviceCaps(aHdc,LOGPIXELSX);
  int mxPage = GetDeviceCaps(this->MemoryHdc,LOGPIXELSX);
  
  // Put the bitmap into the device context
  SelectObject(this->MemoryHdc, this->MemoryBuffer);
  
  // save the current state
  this->ScreenMapped = this->Mapped;
  this->ScreenWindowSize[0] = this->Size[0];
  this->ScreenWindowSize[1] = this->Size[1];
  this->ScreenDeviceContext = this->DeviceContext;
  this->ScreenDoubleBuffer = this->DoubleBuffer;
  this->ScreenContextId = this->ContextId;
  
  // we need to release resources
  vtkImager *ren;
  for (this->Imagers->InitTraversal(); (ren = this->Imagers->GetNextItem());)
    {
    ren->SetImageWindow(NULL);
    }

  // adjust settings for ImageWindow
  this->Mapped =0;
  this->Size[0] = xsize;
  this->Size[1] = ysize;
  
  this->DeviceContext = this->MemoryHdc;
  this->DoubleBuffer = 0;
  this->SetupPixelFormat(this->DeviceContext, 
		PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI | PFD_DRAW_TO_BITMAP,
		this->GetDebug(), 24, 32);
  this->SetupPalette(this->DeviceContext);
  this->ContextId = wglCreateContext(this->DeviceContext);
  wglMakeCurrent(this->DeviceContext, this->ContextId);

  for (this->Imagers->InitTraversal(); (ren = this->Imagers->GetNextItem());)
    {
    ren->SetImageWindow(this);
    }
  this->OpenGLInit();
}

HDC vtkWin32OpenGLImageWindow::GetMemoryDC()
{
  return this->MemoryHdc;
}


void vtkWin32OpenGLImageWindow::ResumeScreenRendering()
{
  GdiFlush();
  DeleteDC(this->MemoryHdc); 
  DeleteObject(this->MemoryBuffer);
  
  // we need to release resources
  vtkImager *ren;
  for (this->Imagers->InitTraversal(); (ren = this->Imagers->GetNextItem());)
    {
    ren->SetImageWindow(NULL);
    }

  this->Mapped = this->ScreenMapped;
  this->Size[0] = this->ScreenWindowSize[0];
  this->Size[1] = this->ScreenWindowSize[1];
  this->DeviceContext = this->ScreenDeviceContext;
  this->DoubleBuffer = this->ScreenDoubleBuffer;
  this->ContextId = this->ScreenContextId;
  wglMakeCurrent(this->DeviceContext, this->ContextId);

  for (this->Imagers->InitTraversal(); (ren = this->Imagers->GetNextItem());)
    {
    ren->SetImageWindow(this);
    }
}

void vtkWin32OpenGLImageWindow::SetContextId(HGLRC arg) // hsr
{													   // hsr	
  this->ContextId = arg;							   // hsr
}													   // hsr

void vtkWin32OpenGLImageWindow::SetDeviceContext(HDC arg) // hsr
{														 // hsr
  this->DeviceContext = arg;							 // hsr
}														 // hsr

float *vtkWin32OpenGLImageWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  float   *data = NULL;

  float   *p_data = NULL;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  if (front)
    {
    glReadBuffer(GL_FRONT);
    }
  else
    {
    glReadBuffer(GL_BACK);
    }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  data = new float[ (width*height*4) ];

  glReadPixels( x_low, y_low, width, height, GL_RGBA, GL_FLOAT, data);

  return data;
}
void vtkWin32OpenGLImageWindow::ReleaseRGBAPixelData(float *data) 
  {
  delete[] data;
  }

void vtkWin32OpenGLImageWindow::SetRGBAPixelData(int x1, int y1, 
                                                  int x2, int y2,
                                                  float *data, int front,
                                                  int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;
  float   *p_data = NULL;

  // set the current window 
  this->MakeCurrent();

  if (front)
    {
    glDrawBuffer(GL_FRONT);
    }
  else
    {
    glDrawBuffer(GL_BACK);
    }

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }
  
  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  /* write out a row of pixels */
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1), 
                 (2.0 * (GLfloat)(y_low) / this->Size[1] - 1), 
		 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  if (!blend)
    {
    glDisable(GL_BLEND);
    glDrawPixels( width, height, GL_RGBA, GL_FLOAT, data);
    glEnable(GL_BLEND);
    }
  else
    {
    glDrawPixels( width, height, GL_RGBA, GL_FLOAT, data);
    }    

}




