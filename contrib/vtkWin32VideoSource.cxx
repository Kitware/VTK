/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32VideoSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "vtkWin32VideoSource.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkWin32VideoSource* vtkWin32VideoSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32VideoSource");
  if(ret)
    {
    return (vtkWin32VideoSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32VideoSource;
}

//----------------------------------------------------------------------------
vtkWin32VideoSource::vtkWin32VideoSource()
{
  this->Initialized = 0;

  this->FrameRate = 30;

  this->FlipFrames = 0;
  this->FrameBufferRowAlignment = 4;

  this->CapWnd = NULL;
  this->ParentWnd = NULL;
  this->BitMapSize = 0;
  this->BitMapPtr = NULL;

  this->FatalVFWError = 0;
}

//----------------------------------------------------------------------------
vtkWin32VideoSource::~vtkWin32VideoSource()
{ 
  this->vtkWin32VideoSource::ReleaseSystemResources();

  if (this->BitMapPtr != NULL)
    {
    delete [] (char *)(this->BitMapPtr);
    }
  this->BitMapPtr = NULL;
  this->BitMapSize = 0;
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWin32VideoSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// This is empty for now because we aren't displaying the capture window
LONG FAR PASCAL
vtkWin32VideoSourceWinProc(HWND hwnd, UINT message, 
			   WPARAM wParam, LPARAM lParam)
{
  switch(message) {

  /* Let all messages pass through
  case WM_MOVE:
    // cerr << "WM_MOVE\n";
    break;

  case WM_SIZE:
    // cerr << "WM_SIZE\n";
    break;
    
  case WM_DESTROY:
    // cerr << "WM_DESTROY\n";    
    break;

  case WM_CLOSE:
    // cerr << "WM_CLOSE\n";
    break;
  */

  default:
    return(DefWindowProc(hwnd, message, wParam, lParam));
  }
  return 0;
}

//----------------------------------------------------------------------------
LRESULT PASCAL vtkWin32VideoSourceCallbackProc(HWND hwndC, LPVIDEOHDR lpVHdr)
{
  vtkWin32VideoSource *self = (vtkWin32VideoSource *)(capGetUserData(hwndC));
  self->InternalGrab(lpVHdr);

  return 0;
}

//----------------------------------------------------------------------------
// this callback is left in for debug purposes
LRESULT PASCAL vtkWin32VideoSourceStatusCallbackProc(HWND hwndC, int nID, 
						     LPCSTR lpsz)
{
  vtkWin32VideoSource *self = (vtkWin32VideoSource *)(capGetUserData(hwndC));

  /* 
  if (nID == IDS_CAP_BEGIN)
    {
    cerr << "start of capture\n";
    }

  if (nID == IDS_CAP_END)
    {
    cerr << "end of capture\n";
    }
  */

  return 1;
}

//----------------------------------------------------------------------------
LRESULT PASCAL vtkWin32VideoSourceErrorCallbackProc(HWND hwndC,
						    int ErrID, 
						    LPSTR lpErrorText)
{
  if (ErrID)
    {
    char buff[84];
    sprintf(buff,"Error# %d",ErrID);
    MessageBox(hwndC,lpErrorText, buff, MB_OK | MB_ICONEXCLAMATION);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::Initialize()
{
  int i;

  if (this->Initialized || this->FatalVFWError)
    {
    return;
    }

  // Preliminary update of frame buffer, just in case we don't get
  // though the initialization but need the framebuffer for Updates
  this->UpdateFrameBuffer();

  // It is necessary to create not one, but two windows in order to
  // do frame grabbing under VFW.  Why do we need any?

  // get necessary process info
  HINSTANCE hinstance = GetModuleHandle(NULL);

  strcpy(this->WndClassName,"VTKVideo");

  // set up a class for the main window
  WNDCLASS wc;
  wc.lpszClassName = this->WndClassName;
  wc.hInstance = hinstance;
  wc.lpfnWndProc = &vtkWin32VideoSourceWinProc;
  wc.hCursor = LoadCursor(NULL,IDC_ARROW);
  wc.hIcon = NULL;
  wc.lpszMenuName = NULL;
  wc.hbrBackground = NULL;
  wc.style = CS_HREDRAW|CS_VREDRAW;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
    
  for (i = 1; i <= 10; i++)
    {
    if (RegisterClass(&wc))
      {
      break;
      }
    // try again with a slightly different name
    sprintf(this->WndClassName,"VTKVideo %d",i);
    }
    
  if (i > 32)
    {
    vtkErrorMacro(<< "Initialize: failed to register VTKVideo class"\
                    << " (" << GetLastError() << ")");
    return;
    }

  DWORD style = WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

  if (this->Preview)
    {
    style |= WS_VISIBLE;
    }

  // set up the parent window, but don't show it
  this->ParentWnd = CreateWindow(
		"VTKVideo",
		"VTK Video Window",
		style,
                0, 0, 
		this->FrameSize[0]+2*GetSystemMetrics(SM_CXFRAME), 
		this->FrameSize[1]+2*GetSystemMetrics(SM_CYFRAME)
                                  +GetSystemMetrics(SM_CYBORDER)
		                  +GetSystemMetrics(SM_CYSIZE),
                NULL,
                NULL,
                hinstance,
                NULL);
    
  if (!this->ParentWnd) 
    {
    vtkErrorMacro(<< "Initialize: failed to create window"\
                    << " (" << GetLastError() << ")");
    return;
    }

  // Create the capture window
  this->CapWnd = capCreateCaptureWindow("Capture",
		      WS_CHILD|WS_VISIBLE, 0, 0, 
		      this->FrameSize[0], this->FrameSize[1],
		      this->ParentWnd,1);

  if (!this->CapWnd) 
    {
    vtkErrorMacro(<< "Initialize: failed to create capture window"\
                    << " (" << GetLastError() << ")");
    this->ReleaseSystemResources();
    return;
    }

  // connect to the driver
  if (!capDriverConnect(this->CapWnd,0))
    {
    MessageBox(this->ParentWnd, "Can't find video hardware", "", 
	       MB_OK | MB_ICONEXCLAMATION);
    vtkErrorMacro(<< "Initialize: couldn't connect to driver"\
                    << " (" << GetLastError() << ")");
    this->ReleaseSystemResources();
    this->FatalVFWError = 1;
    return;
    }

  capDriverGetCaps(this->CapWnd,&this->CapDriverCaps,sizeof(CAPDRIVERCAPS));
  
  // set up the video format
  this->DoVFWFormatSetup();

  // set the capture parameters
  capCaptureGetSetup(this->CapWnd,&this->CaptureParms,sizeof(CAPTUREPARMS));
    
  if (this->FrameRate > 0)
    {
    this->CaptureParms.dwRequestMicroSecPerFrame = 
	                            int(1000000/this->FrameRate);
    }
  else
    {
    this->CaptureParms.dwRequestMicroSecPerFrame = 0;
    }

  this->CaptureParms.fMakeUserHitOKToCapture = FALSE;
  this->CaptureParms.fYield = 1;
  this->CaptureParms.fCaptureAudio = FALSE;
  this->CaptureParms.vKeyAbort = 0x00;
  this->CaptureParms.fAbortLeftMouse = FALSE;
  this->CaptureParms.fAbortRightMouse = FALSE;
  this->CaptureParms.fLimitEnabled = FALSE;
  this->CaptureParms.wNumAudioRequested = 0;
  this->CaptureParms.wPercentDropForError = 100;
  this->CaptureParms.dwAudioBufferSize = 0;
  this->CaptureParms.AVStreamMaster = AVSTREAMMASTER_NONE;
  
  if (!capCaptureSetSetup(this->CapWnd,&this->CaptureParms,
			    sizeof(CAPTUREPARMS)))
    {
    vtkErrorMacro(<< "Initialize: setup of capture parameters failed"\
                    << " (" << GetLastError() << ")");
    this->ReleaseSystemResources();
    return;
    }

  // set user data for callbacks
  if (!capSetUserData(this->CapWnd,(long)this))
    {
    vtkErrorMacro(<< "Initialize: couldn't set user data for callback"\
                    << " (" << GetLastError() << ")");
    this->ReleaseSystemResources();
    return;
    }
    
  // install the callback to copy frames into the buffer on sync grabs
  if (!capSetCallbackOnFrame(this->CapWnd,
			     &vtkWin32VideoSourceCallbackProc))
    {
    vtkErrorMacro(<< "Initialize: couldn't set frame callback"\
                    << " (" << GetLastError() << ")");
    this->ReleaseSystemResources();
    return;
    }
  // install the callback to copy frames into the buffer on stream grabs
  if (!capSetCallbackOnVideoStream(this->CapWnd,
				   &vtkWin32VideoSourceCallbackProc))
    {
    vtkErrorMacro(<< "Initialize: couldn't set stream callback"\
                    << " (" << GetLastError() << ")");
    this->ReleaseSystemResources();
    return;
    }
  // install the callback to decide whether to continue streaming
  if (!capSetCallbackOnStatus(this->CapWnd,
			     &vtkWin32VideoSourceStatusCallbackProc))
    {
    vtkErrorMacro(<< "Initialize: couldn't set status callback"\
                    << " (" << GetLastError() << ")");
    this->ReleaseSystemResources();
    return;
    }
  // install the callback to send messages to user
  if (!capSetCallbackOnError(this->CapWnd,
			     &vtkWin32VideoSourceErrorCallbackProc))
    {
    vtkErrorMacro(<< "Initialize: couldn't set error callback"\
                    << " (" << GetLastError() << ")");
    this->ReleaseSystemResources();
    return;
    }
  
  if (this->Preview)
    {
    capOverlay(this->CapWnd,TRUE);
    }

  // update framebuffer again to reflect any changes which
  // might have occurred
  this->UpdateFrameBuffer();

  this->Initialized = 1;
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::ReleaseSystemResources()
{
  if (this->Playing)
    {
    this->Stop();
    }  

  if (this->CapWnd)
    {
    capDriverDisconnect(this->CapWnd);
    DestroyWindow(this->CapWnd);
    this->CapWnd = NULL;
    }
  if (this->ParentWnd)
    {
    DestroyWindow(this->ParentWnd);
    this->ParentWnd = NULL;
    }
  UnregisterClass(this->WndClassName,GetModuleHandle(NULL));

  this->FatalVFWError = 0;
  this->Initialized = 0;
}

//----------------------------------------------------------------------------
// copy the Device Independent Bitmap from the VFW framebuffer into the
// vtkVideoSource framebuffer (don't do the unpacking yet)
void vtkWin32VideoSource::InternalGrab(LPVIDEOHDR lpVHdr)
{
  // the VIDEOHDR has the following contents, for quick ref:
  //
  // lpData                 pointer to locked data buffer
  // dwBufferLength         Length of data buffer
  // dwBytesUsed            Bytes actually used
  // dwTimeCaptured         Milliseconds from start of stream
  // dwUser                 for client's use
  // dwFlags                assorted flags (see VFW.H)
  // dwReserved[4]          reserved for driver

  unsigned char *cptrDIB = lpVHdr->lpData;

  // get a thread lock on the frame buffer
  this->FrameBufferMutex->Lock();
 
  if (this->AutoAdvance)
    {
    this->AdvanceFrameBuffer(1);
    }

  int index = this->FrameBufferIndex;
  
  this->FrameBufferTimeStamps[index] = vtkTimerLog::GetCurrentTime();

  unsigned char *ptr = (unsigned char *)
    (((vtkScalars *)this->FrameBuffer[index])->GetVoidPointer(0));

  // the DIB has rows which are multiples of 4 bytes
  int outBytesPerRow = ((this->FrameBufferExtent[1]-
			 this->FrameBufferExtent[0]+1)
			* this->FrameBufferBitsPerPixel + 7)/8;
  outBytesPerRow += outBytesPerRow % this->FrameBufferRowAlignment;
  int inBytesPerRow = this->FrameSize[0] 
		       * (this->BitMapPtr->bmiHeader.biBitCount/8);
  outBytesPerRow += outBytesPerRow % 4;
  int rows = this->FrameBufferExtent[3]-this->FrameBufferExtent[2]+1;

  cptrDIB += this->FrameBufferExtent[0]*\
                     (this->BitMapPtr->bmiHeader.biBitCount/8);
  cptrDIB += this->FrameBufferExtent[2]*inBytesPerRow;

  // uncompress or simply copy the DIB
  switch (this->BitMapPtr->bmiHeader.biCompression)
    {
    case BI_RGB:
      if (outBytesPerRow == inBytesPerRow)
	{
	memcpy(ptr,cptrDIB,inBytesPerRow*rows);
	}
      else
	{
	while (--rows >= 0)
	  {
	  memcpy(ptr,cptrDIB,outBytesPerRow);
	  ptr += outBytesPerRow;
	  cptrDIB += inBytesPerRow;
	  }
	}
      break;
    case BI_RLE8:  // not handled
    case BI_RLE4:
    case BI_BITFIELDS:
      break;
    }

  this->Modified();

  this->FrameBufferMutex->Unlock();
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::Grab(int numFrames)
{
  if (numFrames > this->FrameBufferSize || numFrames < 1)
    {
    vtkErrorMacro(<< "Grab: # of frames must be at least 1");
    return;
    }

  if (this->Playing)
    {
    return;
    }

  // ensure that the frame buffer is properly initialized
  this->Initialize();
  if (!this->Initialized)
    {
    return;
    }

  // just do the grabs, the callback does the rest
  int f;
  for (f = 0; f < numFrames; f++) 
    {
    capGrabFrameNoStop(this->CapWnd);
    }
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::Play()
{
  this->Initialize();
  if (!this->Initialized)
    {
    return;
    }

  if (!this->Playing)
    {
    this->Playing = 1;
    this->Modified();
    capCaptureSequenceNoFile(this->CapWnd);
    }
}
    
//----------------------------------------------------------------------------
void vtkWin32VideoSource::Stop()
{
  if (this->Playing)
    {
    this->Playing = 0;
    this->Modified();

    capCaptureStop(this->CapWnd);
    }
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::UnpackRasterLine(char *outptr, char *inptr, 
					   int start, int count)
{
  char alpha = (char)(this->Opacity*255);
  int i;

  switch (this->FrameBufferBitsPerPixel)
    {
    case 1:
      {
      int rawBits;
      inptr += start/8;
      i = start % 8;
      while (count >= 0)
	{ 
	rawBits = *inptr++;
	for (; i < 8 && --count >= 0; i++)
	  {
	  *outptr++ = -((rawBits >> i) & 0x01);
	  }
	i = 0;
	}
      }
      break;
    case 4:
      {
      int rawNibbles;
      inptr += start/2;
      i = start % 2;
      while (count >= 0)
	{ 
	rawNibbles = *inptr++;
	for (; i < 8 && --count >= 0; i += 4)
	  {
	  *outptr++ = ((rawNibbles >> i) & 0x0f) << 4;
	  }
	i = 0;
	}
      }
      break;
    case 8:
      {
      inptr += start;
      memcpy(outptr,inptr,count);
      }
      break;
    case 16:
      {
      inptr += 2*start;
      unsigned short rawWord;
      unsigned short *shptr = (unsigned short *)inptr;
      switch (this->OutputFormat)
	{
	case VTK_RGB:
	  { // unpack 16 bits to 24 bits
	  while (--count >= 0)
	    {
	    rawWord = *shptr++;
	    *outptr++ = (rawWord & 0x7c00) >> 7;
	    *outptr++ = (rawWord & 0x03e0) >> 2;
	    *outptr++ = (rawWord & 0x001f) << 3;
	    }
	  }
	  break;
	case VTK_RGBA:
	  { // unpack 16 bits to 32 bits
	  while (--count >= 0)
	    {
	    rawWord = *shptr++;
	    *outptr++ = (rawWord & 0x7c00) >> 7;
	    *outptr++ = (rawWord & 0x03e0) >> 2;
	    *outptr++ = (rawWord & 0x001f) << 3;
	    *outptr++ = alpha;
	    }
	  break;
	  }
	}
      }
    case 24:
      {
      inptr += 3*start;
      switch (this->OutputFormat)
	{
	case VTK_RGB:
	  { // must do BGR to RGB conversion
	  outptr += 3;
	  while (--count >= 0)
	    {
	    *--outptr = *inptr++;
	    *--outptr = *inptr++;
	    *--outptr = *inptr++;
	    outptr += 6;
	    }
	  }	
	  break;
	case VTK_RGBA:
	  { // must do BGR to RGBX conversion
	  outptr += 4;
	  while (--count >= 0)
	    {
	    *--outptr = alpha;
	    *--outptr = *inptr++;
	    *--outptr = *inptr++;
	    *--outptr = *inptr++;
	    outptr += 8;
	    }
	  }
	  break;
	}
      }
      break;
    case 32:
      inptr += 4*start;
      switch (this->OutputFormat)
	{
	case VTK_RGB:
	  { // must do BGRX to RGB conversion
	  outptr += 3;
	  while (--count >= 0)
	    {
	    *--outptr = *inptr++;
	    *--outptr = *inptr++;
	    *--outptr = *inptr++;
	    inptr++;
	    outptr += 6;
	    }
	  }
	  break;
	case VTK_RGBA:
	  {
	  outptr += 4;
	  while (--count >= 0)
	    {
	    *--outptr = alpha;
	    *--outptr = *inptr++;
	    *--outptr = *inptr++;
	    *--outptr = *inptr++;
	    inptr++;
	    outptr += 8;
	    }
	  }
	  break;
	}
      break;
    }
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::VideoFormatDialog()
{
  this->Initialize();
  if (!this->Initialized)
    {
    return;
    }

  if (!this->CapDriverCaps.fHasDlgVideoFormat)
    {
    MessageBox(this->ParentWnd, "The video device has no Format dialog.", "", 
	       MB_OK | MB_ICONEXCLAMATION);
    return;
    }

  capGetStatus(this->CapWnd,&this->CapStatus,sizeof(CAPSTATUS));
  if (this->CapStatus.fCapturingNow)
    {
    MessageBox(this->ParentWnd, "Can't alter video format while grabbing.","", 
	       MB_OK | MB_ICONEXCLAMATION);
    return;
    } 

  int success = capDlgVideoFormat(this->CapWnd);
  if (success)
    {
    this->FrameBufferMutex->Lock();
    this->DoVFWFormatCheck();
    this->FrameBufferMutex->Unlock();
    }
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::VideoSourceDialog()
{
  this->Initialize();
  if (!this->Initialized)
    {
    return;
    }

  if (!this->CapDriverCaps.fHasDlgVideoSource)
    {
    MessageBox(this->ParentWnd, "The video device has no Source dialog.", "", 
	       MB_OK | MB_ICONEXCLAMATION);
    return;
    }

  capGetStatus(this->CapWnd,&this->CapStatus,sizeof(CAPSTATUS));
  if (this->CapStatus.fCapturingNow)
    {
    MessageBox(this->ParentWnd, "Can't alter video source while grabbing.","", 
	       MB_OK | MB_ICONEXCLAMATION);
    return;
    } 

  int success = capDlgVideoSource(this->CapWnd);
  if (success)
    {
    this->FrameBufferMutex->Lock();
    this->DoVFWFormatCheck();
    this->FrameBufferMutex->Unlock();
    }
}

//----------------------------------------------------------------------------
// try for the specified frame size
void vtkWin32VideoSource::SetFrameSize(int x, int y, int z)
{
  if (x == this->FrameSize[0] && 
      y == this->FrameSize[1] && 
      z == this->FrameSize[2])
    {
    return;
    }

  if (x < 1 || y < 1 || z != 1) 
    {
    vtkErrorMacro(<< "SetFrameSize: Illegal frame size");
    return;
    }

  this->FrameSize[0] = x;
  this->FrameSize[1] = y;
  this->FrameSize[2] = z;
  this->Modified();

  if (this->Initialized) 
    {
    this->FrameBufferMutex->Lock();
    this->UpdateFrameBuffer();
    this->DoVFWFormatSetup();
    this->FrameBufferMutex->Unlock();
    }
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::SetFrameRate(float rate)
{
  if (rate == this->FrameRate)
    {
    return;
    }

  this->FrameRate = rate;
  this->Modified();

  if (this->Initialized)
    {
    capCaptureGetSetup(this->CapWnd,&this->CaptureParms,sizeof(CAPTUREPARMS));
    if (this->FrameRate > 0)
      {
      this->CaptureParms.dwRequestMicroSecPerFrame = 
	                    int(1000000/this->FrameRate);
      }
    else
      {
      this->CaptureParms.dwRequestMicroSecPerFrame = 0;
      }
    capCaptureSetSetup(this->CapWnd,&this->CaptureParms,sizeof(CAPTUREPARMS));
    }
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::SetOutputFormat(int format)
{
  if (format == this->OutputFormat)
    {
    return;
    }

  this->OutputFormat = format;

  // convert color format to number of scalar components
  int numComponents;

  switch (this->OutputFormat)
    {
    case VTK_RGBA:
      numComponents = 4;
      break;
    case VTK_RGB:
      numComponents = 3;
      break;
    case VTK_LUMINANCE:
      numComponents = 1;
      break;
    default:
      vtkErrorMacro(<< "SetOutputFormat: Unrecognized color format.");
      break;
    }
  this->NumberOfScalarComponents = numComponents;

  if (this->FrameBufferBitsPerPixel != numComponents*8)
    {
    this->FrameBufferMutex->Lock();
    this->FrameBufferBitsPerPixel = numComponents*8;
    if (this->Initialized)
      {
      this->UpdateFrameBuffer();
      this->DoVFWFormatSetup();
      }
    this->FrameBufferMutex->Unlock();
    }

  this->Modified();
}

//----------------------------------------------------------------------------
// check the current video format and set up the VTK video framebuffer to match
void vtkWin32VideoSource::DoVFWFormatCheck()
{
  // get the real video format
  int formatSize = capGetVideoFormatSize(this->CapWnd);
  if (formatSize > this->BitMapSize)
    {
    if (this->BitMapPtr)
      {
      delete [] ((char *)this->BitMapPtr);
      }
    this->BitMapPtr = (LPBITMAPINFO) new char[formatSize];
    this->BitMapSize = formatSize;
    }
  capGetVideoFormat(this->CapWnd,this->BitMapPtr,formatSize);
  
  int bpp = this->BitMapPtr->bmiHeader.biBitCount;
  int width = this->BitMapPtr->bmiHeader.biWidth;
  int height = this->FrameSize[1] = this->BitMapPtr->bmiHeader.biHeight;
  int compression = this->BitMapPtr->bmiHeader.biCompression;

  if (compression != BI_RGB)
    {
    vtkWarningMacro(<< "DoVFWFormatCheck: video compression on: can't grab");
    }

  if (bpp != this->FrameBufferBitsPerPixel)
    {
    switch (bpp)
      {
      case 1:
      case 4:
      case 8:
	this->OutputFormat = VTK_LUMINANCE;
	this->NumberOfScalarComponents = 1;
	break;
      case 16:
      case 32:
	if (this->OutputFormat != VTK_RGBA)
	  {
	  this->OutputFormat = VTK_RGB;
	  this->NumberOfScalarComponents = 3;
	  }
	break;
      }
    }

  if (bpp != this->FrameBufferBitsPerPixel ||
      this->FrameSize[0] != width ||
      this->FrameSize[1] != height)
    {
    this->FrameBufferBitsPerPixel = bpp;
    this->FrameSize[0] = width;
    this->FrameSize[1] = height;
    this->Modified();
    this->UpdateFrameBuffer();
    }
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::DoVFWFormatSetup()
{
  static int colorBits[3] = { 24, 32, 16 };
  static int greyBits[3] = { 8, 4, 1 };
  int i, bytesPerRow, bitCount;

  // get the real video format
  int formatSize = capGetVideoFormatSize(this->CapWnd);
  if (formatSize > this->BitMapSize)
    {
    if (this->BitMapPtr)
      {
      delete [] ((char *)this->BitMapPtr);
      }
    this->BitMapPtr = (LPBITMAPINFO) new char[formatSize];
    this->BitMapSize = formatSize;
    }
  capGetVideoFormat(this->CapWnd,this->BitMapPtr,formatSize);
  
  // set the format of the captured frames
  this->BitMapPtr->bmiHeader.biWidth = this->FrameSize[0];
  this->BitMapPtr->bmiHeader.biHeight = this->FrameSize[1];
  this->BitMapPtr->bmiHeader.biCompression = BI_RGB;
  this->BitMapPtr->bmiHeader.biClrUsed = 0;
  this->BitMapPtr->bmiHeader.biClrImportant = 0;
  
  for (i = 0; i < 4; i++)
    { // try for a 
    if (this->OutputFormat == VTK_RGBA || this->OutputFormat == VTK_RGB)
      {
      bitCount = colorBits[i];
      }
    else
      {
      bitCount = greyBits[i];
      }
    bytesPerRow = (this->FrameSize[0]*bitCount+7)/8;
    bytesPerRow += bytesPerRow % this->FrameBufferRowAlignment;
    this->BitMapPtr->bmiHeader.biBitCount = bitCount;
    this->BitMapPtr->bmiHeader.biSizeImage = bytesPerRow*this->FrameSize[1];
    if (capSetVideoFormat(this->CapWnd,this->BitMapPtr,
			  sizeof(BITMAPINFOHEADER)))
      {
      break;
      }
    }
  if (i > 4)
    {
    vtkWarningMacro(<< "DoVFWFormatSetup: invalid video format for device"\
                    << " (" << GetLastError() << ")");
    }
  this->DoVFWFormatCheck();
}











