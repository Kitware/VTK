/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuartzImageWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/glaux.h>
#endif
#include "vtkQuartzImageWindow.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkQuartzImageWindow* vtkQuartzImageWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkQuartzImageWindow");
  if(ret)
    {
    return (vtkQuartzImageWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkQuartzImageWindow;
}




vtkQuartzImageWindow::vtkQuartzImageWindow()
{
  this->ApplicationInstance = NULL;
  this->Palette = NULL;
  this->ContextId = 0;
  this->WindowId = 0;
  this->ParentId = 0;
  this->NextWindowId = 0;
  this->DeviceContext = 0;		// hsr
  this->SetWindowName("Visualization Toolkit - Quartz");
  // we default to double buffered in contrast to other classes
  // mostly because in OpenGL double buffering should be free
  this->DoubleBuffer = 1;
  this->Erase = 1;
}

vtkQuartzImageWindow::~vtkQuartzImageWindow()
{
  if (this->WindowId && this->OwnWindow)
    {
//    DestroyWindow(this->WindowId);
    }
}

void vtkQuartzImageWindow::Render()
{
  if (this->WindowCreated)
    {
    this->MakeCurrent();
    }
  this->vtkImageWindow::Render();
}

void vtkQuartzImageWindow::Clean()
{
  /* finish OpenGL rendering */
  if (this->ContextId) 
    {
   // wglMakeCurrent(NULL, NULL);
   // wglDeleteContext(this->ContextId);
    this->ContextId = NULL;
    }
  if (this->Palette)
    {
    //SelectPalette(this->DeviceContext, this->OldPalette, FALSE); // SVA delete the old palette
    //DeleteObject(this->Palette);
    this->Palette = NULL;
    }
}

void vtkQuartzImageWindow::SetWindowName( char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  if (this->WindowId)
    {
    //SetWindowText(this->WindowId,this->WindowName);
    }
}

void vtkQuartzImageWindow::MakeCurrent()
{
  //wglMakeCurrent(this->DeviceContext, this->ContextId);
}

void vtkQuartzImageWindow::SetSize(int x, int y)
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
         // SetWindowExtEx(this->DeviceContext,x,y,NULL);
         // SetViewportExtEx(this->DeviceContext,x,y,NULL);
         // SetWindowPos(this->WindowId,HWND_TOP,0,0,
           // x, y, SWP_NOMOVE | SWP_NOZORDER);
          }
        else
          {
         // SetWindowPos(this->WindowId,HWND_TOP,0,0,
           // x+2*GetSystemMetrics(SM_CXFRAME),
           // y+2*GetSystemMetrics(SM_CYFRAME) +GetSystemMetrics(SM_CYCAPTION),
           // SWP_NOMOVE | SWP_NOZORDER);
          }
        resizing = 0;
        }
      }
    }
}

void vtkQuartzImageWindow::SetPosition(int x, int y)
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
   
     //   SetWindowPos(this->WindowId,HWND_TOP,x,y,
    //        0, 0, SWP_NOSIZE | SWP_NOZORDER);
        resizing = 0;
        }
      }
    }
}


// End the rendering process and display the image.
void vtkQuartzImageWindow::SwapBuffers()
{
  glFlush();
  if (this->DoubleBuffer)
    {
    //vtkWin32OpenGLSwapBuffers(this->DeviceContext);
    vtkDebugMacro(<< " SwapBuffers\n");
    }
}

// End the rendering process and display the image.
void vtkQuartzImageWindow::Frame()
{
  glFlush();
  vtkDebugMacro(<< "Frame\n");
  if (this->DoubleBuffer)
    {
    //vtkWin32OpenGLSwapBuffers(this->DeviceContext);
    }
}


void vtkQuartzImageWindow::SetupPixelFormat(void *hDC, int dwFlags, 
						  int debug, int bpp, 
						  int zbpp)
{
//  PIXELFORMATDESCRIPTOR pfd = {
//    sizeof(PIXELFORMATDESCRIPTOR),  /* size */
//    1,                              /* version */
//    dwFlags         ,               /* support double-buffering */
//    PFD_TYPE_RGBA,                  /* color type */
//    bpp,                             /* prefered color depth */
//    0, 0, 0, 0, 0, 0,               /* color bits (ignored) */
//    0,                              /* no alpha buffer */
//    0,                              /* alpha bits (ignored) */
//    0,                              /* no accumulation buffer */
//    0, 0, 0, 0,                     /* accum bits (ignored) */
//    0,                              /* depth buffer */
//    0,                              /* no stencil buffer */
//    0,                              /* no auxiliary buffers */
//    PFD_MAIN_PLANE,                 /* main layer */
//    0,                              /* reserved */
//    0, 0, 0,                        /* no layer, visible, damage masks */
//  };
  int pixelFormat;
}

void vtkQuartzImageWindow::SetupPalette(void *hDC)
{
   // int pixelFormat = GetPixelFormat(hDC);
}

void vtkQuartzImageWindow::OpenGLInit()
{
  glMatrixMode( GL_MODELVIEW );
  glClearColor(0,0,0,1);
  glDisable(GL_DEPTH_TEST);
}





