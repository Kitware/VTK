/*=========================================================================

  Program:   Visualization Toolkit
  Module:    %M%
  Language:  C++
  Date:      %G%
  Version:   %I%


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
 
 
#include "vtkWin32OglrRenderWindow.h"
#include "vtkMFCInteractor.h"
#include "vtkActor.h" 
#include <gl/gl.h>

// states
#define VTKXI_START  0
#define VTKXI_ROTATE 1
#define VTKXI_ZOOM   2
#define VTKXI_PAN    3

// Description:
// Construct object so that light follows camera motion.
vtkMFCInteractor::vtkMFCInteractor()
{
	static timerId = 1;
  
	State = VTKXI_START;
	WindowId = 0;
	TimerId = timerId++;
	dcMem = NULL;
	renWindow = NULL;
	m_bitmap = NULL;
	pOldBitmap = NULL;
	m_hrc = NULL;
	m_hwnd = NULL;
	m_hDC = NULL;
	m_left = 0;
	m_top = 0;
	m_width = 0;
	m_height = 0;
	m_hPalette = NULL;
}

vtkMFCInteractor::~vtkMFCInteractor()
{
	if (m_hPalette) {
		DeleteObject(m_hPalette);
	}

	if (m_hrc != NULL) {
		wglMakeCurrent(NULL, NULL) ;
		wglDeleteContext(m_hrc);
		m_hrc = NULL;
	}
	if (pOldBitmap != NULL) {
		dcMem->SelectObject(pOldBitmap);
		pOldBitmap == NULL;
	}
	if (dcMem != NULL) {				 
		delete dcMem;
		dcMem = NULL;
	}
	if (m_bitmap != NULL) {
		DeleteObject(m_bitmap);
		m_bitmap = NULL;
	}

}

void  vtkMFCInteractor::Start()
{
	// not implementet jet
}

void vtkMFCInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRenderWindowInteractor::PrintSelf(os,indent);
}

// Description:
// Begin processing keyboard strokes.
void vtkMFCInteractor::Initialize()
{
  static int any_initialized = 0;
  vtkWin32OglrRenderWindow *ren;
  //int depth;
  int *size;
  int *position;
  int argc = 0;

  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
    {
    vtkErrorMacro(<<"No renderer defined!");
    return;
    }

  this->Initialized = 1;

  // get the info we need from the RenderingWindow
  ren = (vtkWin32OglrRenderWindow *)(this->RenderWindow);
  ren->Render();
  size    = ren->GetSize();
  position= ren->GetPosition();
  this->WindowId = ren->GetWindowId();

  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

void  vtkMFCInteractor::UpdateSize(int x,int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0])||(y != this->Size[1]))
    {
    this->Size[0] = x;
    this->Size[1] = y;
    this->RenderWindow->SetSize(x,y);
    }

}

void vtkMFCInteractor::MakeDirectRenderer(CDC* pDC, CRect *rcBounds,vtkRenderWindow *renw)
{
	ASSERT(pDC != NULL);
	BOOL bResult;
	int bitsperpixel;

	this->m_top		= rcBounds->top;
	this->m_left	= rcBounds->left;
	this->m_width	= rcBounds->Width();
	this->m_height	= rcBounds->Height();

	if (dcMem != NULL) {				 // delete old stuff
		dcMem->SelectObject(pOldBitmap);
		delete dcMem;
		dcMem = NULL;
		pOldBitmap = NULL;
	}
	
	if (m_bitmap != NULL) {
		DeleteObject(m_bitmap);
		m_bitmap = NULL;
	}

	if(m_hrc != NULL) {
		wglDeleteContext(m_hrc);
		m_hrc = NULL;
	}

	// check out the color depth of the screen-bitmap
	bitsperpixel = pDC->GetDeviceCaps(BITSPIXEL);
	if(bitsperpixel >= 8) {	// only with 8 bit color or more

 		m_hwnd = (pDC->GetWindow())->GetSafeHwnd();
		m_hDC =::GetDC(m_hwnd);

		renWindow = renw;
	
		DescribePixelFormat(m_hDC, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, bitsperpixel);
		m_hrc = wglCreateContext(m_hDC);
		ASSERT(m_hrc != NULL);
		bResult = wglMakeCurrent(m_hDC, m_hrc);
		ASSERT(bResult);
		
		((vtkWin32OglrRenderWindow *) renWindow)->SetContextId(m_hrc);
		renWindow->SetWindowId(m_hwnd);
		((vtkWin32OglrRenderWindow *) renWindow)->SetDeviceContext(m_hDC);
		renWindow->SetSize(m_width,m_height);
		renWindow->DoubleBufferOn();
		renWindow->SwapBuffersOff();		// we do swapbuffers by ourselfs
    glEnable(GL_DEPTH_TEST);
		if(!this->Initialized) this->Initialize();
	}
}
 
void vtkMFCInteractor::MakeIndirectRenderer(int bitmap_width,int bitmap_height,int bitsperpixel,vtkRenderWindow *renw)
{
	void *m_pBits;

	BOOL bResult;
	BITMAPINFOHEADER bitmapheader;
		
	memset(&bitmapheader,0,sizeof(BITMAPINFOHEADER));

	bitmapheader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapheader.biWidth = bitmap_width;
	bitmapheader.biHeight = bitmap_height;
	bitmapheader.biPlanes = 1;			 // always 1 plane
	bitmapheader.biBitCount = bitsperpixel;			 
	bitmapheader.biCompression = BI_RGB; // no compression

	if (dcMem != NULL) {				 // delete old stuff
		dcMem->SelectObject(pOldBitmap);
		delete dcMem;
		dcMem = NULL;
		pOldBitmap == NULL;
	}
	
	if (m_bitmap != NULL) {
		DeleteObject(m_bitmap);
		m_bitmap = NULL;
	}

	if(m_hrc != NULL) {
		wglDeleteContext(m_hrc);
		m_hrc = NULL;
	}

	// create new memory device context
	dcMem = new CDC;
	dcMem->CreateCompatibleDC(NULL);		  // we need no master
	if (dcMem != NULL) 
    {
		// make a bitmap to draw to
		m_bitmap = CreateDIBSection(dcMem->GetSafeHdc(),
				(BITMAPINFO *) &bitmapheader, // ok!!, possible, because there is no palette
			    DIB_PAL_COLORS,
			    &m_pBits,NULL,0);
		pOldBitmap = (HBITMAP) ::SelectObject(dcMem->GetSafeHdc(),m_bitmap);
		DescribePixelFormat(dcMem->GetSafeHdc(),	PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,bitsperpixel);
		m_hrc = wglCreateContext(dcMem->m_hDC);
		bResult = wglMakeCurrent(dcMem->m_hDC, m_hrc);
    glEnable(GL_DEPTH_TEST);
		if (!bResult) return;			  // shouldn't happen
	
		m_hwnd = NULL;				 // there is no coresspoding window
		m_hDC = dcMem->GetSafeHdc(); //

		if (m_bitmap != NULL) 
      {
			if (pOldBitmap != NULL) 
        {	// everything alright ?
				m_top		= 0;
				m_left		= 0;
				m_width		= bitmap_width;
				m_height	= bitmap_height;
		
				renWindow = renw;
				renWindow->DoubleBufferOff();
				renWindow->SwapBuffersOff();
	
				((vtkWin32OglrRenderWindow *) renWindow)->SetContextId(m_hrc);
				renWindow->SetWindowId(m_hwnd);
				((vtkWin32OglrRenderWindow *) renWindow)->SetDeviceContext(dcMem->m_hDC);
				renWindow->SetSize(m_width,m_height);
				Size[0] = m_width;
				Size[1] = m_height;
			  }
		  }
	  }
}

void vtkMFCInteractor::OnMouseMove(CWnd *wnd,UINT nFlags, CPoint point) 
{
	if(this==NULL) return;					//	not existent

	lastPos.x = point.x;
	lastPos.y = point.y;	
}

void vtkMFCInteractor::OnRButtonDown(CWnd *wnd,UINT nFlags, CPoint point) 
{
	if(this==NULL) return;					//	not existent

	m_hwnd = wnd->GetSafeHwnd();
	m_hDC =::GetDC(m_hwnd);
	wglMakeCurrent(m_hDC,m_hrc);

	FindPokedCamera(point.x,Size[1] - point.y);
	if (State == VTKXI_START) {
		State = VTKXI_ZOOM;
		SetTimer(m_hwnd,TimerId,10,NULL);
	}
}

void vtkMFCInteractor::OnRButtonUp(CWnd *wnd,UINT nFlags, CPoint point) 
{
	if(this==NULL) return;					//	not existent

	m_hwnd = wnd->GetSafeHwnd();
	m_hDC =::GetDC(m_hwnd);
	wglMakeCurrent(m_hDC,m_hrc);

	if (State == VTKXI_ZOOM) {
		State = VTKXI_START;
		KillTimer(m_hwnd,TimerId);
	}
}

void vtkMFCInteractor::OnLButtonDown(CWnd *wnd,UINT nFlags, CPoint point) 
{
	if(this==NULL) return;					//	not existent

	m_hwnd = wnd->GetSafeHwnd();
	m_hDC =::GetDC(m_hwnd);
	wglMakeCurrent(m_hDC,m_hrc);

	FindPokedCamera(point.x,Size[1] - point.y);
	if (nFlags & MK_SHIFT) { // Pan
		float *FocalPoint;
		float *Result;

		if (State != VTKXI_START) return;
		State = VTKXI_PAN;

		// calculate the focal depth since we'll be using it a lot
		FocalPoint = CurrentCamera->GetFocalPoint();
      
		CurrentRenderer->SetWorldPoint(FocalPoint[0],FocalPoint[1], FocalPoint[2],1.0);
		CurrentRenderer->WorldToDisplay();
		Result = CurrentRenderer->GetDisplayPoint();
		FocalDepth = Result[2];

		SetTimer(m_hwnd,TimerId,10,NULL);
	} else { //	Rotate
		if (State != VTKXI_START) return;
		State = VTKXI_ROTATE;
		SetTimer(m_hwnd,TimerId,10,NULL);
	}
}

void vtkMFCInteractor::OnLButtonUp(CWnd *wnd,UINT nFlags, CPoint point) 
{
	if(this==NULL) return;					//	not existent

	m_hwnd = wnd->GetSafeHwnd();
	m_hDC =::GetDC(m_hwnd);
	wglMakeCurrent(m_hDC,m_hrc);

	if (State == VTKXI_ROTATE) {
		State = VTKXI_START;
		KillTimer(m_hwnd,TimerId);
	}

	if (State == VTKXI_PAN) {
		State = VTKXI_START;
		KillTimer(m_hwnd,TimerId);
	}

}

void vtkMFCInteractor::OnSize(CWnd *wnd,UINT nType, int cx, int cy) 
{
	if(this==NULL) return;					//	not existent

	m_hwnd = wnd->GetSafeHwnd();
	m_hDC =::GetDC(m_hwnd);
	wglMakeCurrent(m_hDC,m_hrc);
	// if the size changed send iren on to the RenderWindow
	if ((cx != Size[0])||(cy != Size[1]))
	{
		Size[0] = cx;
		Size[1] = cy;
		RenderWindow->SetSize(cx,cy);
		glViewport(0,0,cx,cy);
	}

	Update();
}

void vtkMFCInteractor::OnTimer(CWnd *wnd,UINT nIDEvent) 
{
	if(this==NULL) return;					//	not existent

	float xf,yf;

	m_hwnd = wnd->GetSafeHwnd();
	m_hDC =::GetDC(m_hwnd);
	wglMakeCurrent(m_hDC,m_hrc);

	switch (State) {
		case VTKXI_ROTATE :
   			xf = (lastPos.x - Center[0]) * DeltaAzimuth;
   			yf = ((Size[1] - lastPos.y) - Center[1]) * DeltaElevation;
   			CurrentCamera->Azimuth(xf);
   			CurrentCamera->Elevation(yf);
   		 	CurrentCamera->OrthogonalizeViewUp();
   			if (LightFollowCamera) {
				/* get the first light */
				CurrentLight->SetPosition(CurrentCamera->GetPosition());
				CurrentLight->SetFocalPoint(CurrentCamera->GetFocalPoint());
			}
			Update();
      		break;
    	case VTKXI_PAN :
	  	    {
			    float  FPoint[3];
				float *PPoint;
				float  APoint[3];
				float  RPoint[4];

				// get the current focal point and position
				memcpy(FPoint,CurrentCamera->GetFocalPoint(),sizeof(float)*3);
				PPoint = CurrentCamera->GetPosition();

      			xf = lastPos.x;
      			yf = Size[1] - lastPos.y;
				APoint[0] = xf;
				APoint[1] = yf;
				APoint[2] = FocalDepth;
				CurrentRenderer->SetDisplayPoint(APoint);
				CurrentRenderer->DisplayToWorld();
				memcpy(RPoint,CurrentRenderer->GetWorldPoint(),sizeof(float)*4);
				if (RPoint[3]) {
					RPoint[0] /= RPoint[3];
					RPoint[1] /= RPoint[3];
					RPoint[2] /= RPoint[3];
				}
  	    		/*
   	    		* Compute a translation vector, moving everything 1/10 
   	    		* the distance to the cursor. (Arbitrary scale factor)
   	    		*/		
    	  		CurrentCamera->SetFocalPoint(
				(FPoint[0]-RPoint[0])/10.0 + FPoint[0],
				(FPoint[1]-RPoint[1])/10.0 + FPoint[1],
				(FPoint[2]-RPoint[2])/10.0 + FPoint[2]);
      			CurrentCamera->SetPosition(
				(FPoint[0]-RPoint[0])/10.0 + PPoint[0],
				(FPoint[1]-RPoint[1])/10.0 + PPoint[1],
				(FPoint[2]-RPoint[2])/10.0 + PPoint[2]);
      
      			if (LightFollowCamera) {
					/* get the first light */
					CurrentLight->SetPosition(CurrentCamera->GetPosition());
					CurrentLight->SetFocalPoint(CurrentCamera->GetFocalPoint());
				}
				Update();
            }
      		break;
   		case VTKXI_ZOOM :
		   	{
      			float zoomFactor;
      			float *clippingRange;
      			yf = ((Size[1] - lastPos.y) - Center[1])/(float)Center[1];
      			zoomFactor = pow((double)1.1,(double)yf);
      			clippingRange = CurrentCamera->GetClippingRange();
      			CurrentCamera->SetClippingRange(clippingRange[0]/zoomFactor,			  clippingRange[1]/zoomFactor);
      			CurrentCamera->Zoom(zoomFactor);
				Update();
      		}
   			break;
	}	
}

