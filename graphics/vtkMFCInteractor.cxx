/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMFCInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Horst Schreiber for developing this MFC code
			 with additions from Nick Edgington

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
 
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxole.h>         // MFC OLE classes
 
#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkMFCInteractor.h"
#include "vtkActor.h" 
#include <gl/gl.h>

// states
#define VTKXI_START  0
#define VTKXI_ROTATE 1
#define VTKXI_ZOOM   2
#define VTKXI_PAN    3
#define VTKXI_LOOP   4

#define MAXWRITE 65536

#define TIMEROFFSETT 0x100
static BOOL bAuto = FALSE;
//static HANDLE Mutex = CreateMutex(NULL,FALSE,NULL);

// Description:
// Construct object so that light follows camera motion.
vtkMFCInteractor::vtkMFCInteractor()
{
  static int timerId = 1;
  
  this->State = VTKXI_START;
  this->WindowId = 0;
  
  this->TimerId = timerId++;
  this->OldBitmap = NULL;
  this->RenderWindow = NULL;
  this->WindowBitmap = NULL;
  this->OldBitmap = NULL;
  this->WindowRC = NULL;
  this->WindowHandle = NULL;
  this->WindowDC = NULL;
  this->WindowLeft = 0;
  this->WindowTop = 0;
  this->WindowWidth = 0;
  this->WindowHeight = 0;
  this->WindowPalette = NULL;
  this->MemoryDC = NULL;
  this->MiliSeconds = 10;
  this->Mutex = CreateMutex(NULL,FALSE,NULL);
}

vtkMFCInteractor::~vtkMFCInteractor()
{
  if (this->WindowPalette) 
    {
    DeleteObject(this->WindowPalette);
    }

  if (this->WindowRC != NULL) 
    {
    wglMakeCurrent(NULL, NULL) ;
    wglDeleteContext(this->WindowRC);
    this->WindowRC = NULL;
    }
  if (this->OldBitmap != NULL) 
    {
    this->MemoryDC->SelectObject(this->OldBitmap);
    this->OldBitmap == NULL;
    }
  if (this->MemoryDC != NULL) 
    {
    delete this->MemoryDC;
    this->MemoryDC = NULL;
    }
  if (this->WindowBitmap != NULL) 
    {
    DeleteObject(this->WindowBitmap);
    this->WindowBitmap = NULL;
    }
}

