/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OffscreenRenderWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Horst Schreiber for developing this MFC code

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkWin32OffscreenRenderWindow.h"
#include <gl/gl.h>
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkWin32OffscreenRenderWindow* vtkWin32OffscreenRenderWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32OffscreenRenderWindow");
  if(ret)
    {
    return (vtkWin32OffscreenRenderWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32OffscreenRenderWindow;
}

vtkWin32OffscreenRenderWindow::vtkWin32OffscreenRenderWindow()
{
  this->MBpp = 24;
  this->MZBpp = 32; // z buffer bits
  this->MhBitmap = NULL;
  this->MhOldBitmap = NULL;
  this->Size[0] = 256;
  this->Size[1] = 256;
  this->Initialize();
}

vtkWin32OffscreenRenderWindow::~vtkWin32OffscreenRenderWindow()
{
  this->Clean();
}

void vtkWin32OffscreenRenderWindow::Clean()
{
  
  if(this->DeviceContext != NULL)
    {
    if(this->MhOldBitmap != NULL)
      {
      SelectObject(this->DeviceContext, this->MhOldBitmap);
      }
    if(this->MhBitmap != NULL)
      {
      DeleteObject(this->MhBitmap);
      }
    }
  
  vtkWin32OpenGLRenderWindow::Clean();
  
  this->MhOldBitmap = NULL;
  this->MhBitmap = NULL;
}

void vtkWin32OffscreenRenderWindow::SetSize(int x, int y)
{
  static int resizing = 0;

  if ((this->Size[0] != x) || (this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    this->WindowInitialize(); // reset bitmap
    }
}

// Description:
// End the rendering process and display the image.
void vtkWin32OffscreenRenderWindow::Frame(void)
{
  glFlush();
}

void vtkWin32OffscreenRenderWindow::WindowInitialize()
{
  void *pBits;
  vtkRenderer *ren;

  BITMAPINFOHEADER bmi;

  memset(&bmi, 0, sizeof(BITMAPINFOHEADER));

  bmi.biSize = sizeof(BITMAPINFOHEADER);
  bmi.biWidth = this->Size[0];
  bmi.biHeight = this->Size[1];
  bmi.biPlanes = 1;
  bmi.biBitCount = this->MBpp;
  bmi.biCompression = BI_RGB; // no compression

  this->Clean(); // make sure everything's clean

  this->DeviceContext = CreateCompatibleDC(NULL);

  if(this->DeviceContext == NULL)
    {
    vtkErrorMacro(<< "couldn't create compatible DC\n");
    return;
    }

  // make a bitmap to draw to
  this->MhBitmap = CreateDIBSection(this->DeviceContext,
                                    (BITMAPINFO *) &bmi, DIB_RGB_COLORS,
                                    &pBits, NULL, 0);

  if(this->MhBitmap == NULL)
    {
    DWORD dwError = GetLastError();
    vtkErrorMacro(<< "couldn't create dib section Windows error "
                  << dwError << "\n");
    return;
    }

  this->MhOldBitmap = (HBITMAP) SelectObject(this->DeviceContext,
                                             this->MhBitmap);
  this->SetupPixelFormat(this->DeviceContext,
                         PFD_SUPPORT_OPENGL|PFD_DRAW_TO_BITMAP,
                         this->GetDebug(), this->MBpp, this->MZBpp);

  if(this->MBpp < 16)
    {
    // setup palette
    this->SetupPalette(this->DeviceContext);
    }

  this->ContextId = wglCreateContext(this->DeviceContext);

  if(this->ContextId == NULL)
    {
    DWORD dwError = GetLastError();
    vtkErrorMacro(<< "couldn't create rendering context Windows error "
                  << dwError << "\n");
    return;
    }

  this->MakeCurrent();
  this->OpenGLInit();
  this->DoubleBufferOff();
  this->SwapBuffersOff();

  // The clean disassociates the renderers from the render window. 
  // Re-associate the renderer with the render window.
  for (this->Renderers->InitTraversal(); 
       (ren = this->Renderers->GetNextItem()); )
    {
    ren->SetRenderWindow(this);
    }
}

void vtkWin32OffscreenRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkWin32OpenGLRenderWindow::PrintSelf(os,indent);
}

// Description:
// Get the current size of the window.
int *vtkWin32OffscreenRenderWindow::GetSize(void)
{
  return(this->Size);
}
