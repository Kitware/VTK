/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWin32Viewer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


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
#include "vtkImageWin32Viewer.h"

//----------------------------------------------------------------------------
vtkImageWin32Viewer::vtkImageWin32Viewer()
{
  this->ApplicationInstance = NULL;
  this->Palette = NULL;
  this->WindowId = 0;
  this->ParentId = 0;
  this->DeviceContext = (HDC)0;
  this->OwnWindow = 0;
  
  if ( this->WindowName )
    delete [] this->WindowName;
  this->WindowName = strdup("Visualization Toolkit - ImageWin32");
}


//----------------------------------------------------------------------------
vtkImageWin32Viewer::~vtkImageWin32Viewer()
{
}


//----------------------------------------------------------------------------
void vtkImageWin32Viewer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageViewer::PrintSelf(os, indent);
}

// Description:
// Get the position in screen coordinates of the window.
int *vtkImageWin32Viewer::GetPosition(void)
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

void vtkImageWin32Viewer::SetPosition(int x, int y)
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

//----------------------------------------------------------------------------
// Description:
// A templated function that handles gray scale images.
template <class T>
static void vtkImageWin32ViewerRenderGrey(vtkImageWin32Viewer *self, 
					  vtkImageRegion *region,
					  T *inPtr, unsigned char *outPtr,
					  float shift, float scale)
{
  int colorIdx;
  T *inPtr0, *inPtr1;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx0, idx1;
  
  region->GetExtent(inMin0, inMax0, inMin1, inMax1);
  region->GetIncrements(inInc0, inInc1);
  
  // Loop through in regions pixels
  inPtr1 = inPtr;
  for (idx1 = inMin1; idx1 <= inMax1; idx1++)
    {
    inPtr0 = inPtr1;
    for (idx0 = inMin0; idx0 <= inMax0; idx0++)
      {

      colorIdx = (int)(((float)(*inPtr0) + shift) * scale);
      if (colorIdx < 0)
	      {
	      colorIdx = 0;
	      }
      if (colorIdx > 255)
	      {
	      colorIdx = 255;
	      }

      *outPtr++ = (unsigned char)(colorIdx);
      *outPtr++ = (unsigned char)(colorIdx);
      *outPtr++ = (unsigned char)(colorIdx);
      inPtr0 += inInc0;
      }
    // rows must be a multiple of four bytes
    // so pad it if neccessary
    outPtr = outPtr + (4 - ((inMax0-inMin0 + 1)*3)%4)%4;
    inPtr1 += inInc1;
    }
}


//----------------------------------------------------------------------------
// Description:
// A templated function that handles color images. (only True Color 24 bit)
template <class T>
static void vtkImageWin32ViewerRenderColor(vtkImageWin32Viewer *self, 
					   vtkImageRegion *region,
					   T *redPtr, T *greenPtr, T *bluePtr,
					   unsigned char *outPtr,
					   float shift, float scale)
{
  int red, green, blue;
  T *redPtr0, *redPtr1;
  T *bluePtr0, *bluePtr1;
  T *greenPtr0, *greenPtr1;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx0, idx1;
  
  region->GetExtent(inMin0, inMax0, inMin1, inMax1);
  region->GetIncrements(inInc0, inInc1);
  
  // Loop through in regions pixels
  redPtr1 = redPtr;
  greenPtr1 = greenPtr;
  bluePtr1 = bluePtr;
  for (idx1 = inMin1; idx1 <= inMax1; idx1++)
    {
    redPtr0 = redPtr1;
    greenPtr0 = greenPtr1;
    bluePtr0 = bluePtr1;
    for (idx0 = inMin0; idx0 <= inMax0; idx0++)
      {

      red = (int)(((float)(*redPtr0) + shift) * scale);
      if (red < 0) red = 0;
      if (red > 255) red = 255;
      green = (int)(((float)(*greenPtr0) + shift) * scale);
      if (green < 0) green = 0;
      if (green > 255) green = 255;
      blue = (int)(((float)(*bluePtr0) + shift) * scale);
      if (blue < 0) blue = 0;
      if (blue > 255) blue = 255;

      *outPtr++ = (unsigned char)(blue);
      *outPtr++ = (unsigned char)(green);
      *outPtr++ = (unsigned char)(red);

      redPtr0 += inInc0;
      greenPtr0 += inInc0;
      bluePtr0 += inInc0;
      }
    // rows must be a multiple of four bytes
    // so pad it if neccessary
    outPtr = outPtr + (4 - ((inMax0-inMin0 + 1)*3)%4)%4;

    redPtr1 += inInc1;
    greenPtr1 += inInc1;
    bluePtr1 += inInc1;
    }
}