void vtkMFCInteractor::OnChar(CWnd *wnd,UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(this==NULL) return;					//	not existent

	m_hwnd = wnd->GetSafeHwnd();
	m_hDC =::GetDC(m_hwnd);
	wglMakeCurrent(m_hDC,m_hrc);

    switch (nChar)	{
        // case 'e': exit(1); break;
		case 'u':
		    if (UserMethod) (*UserMethod)(UserMethodArg);
			break;
		case 'r':
		    FindPokedRenderer(lastPos.x,Size[1]-lastPos.y);
			CurrentRenderer->ResetCamera();
			Update();
			break;
	  	case 'w':
	  	    {
	  		    vtkActorCollection *ac;
	  		    vtkActor *anActor;
	  
          	    FindPokedRenderer(lastPos.x,Size[1]-lastPos.y);
	  		    ac = CurrentRenderer->GetActors();
	  		    for (ac->InitTraversal(); anActor = ac->GetNextItem(); ) anActor->GetProperty()->SetWireframe();
				Update();
			}
  		    break;
		case 's':
		    {
	  		    vtkActorCollection *ac;
	  		    vtkActor *anActor;
	  
             	FindPokedRenderer(lastPos.x,Size[1]-lastPos.y);
	  		    ac = CurrentRenderer->GetActors();
	  		    for (ac->InitTraversal(); anActor = ac->GetNextItem(); ) anActor->GetProperty()->SetSurface();
				Update();
       	    }
			break;
	    case '3':
		    {
				if (renWindow->GetStereoRender()) renWindow->StereoRenderOff();
				else renWindow->StereoRenderOn();
				Update();
			}
	  		break;
		case 'p':
		    {
				FindPokedRenderer(lastPos.x,Size[1]-lastPos.y);
				if (StartPickMethod) (*StartPickMethod)(StartPickMethodArg);
	  			Picker->Pick(lastPos.x, Size[1]-lastPos.y,0.0, CurrentRenderer);
				HighlightActor(Picker->GetActor());
				if (EndPickMethod) (*EndPickMethod)(EndPickMethodArg);
			}
			break;
	}
    //	wglMakeCurrent (NULL, NULL);
}

