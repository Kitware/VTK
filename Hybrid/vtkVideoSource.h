/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoSource.h
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
// .NAME vtkVideoSource - Superclass of video input devices for VTK
// .SECTION Description
// vtkVideoSource is a superclass for video input interfaces for VTK.
// The goal is to provide an interface which is very similar to the
// interface of a VCR, where the 'tape' is an internal frame buffer
// capable of holding a preset number of video frames.  Specialized
// versions of this class record input from various video input sources.
// This base class records input from a noise source.

// .SECTION See Also
// vtkWin32VideoSource vtkMILVideoSource

#ifndef __vtkVideoSource_h
#define __vtkVideoSource_h

#include "vtkTimerLog.h"
#include "vtkMutexLock.h"
#include "vtkMultiThreader.h"
#include "vtkImageSource.h"
#include "vtkScalarsToColors.h"

class VTK_HYBRID_EXPORT vtkVideoSource : public vtkImageSource
{
public:
  static vtkVideoSource *New();
  vtkTypeMacro(vtkVideoSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Record incoming video at the specified FrameRate.  The recording
  // continues indefinitely until Stop() is called. 
  virtual void Record();

  // Description:
  // Play through the 'tape' sequentially at the specified frame rate.
  // If you have just finished Recoding, you should call Rewind() first.
  virtual void Play();

  // Description:
  // Stop recording or playing.
  virtual void Stop();

  // Description:
  // Rewind to the frame that has the earliest timestamp.  Subsequent grab 
  // and record operations will start on the following frame, therefore
  // if you want to re-record over the this frame you must call Seek(-1)
  // before calling Grab() or Record().
  virtual void Rewind();

  // Description:
  // FastForward to the last frame that was recorded (i.e. to the frame
  // that has the most recent timestamp).
  virtual void FastForward();

  // Description:
  // Seek forwards or backwards by the specified number of frames
  // (positive is forward, negative is backward).
  virtual void Seek(int n); 

  // Description:
  // Grab a single video frame.
  virtual void Grab();

  // Description:
  // Are we in record mode? (record mode and play mode are mutually
  // exclusive).
  vtkGetMacro(Recording,int);

  // Description:
  // Are we in play mode? (record mode and play mode are mutually
  // exclusive).
  vtkGetMacro(Playing,int);

  // Description:
  // Set the full-frame size.  This must be an allowed size for the device,
  // the device may either refuse a request for an illegal frame size or
  // automatically choose a new frame size.
  // The default is usually 320x240x1, but can be device specific.  
  // The 'depth' should always be 1 (unless you have a device that
  // can handle 3D acquisition).
  virtual void SetFrameSize(int x, int y, int z);
  virtual void SetFrameSize(int dim[3]) { 
    this->SetFrameSize(dim[0], dim[1], dim[2]); };
  vtkGetVector3Macro(FrameSize,int);

  // Description:
  // Request a particular frame rate (default 30 frames per second).
  virtual void SetFrameRate(float rate);
  vtkGetMacro(FrameRate,float);

  // Description:
  // Set the output format.  This must be appropriate for device,
  // usually only VTK_LUMINANCE, VTK_RGB, and VTK_RGBA are supported.
  virtual void SetOutputFormat(int format);
  void SetOutputFormatToLuminance() { this->SetOutputFormat(VTK_LUMINANCE); };
  void SetOutputFormatToRGB() { this->SetOutputFormat(VTK_RGB); };
  void SetOutputFormatToRGBA() { this->SetOutputFormat(VTK_RGBA); };
  vtkGetMacro(OutputFormat,int);

  // Description:
  // Set size of the frame buffer, i.e. the number of frames that
  // the 'tape' can store.
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
  // will not modify the existing contents of the framebuffer, only
  // subsequently grabbed frames.
  vtkSetMacro(Opacity,float);
  vtkGetMacro(Opacity,float);  

  // Description:
  // Get the number of frames captured since the beginning of the 
  // last Record session.
  vtkGetMacro(FrameCount, int);

  // Description:
  // Get a time stamp in seconds (resolution of milliseconds) for
  // a video frame.   Time began on Jan 1, 1970.  You can specify
  // a number (negative or positive) to specify the position of the
  // video frame relative to the current frame.
  virtual double GetFrameTimeStamp(int frame);
  virtual double GetFrameTimeStamp() { return this->GetFrameTimeStamp(0); };

  // Description:
  // Initialize the hardware.  This is called automatically
  // on the first Update or Grab.
  virtual void Initialize();

  // Description:
  // Release the video driver.  This is called automatically
  // when the vtkVideoSource is destroyed.
  virtual void ReleaseSystemResources();

  // Description:
  // The internal function which actually does the grab.  You will
  // definitely want to override this if you develop a vtkVideoSource
  // subclass. 
  virtual void InternalGrab();

  // Description:
  // And internal variable which marks the beginning of a Record session.
  // These methods are for internal use only.
  void SetStartTimeStamp(double t) { this->StartTimeStamp = t; };
  double GetStartTimeStamp() { return this->StartTimeStamp; };

protected:
  vtkVideoSource();
  ~vtkVideoSource();
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

  int Recording;
  int Playing;
  float FrameRate;
  int FrameCount;
  double StartTimeStamp;

  int AutoAdvance;
  int NumberOfOutputFrames;

  float Opacity;

  // true if Execute() must apply a vertical flip to each frame
  int FlipFrames;

  // set if output needs to be cleared to be cleared before being written
  int OutputNeedsInitialization;

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
  virtual void ExecuteData(vtkDataObject *data);
  // if some component conversion is required, it is done here:
  virtual void UnpackRasterLine(char *outPtr, char *rowPtr, 
				int start, int count);
private:
  vtkVideoSource(const vtkVideoSource&);  // Not implemented.
  void operator=(const vtkVideoSource&);  // Not implemented.
};

#endif