void vtkImageWin32Viewer::SetSize(int x, int y)
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

        SetWindowPos(this->WindowId,HWND_TOP,0,0,
          x+2*GetSystemMetrics(SM_CXFRAME),
          y+2*GetSystemMetrics(SM_CYFRAME) +GetSystemMetrics(SM_CYCAPTION),
          SWP_NOMOVE | SWP_NOZORDER);
        resizing = 0;
        }
      }
    }
}

// Description:
// Get the current size of the window.
int *vtkImageWin32Viewer::GetSize(void)
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

//----------------------------------------------------------------------------
// Maybe we should cache the dataOut! (MTime)
void vtkImageWin32Viewer::Render(void)
{
  int extent[8];
  int *imageExtent;
  int dataWidth, width, height;
  int size;
  unsigned char *dataOut;
  vtkImageRegion *region;
  void *ptr0, *ptr1, *ptr2;
  float shift, scale;
  BITMAPINFO dataHeader;
  
  if ( ! this->Input)
    {
    // open the window anyhow if one has not been set.
    if (!this->DeviceContext)
      {
      // use default size if not specified
      if (this->Size[0] == 0 )
        {
        this->Size[0] = width;
        this->Size[1] = height;
        }
      if (this->Size[0] == 0 )
        {
        this->Size[0] = 256;
        this->Size[1] = 256;
        }
      this->MakeDefaultWindow();
      }

    vtkErrorMacro(<< "Render: Please Set the input.");
    return;
    }

  this->Input->UpdateImageInformation(&(this->Region));
  imageExtent = this->Region.GetImageExtent();
  
  // determine the Extent of the 2D input region needed
  if (this->WholeImage)
    {
    this->Region.GetImageExtent(2, extent);
    }
  else
    {
    this->Region.GetExtent(2, extent);
    }
  
  if (this->ColorFlag)
    {
    extent[4] = extent[5] = this->Red;
    if (this->Green < extent[4]) extent[4] = this->Green;
    if (this->Green > extent[5]) extent[5] = this->Green;
    if (this->Blue < extent[4]) extent[4] = this->Blue;
    if (this->Blue > extent[5]) extent[5] = this->Blue;
    }
  else
    {
    // Make sure the requested  image is in the range.
    if (this->Coordinate2 < imageExtent[4])
      {
      extent[4] = extent[5] = imageExtent[4];
      }
    else if (this->Coordinate2 > imageExtent[5])
      {
      extent[4] = extent[5] = imageExtent[5];
      }
    else
      {
      extent[4] = extent[5] = this->Coordinate2;
      }
    }

  // Make sure the requested  image is in the range.
  if (this->Coordinate3 < imageExtent[6])
    {
    extent[6] = extent[6] = imageExtent[6];
    }
  else if (this->Coordinate3 > imageExtent[7])
    {
    extent[6] = extent[7] = imageExtent[7];
    }
  else
    {
    extent[6] = extent[7] = this->Coordinate3;
    }
  
  // Get the region form the input
  region = new vtkImageRegion;
  region->SetAxes(this->Region.GetAxes());
  region->SetExtent(4, extent);
  this->Input->UpdateRegion(region);
  if ( ! region->AreScalarsAllocated())
    {
    vtkErrorMacro(<< "View: Could not get region from input.");
    region->Delete();
    return;
    }

  // allocate the display data array.
  width = (extent[1] - extent[0] + 1);
  height = (extent[3] - extent[2] + 1);

  // In case a window has not been set.
  if (!this->DeviceContext)
    {
    // use default size if not specified
    if (this->Size[0] == 0 )
      {
      this->Size[0] = width;
      this->Size[1] = height;
      }
    this->MakeDefaultWindow();
    }
  
  // Allocate output data
  dataWidth = ((width*3+3)/4)*4;
  size = dataWidth * height;
  dataOut = new unsigned char[size];

  shift = this->ColorWindow / 2.0 - this->ColorLevel;
  scale = 255.0 / this->ColorWindow;

  if (this->ColorFlag)
    {
    ptr0 = region->GetScalarPointer(extent[0], extent[2], this->Red);
    ptr1 = region->GetScalarPointer(extent[0], extent[2], this->Green);
    ptr2 = region->GetScalarPointer(extent[0], extent[2], this->Blue);
    // Call the appropriate templated function
    switch (region->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageWin32ViewerRenderColor(this, region, 
			   (float *)(ptr0),(float *)(ptr1),(float *)(ptr2), 
			   dataOut, shift, scale);
	break;
      case VTK_INT:
	vtkImageWin32ViewerRenderColor(this, region, 
			   (int *)(ptr0), (int *)(ptr1), (int *)(ptr2), 
			   dataOut, shift, scale);
	break;
      case VTK_SHORT:
	vtkImageWin32ViewerRenderColor(this, region, 
			   (short *)(ptr0),(short *)(ptr1),(short *)(ptr2), 
			   dataOut, shift, scale);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageWin32ViewerRenderColor(this, region, (unsigned short *)(ptr0),
			   (unsigned short *)(ptr1),(unsigned short *)(ptr2), 
			    dataOut, shift, scale);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageWin32ViewerRenderColor(this, region, (unsigned char *)(ptr0), 
			   (unsigned char *)(ptr1),(unsigned char *)(ptr2), 
			    dataOut, shift, scale);
	break;
      }
    }
  else
    {
    // GreyScale images.
    ptr0 = region->GetScalarPointer();
    // Call the appropriate templated function
    switch (region->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageWin32ViewerRenderGrey(this, region, (float *)(ptr0), dataOut,
				      shift, scale);
	break;
      case VTK_INT:
	vtkImageWin32ViewerRenderGrey(this, region, (int *)(ptr0), dataOut,
				      shift, scale);
	break;
      case VTK_SHORT:
	vtkImageWin32ViewerRenderGrey(this, region, (short *)(ptr0), dataOut,
				      shift, scale);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageWin32ViewerRenderGrey(this, region, (unsigned short *)(ptr0), 
				      dataOut, shift, scale);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageWin32ViewerRenderGrey(this, region, (unsigned char *)(ptr0), 
				      dataOut, shift, scale);
	break;
      }   
    }
  
  // Display the image.
  //dataHeader.bmiColors[0] = NULL;
  dataHeader.bmiHeader.biSize = 40;
  dataHeader.bmiHeader.biWidth = width;
  dataHeader.bmiHeader.biHeight = height;
  dataHeader.bmiHeader.biPlanes = 1;
  dataHeader.bmiHeader.biBitCount = 24;
  dataHeader.bmiHeader.biCompression = BI_RGB;
  dataHeader.bmiHeader.biSizeImage = size;
  dataHeader.bmiHeader.biClrUsed = 0;
  dataHeader.bmiHeader.biClrImportant = 0;
  
  SetDIBitsToDevice(this->DeviceContext,0,0,width,height,0,0,0,height,
		    dataOut,&dataHeader,DIB_RGB_COLORS);
  
  delete dataOut;	 
  region->Delete();
}