void vtkMFCInteractor::Update()
{
	if(this==NULL) return;					//	not existent
	
	if(renWindow!=NULL) {
		renWindow->Render();
		if(m_hwnd!=NULL) SwapBuffers(m_hDC);	// to memory or double buffered
	}
}

HBITMAP vtkMFCInteractor::GetBitmap()
{
	if(this==NULL) return NULL;					//	not existent

	return m_bitmap;
}

void vtkMFCInteractor::BitBlt(CDC *pDC,int x_position,int y_position)
{
	if(this==NULL) return;					//	not existent

	BOOL bValue = pDC->BitBlt(x_position, y_position, m_width, m_height, dcMem, 0, 0, SRCCOPY);
	ASSERT(bValue);
}

void vtkMFCInteractor::DescribePixelFormat(HDC hDC,DWORD flags,int bitsperpixel)
{
	PIXELFORMATDESCRIPTOR	pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = flags;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = (BYTE) bitsperpixel;
//	pfd.cDepthBits = 32;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int nPixelFormat = ::ChoosePixelFormat(hDC, &pfd);
	ASSERT(nPixelFormat != 0);
	int bResult = ::SetPixelFormat(hDC, nPixelFormat, &pfd);
	ASSERT(bResult);
	int iResult = ::DescribePixelFormat(hDC, nPixelFormat, sizeof(pfd), &pfd);
	ASSERT(iResult != 0);

	if (bitsperpixel == 8) {
		if(!m_hPalette) SetupLogicalPalette();
	}
	DoPalette(hDC);
}

