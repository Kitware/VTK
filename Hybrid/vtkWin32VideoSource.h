/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32VideoSource.h
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
  vtkTypeMacro(vtkWin32VideoSource,vtkVideoSource);
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

protected:
  vtkWin32VideoSource();
  ~vtkWin32VideoSource();
  vtkWin32VideoSource(const vtkWin32VideoSource&);
  void operator=(const vtkWin32VideoSource&);

  char WndClassName[16];
  HWND CapWnd;
  HWND ParentWnd;
  CAPSTATUS CapStatus;
  CAPDRIVERCAPS CapDriverCaps;
  CAPTUREPARMS CaptureParms;
  LPBITMAPINFO BitMapPtr;
  int BitMapSize;

  int Preview;

  int FatalVFWError;

  void CheckBuffer();
  void UnpackRasterLine(char *outptr, char *inptr, 
			int start, int count);

  void DoVFWFormatSetup();
  void DoVFWFormatCheck();
};

#endif





