/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoSource.h
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
// .NAME vtkVideoSource - Superclass of video digitizers
// .SECTION Description
// vtkVideoSource is a superclass for video input interfaces for VTK.  
// The most important methods are Grab() (grab a single frame)
// and Play() (grab frames continuously).

// .SECTION See Also
// vtkWin32VideoSource

#ifndef __vtkVideoSource_h
#define __vtkVideoSource_h

#include "vtkTimerLog.h"
#include "vtkDoubleArray.h"
#include "vtkMutexLock.h"
#include "vtkMultiThreader.h"
#include "vtkImageSource.h"
#include "vtkScalarsToColors.h"

class VTK_EXPORT vtkVideoSource : public vtkImageSource
{
public:
  static vtkVideoSource *New();
  const char *GetClassName() {return "vtkVideoSource";};
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Initialize the hardware.  This is called automatically
  // on the first Update or Grab)
  virtual void Initialize();

  // Description:
  // Release the video driver.  This is called automatically
  // when the vtkVideoSource is destroyed.
  virtual void ReleaseSystemResources();

  // Description:
  // Grab a single frame or multiple frames
  virtual void Grab(int n);
  void Grab() { this->Grab(1); }

  // Description:
  // Go into continuous grab mode
  virtual void Play();

  // Description:
  // End continuous grab mode
  virtual void Stop();

  // Description:
  // Are we in continuous grab mode?
  vtkGetMacro(Playing,int);

  // Description:
  // Set the frame rate for 'Play' mode.  The default is zero, which forces
  // one synchronous grab per Update 
  vtkSetMacro(FrameRate,float);
  vtkGetMacro(FrameRate,float);

  // Description:
  // Set the full-frame size (must be an allowed size for the device)
  // Default is usually 320x240x1, but can be device specific.  
  // The 'depth' should always be 1 (unless you have a device that
  // can handle 3D acquisition).
  virtual void SetFrameSize(int dim[3]) { 
    this->SetFrameSize(dim[0], dim[1], dim[2]); };
  virtual void SetFrameSize(int x, int y, int z);
  vtkGetVector3Macro(FrameSize,int);

  // Description:
  // Set the output format (must be appropriate for device,
  // usually only VTK_LUMINANCE, VTK_RGB, VTK_RGBA) are supported.
  virtual void SetOutputFormat(int format);
  void SetOutputFormatToLuminance() { this->SetOutputFormat(VTK_LUMINANCE); };
  void SetOutputFormatToRGB() { this->SetOutputFormat(VTK_RGB); };
  void SetOutputFormatToRGBA() { this->SetOutputFormat(VTK_RGBA); };
  vtkGetMacro(OutputFormat,int);

  // Description:
  // Set size of circular buffer, i.e. the number of frames to store. 
  // Default is 1.
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
  // Advance the buffer by one frame
  virtual void Advance(int n); 
  virtual void Advance() { this->Advance(-1); };

  // Description:
  // Circulate the buffer to bring the previous image to the top.
  // If n is negative, then this circulates the buffer in the opposite 
  // direction.
  virtual void Rewind(int n) { this->Advance(-n); };
  virtual void Rewind() { this->Advance(-1); };

  // Description:
  // Set the clip rectangle for the frames.  The video will be clipped 
  // before it is copied into the framebuffer.  You will not
  // see the effect until the next Grab.  The default ClipRegion is 
  // (0,VTK_INT_MAX,0,VTK_INT_MAX,0,VTK_INT_MAX).
  virtual void SetClipRegion(int r[6]) { 
    this->SetClipRegion(r[0],r[1],r[2],r[3],r[4],r[5]); };
  virtual void SetClipRegion(int x0, int x1, int y0, int y1, int z0, int z1);
  vtkGetVector6Macro(ClipRegion,int);

  // Description:
  // Get/Set the WholeExtent of the output.  This can be used to either
  // clip or pad the video frame.  The clipping/padding is done when
  // the frame is copied to the output.  It is useful e.g. for expanding 
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
  // Set/Get the coordinates of the lower,left corner of the frame. 
  // Default: (0.0,0.0,0.0)
  vtkSetVector3Macro(DataOrigin,float);
  vtkGetVector3Macro(DataOrigin,float);

  // Description:
  // For RGBA output only (4 scalar components), set the opacity.  You will
  // not see the effect until the next Grab.
  vtkSetMacro(Opacity,float);
  vtkGetMacro(Opacity,float);  

  // Description:
  // Enable a video preview window if supported by driver
  vtkSetMacro(Preview,int);
  vtkBooleanMacro(Preview,int);
  vtkGetMacro(Preview,int);

  // Description:
  // Get a time stamp in seconds (resolution of milliseconds) for
  // a video frame.  Time began on Jan 1, 1970.
  virtual double GetFrameTimeStamp(int frame);
  virtual double GetFrameTimeStamp() { return this->GetFrameTimeStamp(0); };

  // Description:
  // Set this flag to automatically do a grab on each Update
  virtual void SetGrabOnUpdate(int yesno);
  vtkBooleanMacro(GrabOnUpdate,int);
  vtkGetMacro(GrabOnUpdate,int);

  // Description:
  // Internal function which actually does the grab.  You will
  // definitely want to override this.
  virtual void InternalGrab();

  // Description:
  // This method returns the largest data that can be generated.
  void ExecuteInformation();
  void UpdateInformation();

protected:
  vtkVideoSource();
  ~vtkVideoSource();
  vtkVideoSource(const vtkVideoSource&) {};
  void operator=(const vtkVideoSource&) {};

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