void vtkMFCInteractor::DoPalette(HDC hDC)
{
	if(this==NULL) return;					//	not existent
	if(!hDC) return;
	
	if (m_hPalette) SelectPalette(hDC, m_hPalette, FALSE);
	else SelectPalette(hDC,(HPALETTE) GetStockObject(DEFAULT_PALETTE), FALSE);
	RealizePalette(hDC);
}

void vtkMFCInteractor::SetupLogicalPalette(void)
{
	if(m_hPalette) return;

	struct {
			WORD			ver;
			WORD			entries;
			PALETTEENTRY	colors[256];
	} palstruct = {0x300, 256};

	BYTE reds[] = {0, 36, 72, 109, 145, 182, 218, 255};
	BYTE greens[] = {0, 36, 72, 109, 145, 182, 218, 255};
	BYTE blues[] = {0, 85, 170, 255};

	for (unsigned int i = 0; i < 256; i++) {
		palstruct.colors[i].peRed = reds[i & 0x07];
		palstruct.colors[i].peGreen = greens[(i >> 3) & 0x07];
		palstruct.colors[i].peBlue = blues[(i >> 6) & 0x03];
		palstruct.colors[i].peFlags = 0; 
	}

	m_hPalette = CreatePalette((LOGPALETTE*) &palstruct);
}

