/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32VideoSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// .SECTION Caveats
// With some capture cards, if this class is leaked and ReleaseSystemResources 
// is not called, you may have to reboot before you can capture again.
// vtkVideoSource used to keep a global list and delete the video sources
// if your program leaked, due to exit crashes that was removed.
//
// .SECTION See Also
// vtkVideoSource vtkMILVideoSource

#ifndef __vtkWin32VideoSource_h
#define __vtkWin32VideoSource_h

#include "vtkVideoSource.h"

class vtkWin32VideoSourceInternal;

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
  virtual void SetFrameSize(int dim[3]) { 
    this->SetFrameSize(dim[0], dim[1], dim[2]); };
  
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
  void LocalInternalGrab(void*);
  void OnParentWndDestroy();

protected:
  vtkWin32VideoSource();
  ~vtkWin32VideoSource();

  char WndClassName[16];
  int BitMapSize;
  int Preview;

  vtkWin32VideoSourceInternal *Internal;

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





