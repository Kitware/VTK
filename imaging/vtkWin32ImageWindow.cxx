
#include "vtkWin32ImageWindow.h"

unsigned char *vtkWin32ImageWindow::GetPixelData(int x1, int y1, int x2, int y2, int)
{

  int width  = (abs(x2 - x1)+1);
  int height = (abs(y2 - y1)+1);
  HDC compatHdc;
  HBITMAP bitmap = (HBITMAP) 0;
  BITMAPINFO dataHeader;
  int dataWidth = ((this->Size[0]*3+3)/4)*4;

  // Define the bitmap header
  dataHeader.bmiHeader.biSize = 40;
  dataHeader.bmiHeader.biWidth = this->Size[0];
  dataHeader.bmiHeader.biHeight = this->Size[1];
  dataHeader.bmiHeader.biPlanes = 1;
  dataHeader.bmiHeader.biBitCount = 24;
  dataHeader.bmiHeader.biCompression = BI_RGB;
  dataHeader.bmiHeader.biSizeImage = dataWidth*this->Size[1];
  dataHeader.bmiHeader.biClrUsed = 0;
  dataHeader.bmiHeader.biClrImportant = 0;
  
  // Create the bitmap
  bitmap = CreateDIBSection(this->DeviceContext, &dataHeader,
                         DIB_RGB_COLORS, (void **)(&(this->DIBPtr)), 
                         NULL, 0);

  // Create a compatible device context
  compatHdc = (HDC) CreateCompatibleDC(this->DeviceContext);

  // Put the bitmap into the device context
  SelectObject(compatHdc, bitmap);

  int y_low = 0;
  int y_hi = 0;
  int x_low = 0;
  int x_hi = 0;

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

  // Get the bitmap
  BitBlt(compatHdc, 0, 0, width, height, this->DeviceContext, x_low, y_low, SRCCOPY);

  // Allocate space for the data
  int size = width*height*3;
  unsigned char*  data = new unsigned char[size];
  if (!data) 
    {
    vtkErrorMacro(<< "Failed to malloc space for pixel data!");
    return (unsigned char*) NULL;
    }

  GetDIBits(compatHdc, bitmap, 0, height, data, &dataHeader, DIB_RGB_COLORS);
 
  // Data is in BGR format.  Change to RGB. 
  unsigned char*  p_data =  data;
  unsigned char*  p_data2 = data;
  unsigned char rbyte;
  unsigned char gbyte;
  unsigned char bbyte;

  for (int i = 0; i < width*height; i++)
    {
	bbyte = *p_data++;
	gbyte = *p_data++;
	rbyte = *p_data++;
	*p_data2++ = rbyte;
	*p_data2++ = gbyte;
	*p_data2++ = bbyte;
	}

  // Free the device context
  DeleteDC(compatHdc);
  
  return data;

}


//---------------------------------------------------------------------------
unsigned char *vtkWin32ImageWindow::GetDIBPtr()
{
	return this->DIBPtr;
}

