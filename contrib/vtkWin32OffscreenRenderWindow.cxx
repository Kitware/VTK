#include "vtkWin32OffscreenRenderWindow.h"
#include <gl/gl.h>

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
