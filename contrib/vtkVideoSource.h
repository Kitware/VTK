/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkVideoSource - Superclass of video digitizers
// .SECTION Description
// vtkVideoSource is a superclass for video input interfaces for VTK.  
// The most important methods are Grab() (grab a single frame), 
// Play() (grab frames continuously), and Stop() (stop grabbing 
// continuously)

// .SECTION See Also
// vtkWin32VideoSource vtkMILVideoSource

#ifndef __vtkVideoSource_h
#define __vtkVideoSource_h

#include "vtkTimerLog.h"
#include "vtkMutexLock.h"
#include "vtkMultiThreader.h"
#include "vtkImageSource.h"
#include "vtkScalarsToColors.h"

class VTK_EXPORT vtkVideoSource : public vtkImageSource
{
public:
  static vtkVideoSource *New();
  vtkTypeMacro(vtkVideoSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Initialize the hardware.  This is called automatically
  // on the first Update or Grab.
  virtual void Initialize();

  // Description:
  // Release the video driver.  This is called automatically
  // when the vtkVideoSource is destroyed.
  virtual void ReleaseSystemResources();

  // Description:
  // Grab a single frame or multiple frames.
  virtual void Grab(int n);
  void Grab() { this->Grab(1); }

  // Description:
  // Go into continuous grab mode.  The video source will be
  // automatically Modified() every time a new frame arrives.
  virtual void Play();

  // Description:
  // End continuous grab mode.
  virtual void Stop();

  // Description:
  // Are we in continuous grab mode?
  vtkGetMacro(Playing,int);

  // Description:
  // Set the frame rate for 'Play' mode.  The default is 30 frames/s 
  // for most video digitizers.
  vtkSetMacro(FrameRate,float);
  vtkGetMacro(FrameRate,float);

  // Description:
  // Set the full-frame size.  This must be an allowed size for the device,
  // the device may either refuse a request for an illegal frame size or
  // automatically choose a new frame size.
  // The default is usually 320x240x1, but can be device specific.  
  // The 'depth' should always be 1 (unless you have a device that
  // can handle 3D acquisition).
  virtual void SetFrameSize(int dim[3]) { 
    this->SetFrameSize(dim[0], dim[1], dim[2]); };
  virtual void SetFrameSize(int x, int y, int z);
  vtkGetVector3Macro(FrameSize,int);

  // Description:
  // Set the output format.  This must be appropriate for device,
  // usually only VTK_LUMINANCE, VTK_RGB, and VTK_RGBA are supported.
  virtual void SetOutputFormat(int format);
  void SetOutputFormatToLuminance() { this->SetOutputFormat(VTK_LUMINANCE); };
  void SetOutputFormatToRGB() { this->SetOutputFormat(VTK_RGB); };
  void SetOutputFormatToRGBA() { this->SetOutputFormat(VTK_RGBA); };
  vtkGetMacro(OutputFormat,int);

  // Description:
  // Set size of the frame buffer, i.e. the number of frames to
  // store. 
  virtual void SetFrameBufferSize(int FrameBufferSize);
  vtkGetMacro(FrameBufferSize,int);

  // Description:
  // Set the number of frames to copy to the output on each execute.
  // The frames will be concatenated along the Z dimension, with the 
  // most recent frame first.
  // Default: 1
  vtkSetMacro(NumberOfOutputFrames,int);
  vtkGetMacro(NumberOfOutputFrames,int);

  // Description:
  // Set whether to automatically advance the buffer before each grab. 
  // Default: on
  vtkBooleanMacro(AutoAdvance,int);
  vtkSetMacro(AutoAdvance,int)
  vtkGetMacro(AutoAdvance,int);

  // Description:
  // Advance the buffer by one frame or n frames. 
  virtual void Advance(int n); 
  virtual void Advance() { this->Advance(-1); };

  // Description:
  // Rewind the buffer by one frame or n frames.
  virtual void Rewind(int n) { this->Advance(-n); };
  virtual void Rewind() { this->Advance(-1); };

  // Description:
  // Set the clip rectangle for the frames.  The video will be clipped 
  // before it is copied into the framebuffer.  Changing the ClipRegion
  // will destroy the current contents of the framebuffer.
  // The default ClipRegion is (0,VTK_INT_MAX,0,VTK_INT_MAX,0,VTK_INT_MAX).
  virtual void SetClipRegion(int r[6]) { 
    this->SetClipRegion(r[0],r[1],r[2],r[3],r[4],r[5]); };
  virtual void SetClipRegion(int x0, int x1, int y0, int y1, int z0, int z1);
  vtkGetVector6Macro(ClipRegion,int);

  // Description:
  // Get/Set the WholeExtent of the output.  This can be used to either
  // clip or pad the video frame.  This clipping/padding is done when
  // the frame is copied to the output, and does not change the contents
  // of the framebuffer.  This is useful e.g. for expanding 
  // the output size to a power of two for texture mapping.  The
  // default is (0,-1,0,-1,0,-1) which causes the entire frame to be
  // copied to the output.
  vtkSetVector6Macro(OutputWholeExtent,int);
  vtkGetVector6Macro(OutputWholeExtent,int);
  
  // Description:
  // Set/Get the pixel spacing. 
  // Default: (1.0,1.0,1.0)
  vtkSetVector3Macro(DataSpacing,float);
  vtkGetVector3Macro(DataSpacing,float);
  
  // Description:
  // Set/Get the coordinates of the lower, left corner of the frame. 
  // Default: (0.0,0.0,0.0)
  vtkSetVector3Macro(DataOrigin,float);
  vtkGetVector3Macro(DataOrigin,float);

  // Description:
  // For RGBA output only (4 scalar components), set the opacity.  This
  // will not modify the contents of the framebuffer, only subsequently
  // grabbed frames.
  vtkSetMacro(Opacity,float);
  vtkGetMacro(Opacity,float);  

  // Description:
  // Enable a video preview window if supported by driver.
  vtkSetMacro(Preview,int);
  vtkBooleanMacro(Preview,int);
  vtkGetMacro(Preview,int);

  // Description:
  // Get a time stamp in seconds (resolution of milliseconds) for
  // a video frame.   Time began on Jan 1, 1970.
  virtual double GetFrameTimeStamp(int frame);
  virtual double GetFrameTimeStamp() { return this->GetFrameTimeStamp(0); };

  // Description:
  // Set this flag to automatically do a grab on each Update.  This might
  // be less CPU-intensive than Play() for doing screen animations. 
  virtual void SetGrabOnUpdate(int yesno);
  vtkBooleanMacro(GrabOnUpdate,int);
  vtkGetMacro(GrabOnUpdate,int);

  // Description:
  // The internal function which actually does the grab.  You will
  // definitely want to override this if you develop a vtkVideoSource
  // subclass.
  virtual void InternalGrab();

  // Description:
  // This method returns the largest data that can be generated.
  void UpdateInformation();

protected:
  vtkVideoSource();
  ~vtkVideoSource();
  vtkVideoSource(const vtkVideoSource&) {};
  void operator=(const vtkVideoSource&) {};
  void ExecuteInformation();

  int Initialized;

  int FrameSize[3];
  int ClipRegion[6];
  int OutputWholeExtent[6];
  float DataSpacing[3];
  float DataOrigin[3];
  int OutputFormat;
  // set according to the OutputFormat
  int NumberOfScalarComponents;
  // The FrameOutputExtent is the WholeExtent for a single output frame.
  // It is initialized in ExecuteInformation. 
  int FrameOutputExtent[6];

  // save this information from the output so that we can see if the
  // output scalars have changed
  int LastNumberOfScalarComponents;
  int LastOutputExtent[6];

  int Playing;
  float FrameRate;

  int AutoAdvance;
  int NumberOfOutputFrames;

  float Opacity;
  int Preview;

  int GrabOnUpdate;

  // true if Execute() must apply a vertical flip to each frame
  int FlipFrames;

  // set if output needs to be cleared to be cleared before being written
  int OutputNeedsInitialization;

  // set if a frame has been grabbed in the 'GrabOnUpdate' hack
  int FrameGrabbed;

  // An example of asynchrony
  vtkMultiThreader *PlayerThreader;
  int PlayerThreadId;

  // A mutex for the frame buffer: must be applied when any of the
  // below data is modified.

  vtkMutexLock *FrameBufferMutex;

  // set according to the needs of the hardware:
  // number of bits per framebuffer pixel
  int FrameBufferBitsPerPixel;
  // byte alignment of each row in the framebuffer
  int FrameBufferRowAlignment;
  // FrameBufferExtent is the extent of frame after it has been clipped 
  // with ClipRegion.  It is initialized in CheckBuffer().
  int FrameBufferExtent[6];

  int FrameBufferSize;
  int FrameBufferIndex;
  void **FrameBuffer;
  double *FrameBufferTimeStamps;

  // Description:
  // These methods can be overridden in subclasses
  virtual void UpdateFrameBuffer();
  virtual void AdvanceFrameBuffer(int n);
  virtual void Execute(vtkImageData *data);
  void Execute() { this->vtkImageSource::Execute(); };
  // if some component conversion is required, it is done here:
  virtual void UnpackRasterLine(char *outPtr, char *rowPtr, 
				int start, int count);
};

#endif