//---------------------------------------------------------------------------
void vtkWin32ImageWindow::SwapBuffers()
{
  static int swapFlag = 0;
  static HDC compatHdc;
  static HDC oldHdc;
  static HBITMAP backBuffer= (HBITMAP) 0;
  static BITMAPINFO dataHeader;
  int dataWidth = ((this->Size[0]*3+3)/4)*4;

  if (swapFlag == 0)
    {
	// Create buffer/check buffer size
	vtkDebugMacro (<< "Creating buffer");
	if (!backBuffer)
	  {

      dataHeader.bmiHeader.biSize = 40;
      dataHeader.bmiHeader.biWidth = this->Size[0];
      dataHeader.bmiHeader.biHeight = this->Size[1];
      dataHeader.bmiHeader.biPlanes = 1;
      dataHeader.bmiHeader.biBitCount = 24;
      dataHeader.bmiHeader.biCompression = BI_RGB;
      dataHeader.bmiHeader.biSizeImage = dataWidth*this->Size[1];
      dataHeader.bmiHeader.biClrUsed = 0;
      dataHeader.bmiHeader.biClrImportant = 0;

      // try using a DIBsection
      backBuffer = CreateDIBSection(this->DeviceContext, &dataHeader,
                             DIB_RGB_COLORS, (void **)(&(this->DIBPtr)), 
                             NULL, 0);

	  // Put the current background into the device context
	  BitBlt(compatHdc, 0, 0, this->Size[0], this->Size[1], this->DeviceContext, 0, 0, SRCCOPY);
	  compatHdc = (HDC) CreateCompatibleDC(this->DeviceContext);
	  }
	else
	  {
      BITMAP bm;
      GetObject(backBuffer, sizeof (BITMAP), (LPSTR) &bm);
      // If region size differs from bitmap size, reallocate the bitmap
	  if ((this->Size[0] != bm.bmWidth) || (this->Size[1] != bm.bmHeight))
        {
        DeleteObject(backBuffer);
	    
        dataHeader.bmiHeader.biWidth = this->Size[0];
        dataHeader.bmiHeader.biHeight = this->Size[1];
        dataHeader.bmiHeader.biSizeImage = dataWidth*this->Size[1];

        // try using a DIBsection
        backBuffer = CreateDIBSection(this->DeviceContext, &dataHeader,
                             DIB_RGB_COLORS, (void **)(&(this->DIBPtr)), 
                             NULL, 0);

	    // Put the current background into the device context
	    BitBlt(compatHdc, 0, 0, this->Size[0], this->Size[1], this->DeviceContext, 0, 0, SRCCOPY);
        }
	  }

    // Put buffer into a compatible device context
    SelectObject(compatHdc, backBuffer);

	// Save the old device context so we can restore it later
    oldHdc = this->DeviceContext;

	// Swap device contexts
    this->DeviceContext = compatHdc;
	swapFlag = 1;
    }
  else
    {
    // Put the buffer on the screen
	vtkDebugMacro (<<"Swapping buffer");
	BitBlt(oldHdc, 0, 0, this->Size[0], this->Size[1], this->DeviceContext, 0, 0, SRCCOPY);
	// Swap the device contexts back
	this->DeviceContext = oldHdc;
    swapFlag = 0;
    }

}


//----------------------------------------------------------------------------
vtkWin32ImageWindow::vtkWin32ImageWindow()
{
  this->ApplicationInstance = NULL;
  this->Palette = NULL;
  this->WindowId = 0;
  this->ParentId = 0;
  this->DeviceContext = (HDC)0;
  this->OwnWindow = 0;
  this->Pen = NULL;
  this->PenColor = RGB(255,0,0);

  this->SetWindowName("Visualization Toolkit - InageWin32");
  this->DIBPtr = (unsigned char*) NULL;
}


//----------------------------------------------------------------------------
vtkWin32ImageWindow::~vtkWin32ImageWindow()
{

  if ((this->OwnWindow) && (this->WindowName))
    {
    delete this->WindowName;
    }

}


//----------------------------------------------------------------------------
void vtkWin32ImageWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageWindow::PrintSelf(os, indent);
}

// Description:
// Get the position in screen coordinates of the window.
int *vtkWin32ImageWindow::GetPosition(void)
{

  vtkDebugMacro (<< "vtkWin32ImageWindow::GetPosition");

  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return(this->Position);
    }

  //#### We need to fix this!!!!
  return this->Position;
}


void vtkWin32ImageWindow::SetBackgroundColor(float r, float g, float b)
{

  vtkDebugMacro(<<"vtkWin32ImageWindow::SetBackgroundColor");

  BYTE red = r * 255.0;
  BYTE green = g * 255.0;
  BYTE blue = b * 255.0;

  COLORREF value = SetBkColor(this->DeviceContext, RGB(red, green, blue));

  if (value == CLR_INVALID)  
    vtkDebugMacro(<<"vtkWin32ImageWindow::SetBackgroundColor - operation failed");

}

void vtkWin32ImageWindow::EraseWindow()
{

  vtkDebugMacro(<<"vtkWin32ImageWindow::EraseWindow");

  int* size = this->GetSize();

  RECT rectSize;
  rectSize.top = 0;
  rectSize.left = 0;
  rectSize.right = size[0] - 1;
  rectSize.bottom = size[1] - 1;

  COLORREF backColor = GetBkColor(this->DeviceContext);	

  if (backColor == CLR_INVALID)
    {
    vtkErrorMacro(<<"vtkWin32ImageWindow::EraseWindow - Invalid background color");
    backColor = RGB(0,0,0);  // black
    }

  if (GetBkMode(this->DeviceContext) == OPAQUE)
    {
    vtkDebugMacro(<<"vtkWin32ImageWindow::EraseWindow - Background is opaque");
    }
  else
    {
    vtkDebugMacro(<<"vtkWin32ImageWindow::EraseWindow - Background is transparent");
    }
      
  HBRUSH hBrush = CreateSolidBrush(backColor);

  FillRect(this->DeviceContext, &rectSize, hBrush);

}


