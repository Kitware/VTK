/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32VideoSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "vtkWin32VideoSource.h"
#include "vtkObjectFactory.h"

// VFW compressed formats are listed at http://www.webartz.com/fourcc/
#define VTK_BI_UYVY 0x59565955

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
  this->OutputFormat = VTK_RGB;
  this->NumberOfScalarComponents = 3;
  this->FrameBufferBitsPerPixel = 24;
  this->FlipFrames = 0;
  this->FrameBufferRowAlignment = 4;

  this->CapWnd = NULL;
  this->ParentWnd = NULL;
  this->BitMapSize = 0;
  this->BitMapPtr = NULL;
  this->WndClassName[0] = '\0';

  this->Preview = 0;

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
  vtkVideoSource::PrintSelf(os,indent);

  os << indent << "Preview: " << (this->Preview ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
// This is empty for now because we aren't displaying the capture window
LONG FAR PASCAL
vtkWin32VideoSourceWinProc(HWND hwnd, UINT message, 
			   WPARAM wParam, LPARAM lParam)
{
  vtkWin32VideoSource *self = (vtkWin32VideoSource *)\
    (GetWindowLong(hwnd,GWL_USERDATA));

  switch(message) {

  case WM_MOVE:
    //cerr << "WM_MOVE\n";
    break;

  case WM_SIZE:
    //cerr << "WM_SIZE\n";
    break;
    
  case WM_DESTROY:
    //cerr << "WM_DESTROY\n";    
    break;

  case WM_CLOSE:
    //cerr << "WM_CLOSE\n";
    self->PreviewOff();
    break;

  default:
    return(DefWindowProc(hwnd, message, wParam, lParam));
  }
  return 0;
}

//----------------------------------------------------------------------------
LRESULT PASCAL vtkWin32VideoSourceCapControlProc(HWND hwndC, int nState)
{
  vtkWin32VideoSource *self = (vtkWin32VideoSource *)(capGetUserData(hwndC));

  if (nState == CONTROLCALLBACK_PREROLL)
    {
    //cerr << "controlcallback preroll\n";
    self->SetStartTimeStamp(vtkTimerLog::GetCurrentTime());      
    }
  else if (nState == CONTROLCALLBACK_CAPTURING)
    {
    //cerr << "controlcallback capturing\n";
    }

  return TRUE;
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

  if (nID == IDS_CAP_BEGIN)
    {
    //cerr << "start of capture\n";
    }

  if (nID == IDS_CAP_END)
    {
    //cerr << "end of capture\n";
    }

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
  wc.cbClsExtra = sizeof(void *);
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

  DWORD style = WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|
                WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

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
		this->FrameSize[0]+2*GetSystemMetrics(SM_CXFIXEDFRAME), 
		this->FrameSize[1]+2*GetSystemMetrics(SM_CYFIXEDFRAME)
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

  // set the user data to 'this'
  SetWindowLong(this->ParentWnd,GWL_USERDATA,(LONG)this);

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
    
  // install the callback to precisely time beginning of grab
  if (!capSetCallbackOnCapControl(this->CapWnd,
				  &vtkWin32VideoSourceCapControlProc))
    {
    vtkErrorMacro(<< "Initialize: couldn't set control callback"\
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
  // install the callback to get info on start/end of streaming
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
  
  capOverlay(this->CapWnd,TRUE);

  // update framebuffer again to reflect any changes which
  // might have occurred
  this->UpdateFrameBuffer();

  this->Initialized = 1;
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::SetPreview(int p)
{
  if (this->Preview == p)
    {
    return;
    }

  this->Preview = p;
  this->Modified();
  
  if (this->CapWnd == NULL || this->ParentWnd == NULL)
    {
    return;
    }

  if (p)
    {
    ShowWindow(this->ParentWnd,SW_SHOWNORMAL);
    }
  else
    {
    ShowWindow(this->ParentWnd,SW_HIDE);
    }
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::ReleaseSystemResources()
{
  if (this->Playing || this->Recording)
    {
    this->Stop();
    }  

  if (this->CapWnd)
    {
    //MessageBox(this->ParentWnd, "capDriverDisconnect(this->CapWnd)", "", MB_OK | MB_ICONEXCLAMATION);
    capDriverDisconnect(this->CapWnd);
    //MessageBox(this->ParentWnd, "DestroyWindow(this->CapWnd)", "", MB_OK | MB_ICONEXCLAMATION);
    DestroyWindow(this->CapWnd);
    this->CapWnd = NULL;
    }
  if (this->ParentWnd)
    {
    //MessageBox(this->ParentWnd, "DestroyWindow(this->ParentWnd)", "", MB_OK | MB_ICONEXCLAMATION);
    DestroyWindow(this->ParentWnd);
    this->ParentWnd = NULL;
    }
  if (this->WndClassName[0] != '\0')
    {
    UnregisterClass(this->WndClassName,GetModuleHandle(NULL));
    this->WndClassName[0] = '\0';
    }

  this->FatalVFWError = 1;
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
  
  this->FrameCount++;
  this->FrameBufferTimeStamps[index] = this->StartTimeStamp + \
                                       0.001 * lpVHdr->dwTimeCaptured;

  unsigned char *ptr = (unsigned char *)
    ((reinterpret_cast<vtkUnsignedCharArray*>(this->FrameBuffer[index])) \
     ->GetPointer(0));

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
    case VTK_BI_UYVY:
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
void vtkWin32VideoSource::Grab()
{
  if (this->Recording)
    {
    return;
    }

  // ensure that the frame buffer is properly initialized
  this->Initialize();
  if (!this->Initialized)
    {
    return;
    }

  // just do the grab, the callback does the rest
  this->SetStartTimeStamp(vtkTimerLog::GetCurrentTime());
  capGrabFrameNoStop(this->CapWnd);
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::Record()
{
  this->Initialize();
  if (!this->Initialized)
    {
    return;
    }

  if (this->Playing)
    {
    this->Stop();
    }

  if (!this->Recording)
    {
    this->Recording = 1;
    this->Modified();
    capCaptureSequenceNoFile(this->CapWnd);
    }
}
    
//----------------------------------------------------------------------------
void vtkWin32VideoSource::Play()
{
  this->vtkVideoSource::Play();
}
    
//----------------------------------------------------------------------------
void vtkWin32VideoSource::Stop()
{
  if (this->Recording)
    {
    this->Recording = 0;
    this->Modified();

    capCaptureStop(this->CapWnd);
    }
  else if (this->Playing)
    {
    this->vtkVideoSource::Stop();
    }
}

//----------------------------------------------------------------------------
// codecs

static inline void vtkYUVToRGB(unsigned char *yuv, unsigned char *rgb)
{ 
  /* // floating point 
  int Y = yuv[0] - 16;
  int U = yuv[1] - 128;
  int V = yuv[2] - 128;

  int R = 1.164*Y + 1.596*V           + 0.5;
  int G = 1.164*Y - 0.813*V - 0.391*U + 0.5;
  int B = 1.164*Y           + 2.018*U + 0.5;
  */

  // integer math
  int Y = (yuv[0] - 16)*76284;
  int U = yuv[1] - 128;
  int V = yuv[2] - 128;

  int R = Y + 104595*V           ;
  int G = Y -  53281*V -  25625*U;
  int B = Y            + 132252*U;

  // round
  R += 32768;
  G += 32768;
  B += 32768;

  // shift
  R >>= 16;
  G >>= 16;
  B >>= 16;

  // clamp
  if (R < 0) { R = 0; }
  if (G < 0) { G = 0; }
  if (B < 0) { B = 0; }

  if (R > 255) { R = 255; };
  if (G > 255) { G = 255; };
  if (B > 255) { B = 255; };

  // output
  rgb[0] = R;
  rgb[1] = G;
  rgb[2] = B;
}

//----------------------------------------------------------------------------
void vtkWin32VideoSource::UnpackRasterLine(char *outptr, char *inptr, 
					   int start, int count)
{
  char alpha = (char)(this->Opacity*255);
  int compression = this->BitMapPtr->bmiHeader.biCompression;
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
      if (compression == VTK_BI_UYVY)
	{
	switch (this->OutputFormat)
	  {
	  case VTK_LUMINANCE:
	    { // unpack UY half-megapixel to one Y pixel
	    while (--count >= 0)
	      {
	      inptr++;
	      *outptr++ = *inptr++;
	      }	    
	    }
	  case VTK_RGB:
	  case VTK_RGBA:
	    { // unpack UYVY megapixel to two RGB or RGBA pixels
	    unsigned char YUV[3];
	    int finish = start + count;
	    int odd = (start % 2 == 1);
	    if (count > 0) { YUV[1+odd] = inptr[0]; }
	    if (count > 1) { YUV[0]     = inptr[1]; }
	    if (count > 2) { YUV[2-odd] = inptr[2]; }
	    while (--count >= 0)
	      {
	      YUV[1+odd] = *inptr++;
	      YUV[0] = *inptr++;
	      odd = !odd;
	      vtkYUVToRGB(YUV,(unsigned char *)outptr);
	      outptr += 3;
	      if (this->OutputFormat == VTK_RGB) 
		{ 
		continue; 
		}
	      *outptr++ = alpha;
	      }
	    }
	  }
	}
      else
	{
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

  if (compression == VTK_BI_UYVY)
    {
    this->FlipFrames = 1;
    }
  else if (compression == BI_RGB)
    {
    this->FlipFrames = 0;
    }
  else
    {
    char fourcchex[16], fourcc[8];
    sprintf(fourcchex,"0x%08x",compression);
    for (int i = 0; i < 4; i++)
      {
      fourcc[i] = (compression >> (8*i)) & 0xff;
      if (!isprint(fourcc[i]))
	{
	fourcc[i] = '?';
	}
      }
    fourcc[4] = '\0';
    vtkWarningMacro(<< "DoVFWFormatCheck: video compression mode " <<
                    fourcchex << " \"" << fourcc << "\": can't grab");
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
	if (compression != VTK_BI_UYVY)
	  {
	  this->OutputFormat = VTK_RGB;
	  this->NumberOfScalarComponents = 3;
	  }
	break;
      case 24:
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