void  vtkMFCInteractor::Start()
{
  // not necessary
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
  vtkWin32OpenGLRenderWindow *ren;
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

  if (this->Initialized) return;
  this->Initialized = 1;

  // get the info we need from the RenderingWindow
  ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
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

void vtkMFCInteractor::Initialize(HWND hwnd, CRect *rcBounds,vtkRenderWindow *renw)
{
  MakeDirectRenderer(hwnd, rcBounds,renw);
}

void vtkMFCInteractor::MakeDirectRenderer(HWND hwnd, CRect *rcBounds,vtkRenderWindow *renw)
{
  BOOL bResult;
  int bitsperpixel;

  WindowRectangle = *rcBounds;

  this->WindowTop       = rcBounds->top;
  this->WindowLeft	= rcBounds->left;
  this->WindowWidth	= rcBounds->Width();
  this->WindowHeight	= rcBounds->Height();

  if (this->MemoryDC != NULL) 
    { // delete old stuff
    this->MemoryDC->SelectObject(this->OldBitmap);
    delete this->MemoryDC;
    this->MemoryDC = NULL;
    this->OldBitmap = NULL;
    }
  
  if (this->WindowBitmap != NULL) 
    {
    DeleteObject(this->WindowBitmap);
    this->WindowBitmap = NULL;
    }
  
  if(this->WindowRC != NULL) 
    {
    wglDeleteContext(this->WindowRC);
    this->WindowRC = NULL;
    }

  // check out the color depth of the screen-bitmap
  bitsperpixel = ::GetDeviceCaps(::GetDC(hwnd),BITSPIXEL);
  if(bitsperpixel >= 8) 
    {	// only with 8 bit color or more
    this->WindowHandle = hwnd; 
    this->WindowDC =::GetDC(this->WindowHandle);
    
    this->RenderWindow = renw;
    
    DescribePixelFormat(this->WindowDC, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, bitsperpixel);
    this->WindowRC = wglCreateContext(this->WindowDC);
    ASSERT(this->WindowRC != NULL);
    bResult = wglMakeCurrent(this->WindowDC, this->WindowRC);
    ASSERT(bResult);
    
    ((vtkWin32OpenGLRenderWindow *) this->RenderWindow)->SetContextId(this->WindowRC);
    this->RenderWindow->SetWindowId(this->WindowHandle);
    ((vtkWin32OpenGLRenderWindow *) this->RenderWindow)->SetDeviceContext(this->WindowDC);
    this->RenderWindow->SetSize(this->WindowWidth,this->WindowHeight);
    this->RenderWindow->DoubleBufferOn();
    this->RenderWindow->SwapBuffersOff();		// we do swapbuffers by ourselfs
    if(!this->Initialized) this->Initialize();
    }
  ((vtkWin32OpenGLRenderWindow *) this->RenderWindow)->WindowInitialize();
  
  wglMakeCurrent(NULL,NULL);
}
 
void vtkMFCInteractor::MakeIndirectRenderer(int bitmap_width,int bitmap_height,int bitsperpixel,vtkRenderWindow *renw)
{
  void *m_pBits;

  BITMAPINFOHEADER bitmapheader;
		
  memset(&bitmapheader,0,sizeof(BITMAPINFOHEADER));

  bitmapheader.biSize = sizeof(BITMAPINFOHEADER);
  bitmapheader.biWidth = bitmap_width;
  bitmapheader.biHeight = bitmap_height;
  bitmapheader.biPlanes = 1;			 // always 1 plane
  bitmapheader.biBitCount = bitsperpixel;			 
  bitmapheader.biCompression = BI_RGB; // no compression

  if (this->MemoryDC != NULL) 
    {// delete old stuff
    this->MemoryDC->SelectObject(this->OldBitmap);
    delete this->MemoryDC;
    this->MemoryDC = NULL;
    this->OldBitmap == NULL;
    }
	
  if (this->WindowBitmap != NULL) 
    {
    DeleteObject(this->WindowBitmap);
    this->WindowBitmap = NULL;
    }
  
  if(this->WindowRC != NULL) 
    {
    wglDeleteContext(this->WindowRC);
    this->WindowRC = NULL;
    }
  
  // create new memory device context
  this->MemoryDC = new CDC;
  this->MemoryDC->CreateCompatibleDC(NULL);		  // we need no master
  if (this->MemoryDC != NULL) 
    {
    // make a bitmap to draw to
    this->WindowBitmap = CreateDIBSection(this->MemoryDC->GetSafeHdc(),
					  (BITMAPINFO *) &bitmapheader, // ok!!, possible, because there is no palette
					  DIB_PAL_COLORS,
					  &m_pBits,NULL,0);
    this->OldBitmap = (HBITMAP) ::SelectObject(this->MemoryDC->GetSafeHdc(),this->WindowBitmap);
    DescribePixelFormat(this->MemoryDC->GetSafeHdc(),	PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,bitsperpixel);
    this->WindowRC = wglCreateContext(this->MemoryDC->m_hDC);
    VERIFY(wglMakeCurrent(this->MemoryDC->m_hDC, this->WindowRC));
    
    this->WindowHandle = NULL;				 // there is no coresspoding window
    this->WindowDC = this->MemoryDC->GetSafeHdc(); //
    
    if (this->WindowBitmap != NULL) 
      {
      if (this->OldBitmap != NULL) 
	{	// everything alright ?
	this->WindowTop		= 0;
	this->WindowLeft        = 0;
	this->WindowWidth       = bitmap_width;
	this->WindowHeight	= bitmap_height;
	
	this->RenderWindow = renw;
	this->RenderWindow->DoubleBufferOff();
	this->RenderWindow->SwapBuffersOff();
	
	((vtkWin32OpenGLRenderWindow *) this->RenderWindow)->SetContextId(this->WindowRC);
	this->RenderWindow->SetWindowId(this->WindowHandle);
	((vtkWin32OpenGLRenderWindow *) this->RenderWindow)->SetDeviceContext(this->MemoryDC->m_hDC);
	this->RenderWindow->SetSize(this->WindowWidth,this->WindowHeight);
	Size[0] = this->WindowWidth;
	Size[1] = this->WindowHeight;
	}
      }
    }
  ((vtkWin32OpenGLRenderWindow *) this->RenderWindow)->WindowInitialize();
  
  VERIFY(wglMakeCurrent(NULL,NULL));
}

void vtkMFCInteractor::OnMouseMove(CWnd *wnd,UINT nFlags, CPoint point) 
{
  ASSERT(wnd);		
	
  WaitForSingleObject(Mutex , INFINITE);

  this->LastPosition.x = point.x;
  this->LastPosition.y = point.y;	

  ReleaseMutex(Mutex);
}

void vtkMFCInteractor::OnRButtonDown(CWnd *wnd,UINT nFlags, CPoint point) 
{
  ASSERT(wnd);

  WaitForSingleObject(Mutex , INFINITE);

  wnd->SetCapture( ); 

  VERIFY(wglMakeCurrent(this->WindowDC,this->WindowRC));

  FindPokedCamera(point.x,Size[1] - point.y);
  if (State == VTKXI_START) 
    {
    State = VTKXI_ZOOM;
    this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
    if (! bAuto) SetTimer(this->WindowHandle,TimerId,10,NULL);
    }
  
  VERIFY(wglMakeCurrent(this->WindowDC,NULL));
  ReleaseMutex(Mutex);
}

void vtkMFCInteractor::OnRButtonUp(CWnd *wnd,UINT nFlags, CPoint point) 
{
  VERIFY(wnd);

  VERIFY(wglMakeCurrent(this->WindowDC,this->WindowRC));

  if (State == VTKXI_ZOOM) 
    {
    State = VTKXI_START;
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    if (! bAuto) KillTimer(this->WindowHandle,TimerId);
    }
  
  ReleaseCapture( );
  VERIFY(wglMakeCurrent(this->WindowDC,NULL));
  ReleaseMutex(Mutex);
}

void vtkMFCInteractor::OnLButtonDown(CWnd *wnd,UINT nFlags, CPoint point) 
{
  VERIFY(wnd);

  WaitForSingleObject(Mutex , INFINITE);

  wnd->SetCapture( ); // nje

  VERIFY(wglMakeCurrent(this->WindowDC,this->WindowRC));

  FindPokedCamera(point.x,Size[1] - point.y);
  if (nFlags & MK_SHIFT) 
    { // Pan
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
    this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
    if (! bAuto) SetTimer(this->WindowHandle,TimerId,10,NULL);
    } 
  else 
    { //	Rotate
    if (State != VTKXI_START) return;
    State = VTKXI_ROTATE;
    this->RenderWindow->SetDesiredUpdateRate(this->DesiredUpdateRate);
    if (! bAuto) SetTimer(this->WindowHandle,TimerId,10,NULL);
    }
  
  VERIFY(wglMakeCurrent(this->WindowDC,NULL));
  ReleaseMutex(Mutex);
}

void vtkMFCInteractor::OnLButtonUp(CWnd *wnd,UINT nFlags, CPoint point) 
{
  WaitForSingleObject(Mutex , INFINITE);
  
  wglMakeCurrent(this->WindowDC,this->WindowRC);

  if (State == VTKXI_ROTATE) 
    {
    State = VTKXI_START;
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    if (! bAuto) KillTimer(this->WindowHandle,TimerId);
    }
  
  if (State == VTKXI_PAN) 
    {
    State = VTKXI_START;
    this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRate);
    if (! bAuto) KillTimer(this->WindowHandle,TimerId);
    }
  
  ReleaseCapture( ); 
  VERIFY(wglMakeCurrent(this->WindowDC,NULL));
  ReleaseMutex(Mutex);
}