void vtkMFCInteractor::GetBitmapInfo(LPBITMAPINFOHEADER lpbi)
{
 	if(this==NULL) return;			//	not existent

	BITMAP              bm;         // bitmap structure

    // fill in BITMAP structure, return NULL if it didn't work

	if(!m_bitmap) return;
    if (!GetObject(m_bitmap, sizeof(bm), (LPSTR)&bm))  return;

    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth = bm.bmWidth;
    lpbi->biHeight = bm.bmHeight;
    lpbi->biPlanes = 1;
    lpbi->biBitCount = bm.bmPlanes * bm.bmBitsPixel;
    lpbi->biCompression = BI_RGB;
    lpbi->biXPelsPerMeter = 0;
    lpbi->biYPelsPerMeter = 0;
    lpbi->biClrUsed = 0;
    lpbi->biClrImportant = 0;
	lpbi->biSizeImage = bm.bmWidth * bm.bmPlanes * bm.bmBitsPixel * bm.bmHeight;
}

HDIB vtkMFCInteractor::GetDIB()
{
	if(this==NULL) return NULL;			//	not existent

    BITMAPINFOHEADER    bi;         // bitmap header
    LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER
    DWORD               dwLen;      // size of memory block
    HANDLE              hDIB;       // handle to DIB, temp handle
	int palsize = 0;

	lpbi = &bi;

	GetBitmapInfo(lpbi);

	// if we need a palette
	if(lpbi->biBitCount<15) palsize = (1L<<lpbi->biBitCount)*sizeof(RGBTRIPLE);
 
    dwLen = bi.biSize + palsize + bi.biSizeImage;

    hDIB = ::GlobalAlloc(GHND, dwLen);
	if (hDIB) {
	    lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(hDIB);
        lpbi->biSize = sizeof(BITMAPINFOHEADER);
		::GetDIBits(dcMem->GetSafeHdc(), GetBitmap(), 0, (UINT)bi.biHeight, 
				NULL,	(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
		::GetDIBits(dcMem->GetSafeHdc(), GetBitmap(), 0, (UINT)bi.biHeight, 
				(LPSTR)lpbi + (WORD)lpbi->biSize + palsize,
				(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
		::GlobalUnlock(hDIB);
	}
	return hDIB;
}

void vtkMFCInteractor::StretchDIB(CDC *pDC,int x_position,int y_position, int x_width,int y_width)
{
	if(this==NULL) return;			//	not existent

  BITMAPINFOHEADER   bi;         // bitmap header
  BITMAPINFOHEADER  *lpbi;       // pointer to BITMAPINFOHEADER
  DWORD              dwLen;      // size of memory block
  HANDLE             hDIB;
	int palsize = 0;

	lpbi = &bi;
 
	GetBitmapInfo(lpbi);

	// if we need a palette
	if(lpbi->biBitCount<15) palsize = (1L<<lpbi->biBitCount)*sizeof(RGBTRIPLE);
    
	dwLen = bi.biSize + palsize + bi.biSizeImage;
  hDIB = ::GlobalAlloc(GHND, dwLen);
  if (hDIB)
    {
	  lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(hDIB);
    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    ::GetDIBits(pDC->GetSafeHdc(), GetBitmap(), 0, (UINT)bi.biHeight, 
				NULL,	(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
	  ::GetDIBits(pDC->GetSafeHdc(), GetBitmap(), 0, (UINT)bi.biHeight, 
				(LPSTR)lpbi + (WORD)lpbi->biSize + palsize,
				(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
    ::StretchDIBits(pDC->GetSafeHdc(),x_position, y_position, x_width, y_width, 0, 0, 
				        lpbi->biWidth, lpbi->biHeight,
				        ((BYTE *)lpbi) + lpbi->biSize + palsize,	
				        (LPBITMAPINFO)lpbi,DIB_RGB_COLORS,SRCCOPY);
    ::GlobalUnlock(hDIB);
    ::GlobalFree(hDIB);
    }
}

