/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32VideoSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWin32VideoSource - Video-for-Windows video digitizer
// .SECTION Description
// vtkWin32VideoSource grabs frames or streaming video from a
// Video for Windows compatible device on the Win32 platform. 

// .SECTION See Also
// vtkVideoSource vtkMILVideoSource

#ifndef __vtkWin32VideoSource_h
#define __vtkWin32VideoSource_h

#include "vtkVideoSource.h"
#include <windows.h>
#include <winuser.h>
#include <vfw.h>

class VTK_HYBRID_EXPORT vtkWin32VideoSource : public vtkVideoSource
{
public:
  static vtkWin32VideoSource *New();
  vtkTypeRevisionMacro(vtkWin32VideoSource,vtkVideoSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Standard VCR functionality: Record incoming video.
  void Record();

  // Description:
  // Standard VCR functionality: Play recorded video.
  void Play();

  // Description:
  // Standard VCR functionality: Stop recording or playing.
  void Stop();

  // Description:
  // Grab a single video frame.
  void Grab();
 
  // Description:
  // Request a particular frame size (set the third value to 1).
  void SetFrameSize(int x, int y, int z);
  
  // Description:
  // Request a particular frame rate (default 30 frames per second).
  void SetFrameRate(float rate);

  // Description:
  // Request a particular output format (default: VTK_RGB).
  void SetOutputFormat(int format);

  // Description:
  // Turn on/off the preview (overlay) window.
  void SetPreview(int p);
  vtkBooleanMacro(Preview,int);
  vtkGetMacro(Preview,int);

  // Description:
  // Bring up a modal dialog box for video format selection.
  void VideoFormatDialog();

  // Description:
  // Bring up a modal dialog box for video input selection.
  void VideoSourceDialog();

  // Description:
  // Initialize the driver (this is called automatically when the
  // first grab is done).
  void Initialize();

  // Description:
  // Free the driver (this is called automatically inside the
  // destructor).
  void ReleaseSystemResources();

  // Description:
  // For internal use only
  void InternalGrab(LPVIDEOHDR VideoHdrPtr);
  void OnParentWndDestroy();

protected:
  vtkWin32VideoSource();
  ~vtkWin32VideoSource();

  char WndClassName[16];
  HWND CapWnd;
  HWND ParentWnd;
  CAPSTATUS CapStatus;
  CAPDRIVERCAPS CapDriverCaps;
  CAPTUREPARMS CaptureParms;
  LPBITMAPINFO BitMapPtr;
  int BitMapSize;

  int Preview;

  void CheckBuffer();
  void UnpackRasterLine(char *outptr, char *inptr, 
                        int start, int count);

  void DoVFWFormatSetup();
  void DoVFWFormatCheck();

private:
  vtkWin32VideoSource(const vtkWin32VideoSource&);  // Not implemented.
  void operator=(const vtkWin32VideoSource&);  // Not implemented.
};

#endif