void vtkMFCInteractor::OnSize(CWnd *wnd,UINT nType, int cx, int cy) 
{
  VERIFY(wnd);

  if(RenderWindow) {
  WaitForSingleObject(Mutex , INFINITE);

  VERIFY(wglMakeCurrent(this->WindowDC,this->WindowRC));

  // if the size changed send iren on to the RenderWindow
  if ((cx != Size[0])||(cy != Size[1]))
    {
    Size[0] = cx;
    Size[1] = cy;
    RenderWindow->SetSize(cx,cy);
    glViewport(0,0,cx,cy);
    }

  Update();
  ReleaseMutex(Mutex);

  VERIFY(wglMakeCurrent(NULL,NULL));
  }
}

void vtkMFCInteractor::OnTimer(CWnd *wnd,UINT nIDEvent) 
{
  float xf,yf;

  if ( bAuto) KillTimer(WindowId,TimerId+TIMEROFFSETT);

  WaitForSingleObject(Mutex , INFINITE);

  VERIFY(wglMakeCurrent(this->WindowDC,this->WindowRC));

  switch (State) 
    {
    case VTKXI_ROTATE :
      xf = (this->LastPosition.x - Center[0]) * DeltaAzimuth;
      yf = ((Size[1] - this->LastPosition.y) - Center[1]) * DeltaElevation;
      CurrentCamera->Azimuth(xf);
      CurrentCamera->Elevation(yf);
      CurrentCamera->OrthogonalizeViewUp();
      if (LightFollowCamera) 
	{
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
      
      xf = this->LastPosition.x;
      yf = Size[1] - this->LastPosition.y;
      APoint[0] = xf;
      APoint[1] = yf;
      APoint[2] = FocalDepth;
      CurrentRenderer->SetDisplayPoint(APoint);
      CurrentRenderer->DisplayToWorld();
      memcpy(RPoint,CurrentRenderer->GetWorldPoint(),sizeof(float)*4);
      if (RPoint[3]) 
	{
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
      
      if (LightFollowCamera) 
	{
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
      yf = ((Size[1] - this->LastPosition.y) - Center[1])/(float)Center[1];
      zoomFactor = pow((double)1.1,(double)yf);
      if (this->CurrentCamera->GetParallelProjection())
	{
	this->CurrentCamera->
	  SetParallelScale(this->CurrentCamera->GetParallelScale()/zoomFactor);
	}
      else
	{
	clippingRange = this->CurrentCamera->GetClippingRange();
	this->CurrentCamera->SetClippingRange(clippingRange[0]/zoomFactor,
					    clippingRange[1]/zoomFactor);
	this->CurrentCamera->Dolly(zoomFactor);
	}
      Update();
      }
      break;
    }	
  
#ifdef TIMER
  if(bAuto) SetTimer(WindowId,TimerId+TIMEROFFSETT,MiliSeconds,NULL);
#endif
  VERIFY(wglMakeCurrent(this->WindowDC,NULL));
  ReleaseMutex(Mutex);
}

void vtkMFCInteractor::OnChar(CWnd *wnd,UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  WaitForSingleObject(Mutex , INFINITE);

  VERIFY(wglMakeCurrent(this->WindowDC,this->WindowRC));

  switch (nChar)	
    {
    case 'l': 
      if (bAuto)
	KillTimer(WindowId,TimerId+TIMEROFFSETT);
      else
	SetTimer(WindowId,TimerId+TIMEROFFSETT,MiliSeconds,NULL);
      bAuto= !bAuto;
      break;
      
      // case 'e': exit(1); break;
    case 'u':
      if (UserMethod) (*UserMethod)(UserMethodArg);
      break;
    case 'r':
      FindPokedRenderer(this->LastPosition.x,Size[1]-this->LastPosition.y);
      CurrentRenderer->ResetCamera();
      Update();
      break;
    case 'w':
      {
      vtkActorCollection *ac;
      vtkActor *anActor, *aPart;
      
      FindPokedRenderer(this->LastPosition.x,Size[1]-this->LastPosition.y);
      ac = CurrentRenderer->GetActors();
      for (ac->InitTraversal(); (anActor = ac->GetNextItem()); )
	{
	for (anActor->InitPartTraversal();(aPart=anActor->GetNextPart()); )
	  {
	  aPart->GetProperty()->SetRepresentationToWireframe();
	  }
	}
      Update();
      }
      break;
    case 's':
      {
      vtkActorCollection *ac;
      vtkActor *anActor, *aPart;
      
      FindPokedRenderer(this->LastPosition.x,Size[1]-this->LastPosition.y);
      ac = CurrentRenderer->GetActors();
      for (ac->InitTraversal(); (anActor = ac->GetNextItem()); )
	{
	for (anActor->InitPartTraversal();(aPart=anActor->GetNextPart()); )
	  {
	  aPart->GetProperty()->SetRepresentationToSurface();
	  }
	}
      Update();
      }
      break;
    case '3':
      {
      if (this->RenderWindow->GetStereoRender()) this->RenderWindow->StereoRenderOff();
      else this->RenderWindow->StereoRenderOn();
      Update();
      }
      break;
    case 'p':
      {
      FindPokedRenderer(this->LastPosition.x,Size[1]-this->LastPosition.y);
      if (StartPickMethod) (*StartPickMethod)(StartPickMethodArg);
      Picker->Pick(this->LastPosition.x, Size[1]-this->LastPosition.y,0.0, CurrentRenderer);
      HighlightActor(Picker->GetActor());
      if (EndPickMethod) (*EndPickMethod)(EndPickMethodArg);
      }
      break;
    }
  VERIFY(wglMakeCurrent(this->WindowDC,NULL));
  ReleaseMutex(Mutex);
}

void vtkMFCInteractor::Update()
{
  if(this->RenderWindow!=NULL) 
    {
    this->RenderWindow->Render();
    if(this->WindowHandle!=NULL) SwapBuffers(this->WindowDC);	
    // to memory or double buffered
    }
}

HBITMAP vtkMFCInteractor::GetBitmap()
{
  return this->WindowBitmap;
}

void vtkMFCInteractor::BitBlt(CDC *pDC,int x_position,int y_position)
{
  BOOL bValue = pDC->BitBlt(x_position, y_position, this->WindowWidth, 
			    this->WindowHeight, this->MemoryDC, 0, 0, SRCCOPY);
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
  pfd.cDepthBits = 16;
  pfd.iLayerType = PFD_MAIN_PLANE;

  int nPixelFormat = ::ChoosePixelFormat(hDC, &pfd);
  ASSERT(nPixelFormat != 0);
  int bResult = ::SetPixelFormat(hDC, nPixelFormat, &pfd);
  ASSERT(bResult);
  int iResult = ::DescribePixelFormat(hDC, nPixelFormat, sizeof(pfd), &pfd);
  ASSERT(iResult != 0);

  if (bitsperpixel == 8) {
  if(!this->WindowPalette) SetupLogicalPalette();
  }
  DoPalette(hDC);
}

void vtkMFCInteractor::DoPalette(HDC hDC)
{
  VERIFY(hDC);

  if (this->WindowPalette) SelectPalette(hDC, this->WindowPalette, FALSE);
  else SelectPalette(hDC,(HPALETTE) GetStockObject(DEFAULT_PALETTE), FALSE);
  RealizePalette(hDC);
}

void vtkMFCInteractor::SetupLogicalPalette(void)
{
  if(this->WindowPalette) return;

  struct {
    WORD          ver;
    WORD          entries;
    PALETTEENTRY  colors[256];
  } palstruct = {0x300, 256};

  BYTE reds[] = {0, 36, 72, 109, 145, 182, 218, 255};
  BYTE greens[] = {0, 36, 72, 109, 145, 182, 218, 255};
  BYTE blues[] = {0, 85, 170, 255};

  for (unsigned int i = 0; i < 256; i++) 
    {
    palstruct.colors[i].peRed = reds[i & 0x07];
    palstruct.colors[i].peGreen = greens[(i >> 3) & 0x07];
    palstruct.colors[i].peBlue = blues[(i >> 6) & 0x03];
    palstruct.colors[i].peFlags = 0; 
    }

  this->WindowPalette = CreatePalette((LOGPALETTE*) &palstruct);
}

void vtkMFCInteractor::GetBitmapInfo(LPBITMAPINFOHEADER lpbi)
{
  BITMAP              bm;         // bitmap structure

  // fill in BITMAP structure, return NULL if it didn't work

  if(!this->WindowBitmap) return;
  if (!GetObject(this->WindowBitmap, sizeof(bm), (LPSTR)&bm))  return;

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

HDIB vtkMFCInteractor::GetDIB(int width,int height,int bitsperpixel)
{
  BITMAPINFOHEADER    bi;         // bitmap header
  LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER
  DWORD               dwLen;      // size of memory block
  HANDLE              hDIB = NULL;       // handle to DIB, temp handle
  int palsize = 0;
  HWND hwnd = this->WindowHandle;

  MakeIndirectRenderer(width,height,bitsperpixel,this->RenderWindow);
  if(GetBitmap()!=NULL) 
    {
    FindPokedRenderer(this->LastPosition.x,Size[1]-this->LastPosition.y);
    this->Update();
    
    lpbi = &bi;
    
    GetBitmapInfo(lpbi);
    
    // if we need a palette
    if(lpbi->biBitCount<15) palsize = (2L<<lpbi->biBitCount)*sizeof(RGBTRIPLE);
    
    dwLen = bi.biSize + palsize + bi.biSizeImage;
    
    hDIB = ::GlobalAlloc(GHND, dwLen);
    if (hDIB) 
      {
      lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(hDIB);
      lpbi->biSize = sizeof(BITMAPINFOHEADER);
      ::GetDIBits(this->MemoryDC->GetSafeHdc(), GetBitmap(), 0, (UINT)bi.biHeight, 
		  NULL,	(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
      ::GetDIBits(this->MemoryDC->GetSafeHdc(), GetBitmap(), 0, (UINT)bi.biHeight, 
		  (LPSTR)lpbi + (WORD)lpbi->biSize + palsize,
		  (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
      ::GlobalUnlock(hDIB);
      }
    
    }
  MakeDirectRenderer(hwnd,&WindowRectangle,this->RenderWindow);
  
  FindPokedRenderer(this->LastPosition.x,Size[1]-this->LastPosition.y);
  this->Update(); 

  return hDIB;
}

BOOL vtkMFCInteractor::StretchDIB(CDC *pDC,int x_position,int y_position, int x_width,int y_width,
				  int width,int height,int bitsperpixel)
{
  LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER
  HANDLE              hDIB;       // handle to DIB, temp handle
  HWND hwnd = this->WindowHandle;
  int palsize = 0;

  hDIB = GetDIB(width,height,bitsperpixel);
  if (!hDIB) return FALSE;

  lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(hDIB);
  if(lpbi->biBitCount<15) palsize = (2L<<lpbi->biBitCount)*sizeof(RGBTRIPLE);
  ::StretchDIBits(pDC->GetSafeHdc(),x_position, y_position, x_width, y_width, 0, 0, 
		  lpbi->biWidth, lpbi->biHeight,
		  (LPSTR)lpbi + (WORD)lpbi->biSize + palsize ,	
		  (LPBITMAPINFO)lpbi,DIB_RGB_COLORS,SRCCOPY);
  ::GlobalUnlock(hDIB);
  ::GlobalFree(hDIB);
  return TRUE;
}

BOOL vtkMFCInteractor::SaveBMP(LPCTSTR lpszPathName,int width,int height,int bitsperpixel)
{
  BOOL bValue = FALSE;
  HWND hwnd = this->WindowHandle;

  // make bitmap to render to

  MakeIndirectRenderer(width,height,bitsperpixel,this->RenderWindow);
	  
  if(GetBitmap()!=NULL) 
    {
    FindPokedRenderer(this->LastPosition.x,Size[1]-this->LastPosition.y);
    this->Update();
    
    BITMAPINFO *pbm;
    
    pbm = CreateBitmapInfoStruct(NULL,GetBitmap());
    if(pbm!=NULL) 
      {
      CreateBMPFile(NULL,(LPSTR) lpszPathName,pbm,GetBitmap(),::GetDC(NULL));
      LocalFree(pbm);
      bValue = TRUE;
      }
    }
		
  MakeDirectRenderer(hwnd,&WindowRectangle,this->RenderWindow);
  FindPokedRenderer(this->LastPosition.x,Size[1]-this->LastPosition.y);
  Update(); 
	
  return bValue;
}

// the following cope is taken from microsoft examples shipped with vc++ 4.0

PBITMAPINFO vtkMFCInteractor::CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp) 
{ 
  BITMAP bmp; 
  PBITMAPINFO pbmi; 
  WORD    cClrBits; 
 
  /* Retrieve the bitmap's color format, width, and height. */ 
 
  if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) return NULL;
 
  /* Convert the color format to a count of bits. */ 
 
  cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
 
  if (cClrBits == 1) cClrBits = 1; 
  else if (cClrBits <= 4) cClrBits = 4; 
  else if (cClrBits <= 8) cClrBits = 8; 
  else if (cClrBits <= 16) cClrBits = 16; 
  else if (cClrBits <= 24) cClrBits = 24; 
  else cClrBits = 32; 
 
  /* 
   * Allocate memory for the BITMAPINFO structure. (This structure 
   * contains a BITMAPINFOHEADER structure and an array of RGBQUAD data 
   * structures.) 
   */ 
 
  if (cClrBits != 24) 
    pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
				    sizeof(BITMAPINFOHEADER) + 
				    sizeof(RGBQUAD) * (2^cClrBits)); 
 
  /* 
   * There is no RGBQUAD array for the 24-bit-per-pixel format. 
   */ 
 
  else 
    pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
				    sizeof(BITMAPINFOHEADER)); 
 
 
 
  /* Initialize the fields in the BITMAPINFO structure. */ 
 
  pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
  pbmi->bmiHeader.biWidth = bmp.bmWidth; 
  pbmi->bmiHeader.biHeight = bmp.bmHeight; 
  pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
  pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
  if (cClrBits < 24) 
    pbmi->bmiHeader.biClrUsed = 2^cClrBits; 
 
 
  /* If the bitmap is not compressed, set the BI_RGB flag. */ 
 
  pbmi->bmiHeader.biCompression = BI_RGB; 
 
  /* 
   * Compute the number of bytes in the array of color 
   * indices and store the result in biSizeImage. 
   */ 
 
  pbmi->bmiHeader.biSizeImage = (pbmi->bmiHeader.biWidth + 7) /8 
    * pbmi->bmiHeader.biHeight 
    * cClrBits; 
 
  /* 
   * Set biClrImportant to 0, indicating that all of the 
   * device colors are important. 
   */ 
 
  pbmi->bmiHeader.biClrImportant = 0; 
 
  return pbmi; 
 
} 
 
void vtkMFCInteractor::CreateBMPFile(HWND hwnd, LPTSTR pszFile, 
				     PBITMAPINFO pbi, 
				     HBITMAP hBMP, HDC hDC) 
{ 
  HANDLE hf;                  /* file handle */ 
  BITMAPFILEHEADER hdr;       /* bitmap file-header */ 
  PBITMAPINFOHEADER pbih;     /* bitmap info-header */ 
  LPBYTE lpBits;              /* memory pointer */ 
  DWORD dwTotal;              /* total count of bytes */ 
  DWORD cb;                   /* incremental count of bytes */ 
  BYTE *hp;                   /* byte pointer */ 
  DWORD dwTmp; 
  
  
  pbih = (PBITMAPINFOHEADER) pbi; 
  lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
  if (!lpBits) return;
  
  /* 
   * Retrieve the color table (RGBQUAD array) and the bits 
   * (array of palette indices) from the DIB. 
   */ 
  
  if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, 
		 lpBits, pbi, DIB_RGB_COLORS)) return;
  
  /* Create the .BMP file. */ 
  
  hf = CreateFile(pszFile, 
		  GENERIC_READ | GENERIC_WRITE, 
		  (DWORD) 0, 
		  (LPSECURITY_ATTRIBUTES) NULL, 
		  CREATE_ALWAYS, 
		  FILE_ATTRIBUTE_NORMAL, 
		  (HANDLE) NULL); 
  
  if (hf == INVALID_HANDLE_VALUE) return;
  
  hdr.bfType = 0x4d42;        /* 0x42 = "B" 0x4d = "M" */ 
  
  /* Compute the size of the entire file. */ 
  
  hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
			pbih->biSize + pbih->biClrUsed 
			* sizeof(RGBQUAD) + pbih->biSizeImage); 
  
  hdr.bfReserved1 = 0; 
  hdr.bfReserved2 = 0; 
  
  /* Compute the offset to the array of color indices. */ 
  
  hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
    pbih->biSize + pbih->biClrUsed 
    * sizeof (RGBQUAD); 
  
  /* Copy the BITMAPFILEHEADER into the .BMP file. */ 
  
  if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
		 (LPDWORD) &dwTmp, (LPOVERLAPPED) NULL)) return;
  
  /* Copy the BITMAPINFOHEADER and RGBQUAD array into the file. */ 
  
  if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
		 + pbih->biClrUsed * sizeof (RGBQUAD), 
		 (LPDWORD) &dwTmp, (LPOVERLAPPED) NULL)) return;
  
  /* Copy the array of color indices into the .BMP file. */ 
  
  dwTotal = cb = pbih->biSizeImage; 
  hp = lpBits; 
  while (cb > MAXWRITE)  
    {
    if (!WriteFile(hf, (LPSTR) hp, (int) MAXWRITE, 
		   (LPDWORD) &dwTmp, (LPOVERLAPPED) NULL)) return;
    cb-= MAXWRITE; 
    hp += MAXWRITE; 
    } 
  
  WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp, (LPOVERLAPPED) NULL);
  CloseHandle(hf);
  
  GlobalFree((HGLOBAL)lpBits);
}
#ifdef TIMER
void vtkMFCInteractor::StartTiming(int count)
{
  if(bAuto==FALSE) 
    {
    MiliSeconds=count;
    bAuto = TRUE;
    SetTimer(this->WindowId,this->TimerId+TIMEROFFSETT,MiliSeconds,NULL);
    }
}

void vtkMFCInteractor::StopTiming()
{
  if(bAuto==TRUE) 
    {
    bAuto = FALSE;
    KillTimer(WindowId,TimerId+TIMEROFFSETT);
    }	
}

void vtkMFCInteractor::OnEnterIdle()
{
  if (State != VTKXI_START) return;
  if ( bAuto) this->RenderWindow->Render();
}
#endif
