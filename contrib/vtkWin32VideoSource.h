/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32VideoSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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

class VTK_EXPORT vtkWin32VideoSource : public vtkVideoSource
{
public:
  static vtkWin32VideoSource *New();
  vtkTypeMacro(vtkWin32VideoSource,vtkVideoSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // See vtkVideoSource
  void Initialize();
  void ReleaseSystemResources();

  void Grab(int n);
  void Grab() { this->Grab(1); };
  void Play();
  void Stop();

  void SetFrameSize(int x, int y, int z);
  
  void SetFrameRate(float rate);

  void SetOutputFormat(int format);

  // Description:
  // bring up a modal dialog box for video format selection
  void VideoFormatDialog();

  // Description:
  // bring up a modal dialog box for video input selection 
  void VideoSourceDialog();

  // Description:
  // For internal use only
  void InternalGrab(LPVIDEOHDR VideoHdrPtr);

protected:
  vtkWin32VideoSource();
  ~vtkWin32VideoSource();
  vtkWin32VideoSource(const vtkWin32VideoSource&) {};
  void operator=(const vtkWin32VideoSource&) {};

  char WndClassName[16];
  HWND CapWnd;
  HWND ParentWnd;
  CAPSTATUS CapStatus;
  CAPDRIVERCAPS CapDriverCaps;
  CAPTUREPARMS CaptureParms;
  LPBITMAPINFO BitMapPtr;
  int BitMapSize;

  int FatalVFWError;

  void CheckBuffer();
  void UnpackRasterLine(char *outptr, char *inptr, 
			int start, int count);

  void DoVFWFormatSetup();
  void DoVFWFormatCheck();
};

#endif