// Initialize the window for rendering.
void vtkQuartzImageWindow::MakeDefaultWindow()
{
  int x, y, width, height;
}

// Get the current size of the window.
int *vtkQuartzImageWindow::GetSize(void)
{
  return this->Size;
}

// Get the position in screen coordinates of the window.
int *vtkQuartzImageWindow::GetPosition(void)
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

void vtkQuartzImageWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageWindow::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "Next Window Id: " << this->NextWindowId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
}


unsigned char *vtkQuartzImageWindow::GetPixelData(int x1, int y1, int x2,
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

void vtkQuartzImageWindow::SetPixelData(int x1, int y1, int x2, int y2,
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
void *vtkQuartzImageWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n"); 

  return this->WindowId;
}

// Set the window id to a pre-existing window.
void vtkQuartzImageWindow::SetWindowId(void *arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << arg << "\n"); 

  this->WindowId = arg;
}

// Set the window id to a pre-existing window.
void vtkQuartzImageWindow::SetParentId(void *arg)
{
  vtkDebugMacro(<< "Setting ParentId to " << arg << "\n"); 

  this->ParentId = arg;
}

// Set the window id of the new window once a WindowRemap is done.
void vtkQuartzImageWindow::SetNextWindowId(void *arg)
{
  vtkDebugMacro(<< "Setting NextWindowId to " << arg << "\n"); 

  this->NextWindowId = arg;
}

void vtkQuartzImageWindow::SetupMemoryRendering(int xsize, int ysize,
						      void *aHdc)
{
  int dataWidth = ((xsize*3+3)/4)*4;
  
//  this->MemoryDataHeader.bmiHeader.biSize = 40;
//  this->MemoryDataHeader.bmiHeader.biWidth = xsize;
//  this->MemoryDataHeader.bmiHeader.biHeight = ysize;
//  this->MemoryDataHeader.bmiHeader.biPlanes = 1;
//  this->MemoryDataHeader.bmiHeader.biBitCount = 24;
//  this->MemoryDataHeader.bmiHeader.biCompression = BI_RGB;
//  this->MemoryDataHeader.bmiHeader.biClrUsed = 0;
//  this->MemoryDataHeader.bmiHeader.biClrImportant = 0;
//  this->MemoryDataHeader.bmiHeader.biSizeImage = dataWidth*ysize;
	
  // try using a DIBsection
  //this->MemoryBuffer = CreateDIBSection(aHdc,
//				&this->MemoryDataHeader, DIB_RGB_COLORS, 
//				(void **)(&(this->MemoryData)),  NULL, 0);
  
  // Create a compatible device context
//  this->MemoryHdc = (HDC)CreateCompatibleDC(aHdc);
//  int cxPage = GetDeviceCaps(aHdc,LOGPIXELSX);
//  int mxPage = GetDeviceCaps(this->MemoryHdc,LOGPIXELSX);
  
  // Put the bitmap into the device context
//  SelectObject(this->MemoryHdc, this->MemoryBuffer);
  
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
//  this->SetupPixelFormat(this->DeviceContext, 
//		PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI | PFD_DRAW_TO_BITMAP,
//		this->GetDebug(), 24, 32);
  this->SetupPalette(this->DeviceContext);
//  this->ContextId = wglCreateContext(this->DeviceContext);
//  wglMakeCurrent(this->DeviceContext, this->ContextId);

  for (this->Imagers->InitTraversal(); (ren = this->Imagers->GetNextItem());)
    {
    ren->SetImageWindow(this);
    }
  this->OpenGLInit();
}

void *vtkQuartzImageWindow::GetMemoryDC()
{
  return this->MemoryHdc;
}


void vtkQuartzImageWindow::ResumeScreenRendering()
{
//  GdiFlush();
//  DeleteDC(this->MemoryHdc); 
//  DeleteObject(this->MemoryBuffer);
  
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
//  wglMakeCurrent(this->DeviceContext, this->ContextId);

  for (this->Imagers->InitTraversal(); (ren = this->Imagers->GetNextItem());)
    {
    ren->SetImageWindow(this);
    }
}

void vtkQuartzImageWindow::SetContextId(void *arg) // hsr
{													   // hsr	
  this->ContextId = arg;							   // hsr
}													   // hsr

void vtkQuartzImageWindow::SetDeviceContext(void *arg) // hsr
{														 // hsr
  this->DeviceContext = arg;							 // hsr
}														 // hsr

float *vtkQuartzImageWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2, int front)
{
  long    xloop,yloop;
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  float   *data = NULL;

  float   *p_data = NULL;
  unsigned long   *buffer;

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
void vtkQuartzImageWindow::ReleaseRGBAPixelData(float *data) 
  {
  delete[] data;
  }

void vtkQuartzImageWindow::SetRGBAPixelData(int x1, int y1, 
                                                  int x2, int y2,
                                                  float *data, int front,
                                                  int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;
  int     xloop,yloop;
  float   *buffer;
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