void vtkImageWin32ViewerSetupRGBPixelFormat(HDC hDC)
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

void vtkImageWin32ViewerSetupGreyPixelFormat(HDC hDC)
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

struct vtkImageWin32ViewerCreateInfo
  {
  HDC DeviceContext;
  HPALETTE Palette;
  };

// creates and applies a RGB palette
void vtkImageWin32ViewerSetupRGBPalette(HDC hDC, 
				     vtkImageWin32ViewerCreateInfo *me)
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

void vtkImageWin32ViewerSetupGreyPalette(HDC hDC, 
				     vtkImageWin32ViewerCreateInfo *me)
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
static int vtkImageWin32DoGrey;

LRESULT APIENTRY vtkImageWin32ViewerWndProc(HWND hWnd, UINT message, 
					    WPARAM wParam, LPARAM lParam)
{
  vtkImageWin32Viewer *me = 
    (vtkImageWin32Viewer *)GetWindowLong(hWnd,GWL_USERDATA);

  switch (message) 
    {
    case WM_CREATE:
      {
        // this code is going to create some stuff that we want to
        // associate with the this pointer. But since there isn't an
        // easy way to tget the this pointer during the create call
        // we'll pass the created info back out
        vtkImageWin32ViewerCreateInfo *info = 
	      new vtkImageWin32ViewerCreateInfo;
        SetWindowLong(hWnd,GWL_USERDATA,(LONG)info);
        info->DeviceContext = GetDC(hWnd);
        if (vtkImageWin32DoGrey)
          {
          vtkImageWin32ViewerSetupGreyPixelFormat(info->DeviceContext);
          vtkImageWin32ViewerSetupGreyPalette(info->DeviceContext,info);
          }
        else
          {
          vtkImageWin32ViewerSetupRGBPixelFormat(info->DeviceContext);
          vtkImageWin32ViewerSetupRGBPalette(info->DeviceContext,info);
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
void vtkImageWin32Viewer::MakeDefaultWindow() 
{
  int count;

  // create our own window if not already set
  this->OwnWindow = 0;

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
      this->ApplicationInstance = AfxGetInstanceHandle();
      }
    }
  if (!this->WindowId)
    {
    WNDCLASS wndClass;
    vtkImageWin32ViewerCreateInfo *info;
    
    if(this->WindowName) delete [] this->WindowName;
    int len = strlen( "Visualization Toolkit - ImageWin32 #") 
      + (int)ceil( (double) log10( (double)(count+1) ) ) + 1; 
    this->WindowName = new char [ len ];
    sprintf(this->WindowName,"Visualization Toolkit - ImageWin32 #%i",count++);
    
    // has the class been registered ?
    if (!GetClassInfo(this->ApplicationInstance,"vtkImage",&wndClass))
        {
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = vtkImageWin32ViewerWndProc;
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
    vtkImageWin32DoGrey = this->GreyScale;
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
		     0, 0, this->Size[0], this->Size[1],
		     NULL, NULL, this->ApplicationInstance, NULL);
      }
    if (!this->WindowId)
      {
      vtkErrorMacro("Could not create window, error:  " << GetLastError());
      return;
      }
    // extract the create info
    info = (vtkImageWin32ViewerCreateInfo *)
      GetWindowLong(this->WindowId,GWL_USERDATA);
    this->DeviceContext = info->DeviceContext;
    this->Palette = info->Palette;
    delete info;
    SetWindowLong(this->WindowId,GWL_USERDATA,(LONG)this);
    
    /* display window */
    ShowWindow(this->WindowId, SW_SHOW);
    this->OwnWindow = 1;
    }
  this->Mapped = 1;
}

// Description:
// Get the window id.
HWND vtkImageWin32Viewer::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << this->WindowId << "\n"); 

  return this->WindowId;
}

// Description:
// Set the window id to a pre-existing window.
void vtkImageWin32Viewer::SetWindowId(HWND arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << arg << "\n"); 

  this->WindowId = arg;
}

// Description:
// Set the window id to a pre-existing window.
void vtkImageWin32Viewer::SetParentId(HWND arg)
{
  vtkDebugMacro(<< "Setting ParentId to " << arg << "\n"); 

  this->ParentId = arg;
}

