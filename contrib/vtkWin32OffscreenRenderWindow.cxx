#include "vtkWin32OffscreenRenderWindow.h"
#include <gl/gl.h>

vtkWin32OffscreenRenderWindow::vtkWin32OffscreenRenderWindow()
{
	this->m_bpp = 24;
	this->m_zbpp = 32; // z buffer bits
	this->m_hBitmap = NULL;
	this->m_hOldBitmap = NULL;
	this->Size[0] = 256;
	this->Size[1] = 256;
	Initialize();
}

vtkWin32OffscreenRenderWindow::~vtkWin32OffscreenRenderWindow()
{
  this->Clean();
}

void vtkWin32OffscreenRenderWindow::Clean()
{
  
  if(DeviceContext != NULL)
    {
    if(m_hOldBitmap != NULL)
      {
      SelectObject(DeviceContext, m_hOldBitmap);
      }
    if(m_hBitmap != NULL)
      {
      DeleteObject(m_hBitmap);
      }
    }
  
  vtkWin32OpenGLRenderWindow::Clean();
  
  m_hOldBitmap = NULL;
  m_hBitmap = NULL;
}

void vtkWin32OffscreenRenderWindow::SetSize(int x, int y)
{
	static int resizing = 0;

	if ((this->Size[0] != x) || (this->Size[1] != y))
	{
		this->Modified();
		this->Size[0] = x;
		this->Size[1] = y;
		WindowInitialize(); // reset bitmap
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
	bmi.biBitCount = this->m_bpp;
	bmi.biCompression = BI_RGB; // no compression

	Clean(); // make sure everything's clean

	DeviceContext = CreateCompatibleDC(NULL);

	if(DeviceContext == NULL)
	{
		vtkErrorMacro(<< "couldn't create compatible DC\n");
		return;
	}

	// make a bitmap to draw to
	m_hBitmap = CreateDIBSection(DeviceContext, (BITMAPINFO *) &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);

	if(m_hBitmap == NULL)
	{
		DWORD dwError = GetLastError();
		vtkErrorMacro(<< "couldn't create dib section Windows error " << dwError << "\n");
		return;
	}

	m_hOldBitmap = (HBITMAP) SelectObject(DeviceContext, m_hBitmap);
	SetupPixelFormat(DeviceContext, PFD_SUPPORT_OPENGL|PFD_DRAW_TO_BITMAP, this->GetDebug(), m_bpp, m_zbpp);

	if(m_bpp < 16)
	{
		// setup palette
		SetupPalette(DeviceContext);
	}

	ContextId = wglCreateContext(DeviceContext);

	if(ContextId == NULL)
	{
		DWORD dwError = GetLastError();
		vtkErrorMacro(<< "couldn't create rendering context Windows error " << dwError << "\n");
		return;
	}

	MakeCurrent();
	OpenGLInit();
	DoubleBufferOff();
	SwapBuffersOff();
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