void vtkWin32ImageWindow::SetPosition(int x, int y)
{
  static int resizing = 0;

  vtkDebugMacro (<< "vtkWin32ImageWindow::SetPosition: " << x << "," << y);

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


void vtkWin32ImageWindow::SetSize(int x, int y)
{
  static int resizing = 0;

  // vtkDebugMacro (<< "vtkWin32ImageWindow::SetSize: " << x << "," << y);

  if ((this->Size[0] != x) || (this->Size[1] != y))
    {
    
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    if (this->Mapped)
      {
      if (!resizing)
        {
        BITMAPINFO dataHeader;
        resizing = 1;

        if (this->ParentId)
          {
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

// Description:
// Get the current size of the window.
int *vtkWin32ImageWindow::GetSize(void)
{
  RECT rect;

  // vtkDebugMacro (<< "vtkWin32ImageWindow::GetSize");

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

void vtkWin32ImageWindowSetupRGBPixelFormat(HDC hDC)
{
  PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),  /* size */
    1,                              /* version */
    PFD_DRAW_TO_WINDOW,
    PFD_TYPE_RGBA,                   /* color type */
    24,                             /* prefered color depth */
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

  pixelFormat = ChoosePixelFormat(hDC, &pfd);
  if (pixelFormat == 0) {
  MessageBox(WindowFromDC(hDC), "ChoosePixelFormat failed.", "Error",
	     MB_ICONERROR | MB_OK);
  exit(1);
  }

  if (SetPixelFormat(hDC, pixelFormat, &pfd) != TRUE) {
  MessageBox(WindowFromDC(hDC), "SetPixelFormat failed.", "Error",
	     MB_ICONERROR | MB_OK);
  exit(1);
  }
}

void vtkWin32ImageWindowSetupGrayPixelFormat(HDC hDC)
{
  PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),  /* size */
    1,                              /* version */
    PFD_DRAW_TO_WINDOW,
    PFD_TYPE_COLORINDEX,            /* color type */
    8,                              /* prefered color depth */
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

  pixelFormat = ChoosePixelFormat(hDC, &pfd);
  if (pixelFormat == 0) {
  MessageBox(WindowFromDC(hDC), "ChoosePixelFormat failed.", "Error",
	     MB_ICONERROR | MB_OK);
  exit(1);
  }

  if (SetPixelFormat(hDC, pixelFormat, &pfd) != TRUE) {
  MessageBox(WindowFromDC(hDC), "SetPixelFormat failed.", "Error",
	     MB_ICONERROR | MB_OK);
  exit(1);
  }
}

// creates and applies a RGB palette
void vtkWin32ImageWindowSetupRGBPalette(HDC hDC, 
					vtkWin32ImageWindow *me)
{
  int pixelFormat = GetPixelFormat(hDC);
  PIXELFORMATDESCRIPTOR pfd;
  LOGPALETTE* pPal;
  int paletteSize;
    
  DescribePixelFormat(hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
  
  if (pfd.dwFlags & PFD_NEED_PALETTE) 
    {
    paletteSize = 1 << pfd.cColorBits;
    } 
  else 
    {
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
  
  for (i=0; i<paletteSize; ++i) 
    {
    pPal->palPalEntry[i].peRed =
      (((i >> pfd.cRedShift) & redMask) * 255) / redMask;
    pPal->palPalEntry[i].peGreen =
      (((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask;
    pPal->palPalEntry[i].peBlue =
      (((i >> pfd.cBlueShift) & blueMask) * 255) / blueMask;
    pPal->palPalEntry[i].peFlags = 0;
    }
  }
  
  me->Palette = CreatePalette(pPal);
  free(pPal);
  
  if (me->Palette) 
    {
    SelectPalette(hDC, me->Palette, FALSE);
    RealizePalette(hDC);
    }

}
void vtkWin32ImageWindow::SetPenColor(float r,float g,float b) 
{
  int ir,ig,ib;
  if (this->DeviceContext != (HDC) 0) 
    {
    ir = 255*r;
    ig = 255*g;
    ib = 255*b;

    this->PenColor = RGB(ir,ig,ib);
    if (this->Pen != NULL) 
      {
      DeleteObject(this->Pen);
      this->Pen = CreatePen(PS_SOLID,0,this->PenColor);
      }
    }
}

void vtkWin32ImageWindow::PenLineTo(int x,int y) 
{
  HPEN hPenOld;
  if (this->DeviceContext != (HDC) 0) 
    {
    hPenOld = (HPEN) SelectObject(this->DeviceContext,this->Pen);
    LineTo(this->DeviceContext,x,y);
    SelectObject(this->DeviceContext,hPenOld);
    }
}
void vtkWin32ImageWindow::PenMoveTo(int x,int y) 
{
  if (this->DeviceContext != (HDC) 0) 
    {
    MoveToEx(this->DeviceContext,x,y,NULL);
    }
}

void vtkWin32ImageWindowSetupGrayPalette(HDC hDC, 
					 vtkWin32ImageWindow *me)
{
  int pixelFormat = GetPixelFormat(hDC);
  PIXELFORMATDESCRIPTOR pfd;
  LOGPALETTE* pPal;
  int paletteSize;
  
  DescribePixelFormat(hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
  
  // we always want a palette on 8 bit displays
  if (pfd.cColorBits == 8 || pfd.dwFlags & PFD_NEED_PALETTE) 
    {
    paletteSize = 1 << pfd.cColorBits;
    } 
  else 
    {
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
  
  for (i=0; i<paletteSize; ++i) 
    {
    pPal->palPalEntry[i].peRed = (255*i)/paletteSize;
    pPal->palPalEntry[i].peGreen = (255*i)/paletteSize;
    pPal->palPalEntry[i].peBlue = (255*i)/paletteSize;
    pPal->palPalEntry[i].peFlags = 0;
    }
  }
  
  me->Palette = CreatePalette(pPal);
  free(pPal);
  
  if (me->Palette) 
    {
    SelectPalette(hDC, me->Palette, FALSE);
    RealizePalette(hDC);
    }
}

// used to pass info into the create routine because there doesn't
// seem to be another way. Could be a problem for multithreaded
// apps but this is unlikely since this doesn't get called very
// often at all.
vtkWin32ImageWindow *vtkWin32ImageWindowPtr = NULL;

LRESULT APIENTRY vtkWin32ImageWindowWndProc(HWND hWnd, UINT message, 
					    WPARAM wParam, LPARAM lParam)
{
  vtkWin32ImageWindow *me =   
    (vtkWin32ImageWindow *)GetWindowLong(hWnd,GWL_USERDATA);

  switch (message) 
    {
    case WM_CREATE:
      {
      me = vtkWin32ImageWindowPtr;
      SetWindowLong(hWnd,GWL_USERDATA,(LONG)me);
      me->DeviceContext = (HDC) GetDC(hWnd);
      me->Pen = CreatePen(PS_SOLID,0,me->PenColor);
      // #### 7/29/97 mwt
      SetMapMode (me->DeviceContext, MM_TEXT);
      // #### 8/13/97 mwt
      //SetBkColor (me->DeviceContext, RGB(me->Background[0], me->Background[1], me->Background[2]));
      SetBkColor (me->DeviceContext, RGB(0,0,0));
      if (me->GetGrayScaleHint())
	{
	vtkWin32ImageWindowSetupGrayPixelFormat(me->DeviceContext);
	vtkWin32ImageWindowSetupGrayPalette(me->DeviceContext,me);
	}
      else
	{
	vtkWin32ImageWindowSetupRGBPixelFormat(me->DeviceContext);
	vtkWin32ImageWindowSetupRGBPalette(me->DeviceContext,me);
	}
      return 0;
      }
    case WM_DESTROY:
      if (me->Palette)
	{
	DeleteObject(me->Palette);
	me->Palette = NULL;
	}
      ReleaseDC(me->WindowId, me->DeviceContext);
      return 0;
    case WM_SIZE:
      /* track window size changes */
      if (me->DeviceContext) 
	{
	me->SetSize((int) LOWORD(lParam),(int) HIWORD(lParam));
	return 0;
	}
    case WM_PALETTECHANGED:
      /* realize palette if this is *not* the current window */
      if (me->DeviceContext && me->Palette && (HWND) wParam != hWnd) 
	{
	UnrealizeObject(me->Palette);
	SelectPalette(me->DeviceContext, me->Palette, FALSE);
	RealizePalette(me->DeviceContext);
	me->Render();
	break;
	}
      break;
    case WM_QUERYNEWPALETTE:
      /* realize palette if this is the current window */
      if (me->DeviceContext && me->Palette) 
	{
	UnrealizeObject(me->Palette);
	SelectPalette(me->DeviceContext, me->Palette, FALSE);
	RealizePalette(me->DeviceContext);
	me->Render();
	return TRUE;
	}
      break;
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      BeginPaint(hWnd, &ps);
      if (me->DeviceContext) 
	{
	me->Render();
	}
      EndPaint(hWnd, &ps);
      return 0;
      }
      break;
    default:
      break;
    }
  return DefWindowProc(hWnd, message, wParam, lParam);
}


//----------------------------------------------------------------------------
void vtkWin32ImageWindow::MakeDefaultWindow() 
{
  static int count = 0;

  vtkDebugMacro (<< "vtkWin32ImageWindow::MakeDefaultWindow");

  // create our own window if not already set
  // #### commented out 7/31/97
  // this->OwnWindow = 0;

  // get the applicaiton instance if we don't have one already
  if (!this->ApplicationInstance)
    {
    // if we have a parent window get the app instance from it
    if (this->ParentId)
      {
      this->ApplicationInstance = 
	(HINSTANCE)GetWindowLong(this->ParentId,GWL_HINSTANCE);
      }
    else
      {
      this->ApplicationInstance = GetModuleHandle(NULL);
      }
    }
  if (!this->WindowId)
    {
    WNDCLASS wndClass;

    int len = strlen( "Visualization Toolkit - ImageWin32 #") 
      + (int)ceil( (double) log10( (double)(count+1) ) ) + 1; 
    char* tempName = new char [len];
    sprintf(tempName,"Visualization Toolkit - ImageWin32 #%i",count++);
    this->SetWindowName(tempName);

    // has the class been registered ?
    if (!GetClassInfo(this->ApplicationInstance,"vtkImage",&wndClass))
        {
		// ### Double buffering - added CS_OWNDC
        wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wndClass.lpfnWndProc = vtkWin32ImageWindowWndProc;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = this->ApplicationInstance;
        wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = "vtkImage";
        RegisterClass(&wndClass);
        }
    
	
    /* create window */
    // use poor mans mutex
    if (vtkWin32ImageWindowPtr)
      {
      vtkErrorMacro("Two windows being created at the same time");
      }
    vtkWin32ImageWindowPtr = this;
    if (this->ParentId)
      {
      this->WindowId = 
	CreateWindow("vtkImage", this->WindowName,
		     WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		     0, 0, this->Size[0], this->Size[1],
		     this->ParentId, NULL, this->ApplicationInstance, NULL);
      }
    else
      {
      this->WindowId = 
	CreateWindow("vtkImage", this->WindowName,
		     WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		     0, 0, this->Size[0]+2*GetSystemMetrics(SM_CXFRAME),
		     this->Size[1] + 2*GetSystemMetrics(SM_CYFRAME) + 
		     GetSystemMetrics(SM_CYCAPTION),
		     NULL, NULL, this->ApplicationInstance, NULL);
      }
    vtkWin32ImageWindowPtr = NULL;
    if (!this->WindowId)
      {
      vtkErrorMacro("Could not create window, error:  " << GetLastError());
      return;
      }
    
    /* display window */
    this->WindowCreated = 1;
    this->OwnWindow = 1;

    ShowWindow(this->WindowId, SW_SHOW);
    
    }

  this->Mapped = 1;
}

// Description:
// Get the window id.
HWND vtkWin32ImageWindow::GetWindowId()
{
  vtkDebugMacro(<< "vtkWin32ImageWindow::GetWindowId - Returning WindowId of " << this->WindowId << "\n"); 

  return this->WindowId;
}

// Description:
// Set the window id to a pre-existing window.
void vtkWin32ImageWindow::SetWindowId(HWND arg)
{
  vtkDebugMacro(<< "vtkWin32ImageWindow::SetWindowID - Setting WindowId to " << arg << "\n"); 

  this->WindowId = arg;
}

// Description:
// Set the window id to a pre-existing window.
void vtkWin32ImageWindow::SetParentId(HWND arg)
{
  vtkDebugMacro(<< "vtkWin32ImageWindow::SetParentID - Setting ParentId to " << arg << "\n"); 

  this->ParentId = arg;
}


