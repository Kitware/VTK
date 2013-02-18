/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVideoSource.h"

#include "vtkCriticalSection.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMultiThreader.h"
#include "vtkMutexLock.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtksys/SystemTools.hxx"

//---------------------------------------------------------------
// Important FrameBufferMutex rules:
//
// The frame grabs are generally done asynchronously, and it is necessary
// to ensure that when the frame buffer is valid when it is being written
// to or read from
//
// The following information can only be changed within a mutex lock,
// and the lock must not be released until the frame buffer agrees with the
// information.
//
// FrameBuffer
// FrameBufferTimeStamps
// FrameBufferSize
// FrameBufferIndex
// FrameBufferExtent
// FrameBufferBitsPerPixel
// FrameBufferRowAlignment
//
// After one of the above has been changed, and before the mutex is released,
// the following must be called to update the frame buffer:
//
// UpdateFrameBuffer()
//
// Likewise, the following function must only be called from within a
// mutex lock because it modifies FrameBufferIndex:
//
// AdvanceFrameBuffer()
//
// Any methods which might be called asynchronously must lock the
// mutex before reading the above information, and you must be very
// careful when accessing any information except for the above.
// These methods include the following:
//
// InternalGrab()
//
// Finally, when Execute() is reading from the FrameBuffer it must do
// so from within a mutex lock.  Otherwise tearing artifacts might result.

vtkStandardNewMacro(vtkVideoSource);

#if defined(_MSC_VER)
#pragma warning ( disable : 4311 )
#pragma warning ( disable : 4312 )
#endif

//----------------------------------------------------------------------------
vtkVideoSource::vtkVideoSource()
{
  int i;

  this->Initialized = 0;

  this->AutoAdvance = 1;

  this->FrameSize[0] = 320;
  this->FrameSize[1] = 240;
  this->FrameSize[2] = 1;

  for (i = 0; i < 6; i++)
    {
    this->FrameBufferExtent[i] = 0;
    }

  this->Playing = 0;
  this->Recording = 0;

  this->FrameRate = 30;

  this->FrameCount = 0;
  this->FrameIndex = -1;

  this->StartTimeStamp = 0;
  this->FrameTimeStamp = 0;

  this->OutputNeedsInitialization = 1;

  this->OutputFormat = VTK_LUMINANCE;
  this->NumberOfScalarComponents = 1;

  this->NumberOfOutputFrames = 1;

  this->Opacity = 1.0;

  for (i = 0; i < 3; i++)
    {
    this->ClipRegion[i*2] = 0;
    this->ClipRegion[i*2+1] = VTK_INT_MAX;
    this->OutputWholeExtent[i*2] = 0;
    this->OutputWholeExtent[i*2+1] = -1;
    this->DataSpacing[i] = 1.0;
    this->DataOrigin[i] = 0.0;
    }

  for (i = 0; i < 6; i++)
    {
    this->LastOutputExtent[i] = 0;
    }
  this->LastNumberOfScalarComponents = 0;

  this->FlipFrames = 0;

  this->PlayerThreader = vtkMultiThreader::New();
  this->PlayerThreadId = -1;

  this->FrameBufferMutex = vtkCriticalSection::New();

  this->FrameBufferSize = 0;
  this->FrameBuffer = NULL;
  this->FrameBufferTimeStamps = NULL;
  this->FrameBufferIndex = 0;
  this->SetFrameBufferSize(1);

  this->FrameBufferBitsPerPixel = 8;
  this->FrameBufferRowAlignment = 1;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkVideoSource::~vtkVideoSource()
{
  // we certainly don't want to access a virtual
  // function after the subclass has destructed!!
  this->vtkVideoSource::ReleaseSystemResources();

  this->SetFrameBufferSize(0);
  this->FrameBufferMutex->Delete();
  this->PlayerThreader->Delete();
}

//----------------------------------------------------------------------------
void vtkVideoSource::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;

  this->Superclass::PrintSelf(os,indent);

  os << indent << "FrameSize: (" << this->FrameSize[0] << ", "
     << this->FrameSize[1] << ", " << this->FrameSize[2] << ")\n";

  os << indent << "ClipRegion: (" << this->ClipRegion[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->ClipRegion[idx];
    }
  os << ")\n";

  os << indent << "DataSpacing: (" << this->DataSpacing[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DataSpacing[idx];
    }
  os << ")\n";

  os << indent << "DataOrigin: (" << this->DataOrigin[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DataOrigin[idx];
    }
  os << ")\n";

  os << indent << "OutputFormat: " <<
    (this->OutputFormat == VTK_RGBA ? "RGBA" :
     (this->OutputFormat == VTK_RGB ? "RGB" :
      (this->OutputFormat == VTK_LUMINANCE_ALPHA ? "LuminanceAlpha" :
       (this->OutputFormat == VTK_LUMINANCE ? "Luminance" : "Unknown"))))
     << "\n";

  os << indent << "OutputWholeExtent: (" << this->OutputWholeExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->OutputWholeExtent[idx];
    }
  os << ")\n";

  os << indent << "FrameRate: " << this->FrameRate << "\n";

  os << indent << "FrameCount: " << this->FrameCount << "\n";

  os << indent << "FrameIndex: " << this->FrameIndex << "\n";

  os << indent << "Recording: " << (this->Recording ? "On\n" : "Off\n");

  os << indent << "Playing: " << (this->Playing ? "On\n" : "Off\n");

  os << indent << "FrameBufferSize: " << this->FrameBufferSize << "\n";

  os << indent << "NumberOfOutputFrames: " << this->NumberOfOutputFrames << "\n";
  os << indent << "AutoAdvance: " << (this->AutoAdvance ? "On\n" : "Off\n");

  os << indent << "Opacity: " << this->Opacity << "\n";

  os << indent << "FlipFrames: " << this->FlipFrames << "\n";

  os << indent << "FrameBufferBitsPerPixel: " << this->FrameBufferBitsPerPixel << "\n";

  os << indent << "FrameBufferRowAlignment: " << this->FrameBufferRowAlignment << "\n";
}

//----------------------------------------------------------------------------
// Update the FrameBuffers according to any changes in the FrameBuffer*
// information.
// This function should always be called from within a FrameBufferMutex lock
// and should never be called asynchronously.
// It sets up the FrameBufferExtent
void vtkVideoSource::UpdateFrameBuffer()
{
  int i, oldExt;
  int ext[3];
  vtkDataArray *buffer;

  // clip the ClipRegion with the FrameSize
  for (i = 0; i < 3; i++)
    {
    oldExt = this->FrameBufferExtent[2*i+1] - this->FrameBufferExtent[2*i] + 1;
    this->FrameBufferExtent[2*i] = ((this->ClipRegion[2*i] > 0)
                             ? this->ClipRegion[2*i] : 0);
    this->FrameBufferExtent[2*i+1] = ((this->ClipRegion[2*i+1] <
                                       this->FrameSize[i]-1)
                             ? this->ClipRegion[2*i+1] : this->FrameSize[i]-1);

    ext[i] = this->FrameBufferExtent[2*i+1] - this->FrameBufferExtent[2*i] + 1;
    if (ext[i] < 0)
      {
      this->FrameBufferExtent[2*i] = 0;
      this->FrameBufferExtent[2*i+1] = -1;
      ext[i] = 0;
      }

    if (oldExt > ext[i])
      { // dimensions of framebuffer changed
      this->OutputNeedsInitialization = 1;
      }
    }

  // total number of bytes required for the framebuffer
  int bytesPerRow = (ext[0]*this->FrameBufferBitsPerPixel+7)/8;
  bytesPerRow = ((bytesPerRow + this->FrameBufferRowAlignment - 1) /
                 this->FrameBufferRowAlignment)*this->FrameBufferRowAlignment;
  int totalSize = bytesPerRow * ext[1] * ext[2];

  i = this->FrameBufferSize;

  while (--i >= 0)
    {
    buffer = reinterpret_cast<vtkDataArray *>(this->FrameBuffer[i]);
    if (buffer->GetDataType() != VTK_UNSIGNED_CHAR ||
        buffer->GetNumberOfComponents() != 1 ||
        buffer->GetNumberOfTuples() != totalSize)
      {
      buffer->Delete();
      buffer = vtkUnsignedCharArray::New();
      this->FrameBuffer[i] = buffer;
      buffer->SetNumberOfComponents(1);
      buffer->SetNumberOfTuples(totalSize);
      }
    }
}

//----------------------------------------------------------------------------
// Initialize() should be overridden to initialize the hardware frame grabber
void vtkVideoSource::Initialize()
{
  if (this->Initialized)
    {
    return;
    }
  this->Initialized = 1;

  this->UpdateFrameBuffer();
}

//----------------------------------------------------------------------------
// ReleaseSystemResources() should be overridden to release the hardware
void vtkVideoSource::ReleaseSystemResources()
{
  if (this->Playing || this->Recording)
    {
    this->Stop();
    }

  this->Initialized = 0;
}

//----------------------------------------------------------------------------
void vtkVideoSource::SetFrameSize(int x, int y, int z)
{
  if (x == this->FrameSize[0] &&
      y == this->FrameSize[1] &&
      z == this->FrameSize[2])
    {
    return;
    }

  if (x < 1 || y < 1 || z < 1)
    {
    vtkErrorMacro(<< "SetFrameSize: Illegal frame size");
    return;
    }

  if (this->Initialized)
    {
    this->FrameBufferMutex->Lock();
    this->FrameSize[0] = x;
    this->FrameSize[1] = y;
    this->FrameSize[2] = z;
    this->UpdateFrameBuffer();
    this->FrameBufferMutex->Unlock();
    }
  else
    {
    this->FrameSize[0] = x;
    this->FrameSize[1] = y;
    this->FrameSize[2] = z;
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkVideoSource::SetFrameRate(float rate)
{
  if (this->FrameRate == rate)
    {
    return;
    }

  this->FrameRate = rate;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkVideoSource::SetClipRegion(int x0, int x1, int y0, int y1,
                                   int z0, int z1)
{
  if (this->ClipRegion[0] != x0 || this->ClipRegion[1] != x1 ||
      this->ClipRegion[2] != y0 || this->ClipRegion[3] != y1 ||
      this->ClipRegion[4] != z0 || this->ClipRegion[5] != z1)
    {
    this->Modified();
    if (this->Initialized)
      { // modify the FrameBufferExtent
      this->FrameBufferMutex->Lock();
      this->ClipRegion[0] = x0; this->ClipRegion[1] = x1;
      this->ClipRegion[2] = y0; this->ClipRegion[3] = y1;
      this->ClipRegion[4] = z0; this->ClipRegion[5] = z1;
      this->UpdateFrameBuffer();
      this->FrameBufferMutex->Unlock();
      }
    else
      {
      this->ClipRegion[0] = x0; this->ClipRegion[1] = x1;
      this->ClipRegion[2] = y0; this->ClipRegion[3] = y1;
      this->ClipRegion[4] = z0; this->ClipRegion[5] = z1;
      }
    }
}

//----------------------------------------------------------------------------
// Copy pseudo-random noise into the frames.  This function may be called
// asynchronously.
void vtkVideoSource::InternalGrab()
{
  int i,index;
  static int randsave = 0;
  int randNum;
  unsigned char *ptr;
  int *lptr;

  // get a thread lock on the frame buffer
  this->FrameBufferMutex->Lock();

  if (this->AutoAdvance)
    {
    this->AdvanceFrameBuffer(1);
    if (this->FrameIndex + 1 < this->FrameBufferSize)
      {
      this->FrameIndex++;
      }
    }

  index = this->FrameBufferIndex % this->FrameBufferSize;
  while (index < 0)
    {
    index += this->FrameBufferSize;
    }

  int bytesPerRow = ((this->FrameBufferExtent[1]-this->FrameBufferExtent[0]+1)*
                     this->FrameBufferBitsPerPixel + 7)/8;
  bytesPerRow = ((bytesPerRow + this->FrameBufferRowAlignment - 1) /
                 this->FrameBufferRowAlignment)*this->FrameBufferRowAlignment;
  int totalSize = bytesPerRow *
                   (this->FrameBufferExtent[3]-this->FrameBufferExtent[2]+1) *
                   (this->FrameBufferExtent[5]-this->FrameBufferExtent[4]+1);

  randNum = randsave;

  // copy 'noise' into the frame buffer
  ptr = reinterpret_cast<vtkUnsignedCharArray *>(this->FrameBuffer[index])->GetPointer(0);

  // Somebody should check this:
  lptr = (int *)(((((intptr_t)ptr) + 3)/4)*4);
  i = totalSize/4;

  while (--i >= 0)
    {
    randNum = 1664525*randNum + 1013904223;
    *lptr++ = randNum;
    }
  unsigned char *ptr1 = ptr + 4;
  i = (totalSize-4)/16;
  while (--i >= 0)
    {
    randNum = 1664525*randNum + 1013904223;
    *ptr1 = randNum;
    ptr1 += 16;
    }
  randsave = randNum;

  this->FrameBufferTimeStamps[index] = vtkTimerLog::GetUniversalTime();

  if (this->FrameCount++ == 0)
    {
    this->StartTimeStamp = this->FrameBufferTimeStamps[index];
    }

  this->Modified();

  this->FrameBufferMutex->Unlock();
}

//----------------------------------------------------------------------------
// Sleep until the specified absolute time has arrived.
// You must pass a handle to the current thread.
// If '0' is returned, then the thread was aborted before or during the wait.
static int vtkThreadSleep(vtkMultiThreader::ThreadInfo *data, double time)
{
  // loop either until the time has arrived or until the thread is ended
  for (int i = 0;; i++)
    {
    double remaining = time - vtkTimerLog::GetUniversalTime();

    // check to see if we have reached the specified time
    if (remaining <= 0.0)
      {
      if (i == 0)
        {
        vtkGenericWarningMacro("Dropped a video frame.");
        }
      return 1;
      }
    // check the ActiveFlag at least every 0.1 seconds
    if (remaining > 0.1)
      {
      remaining = 0.1;
      }

    // check to see if we are being told to quit
    data->ActiveFlagLock->Lock();
    int activeFlag = *(data->ActiveFlag);
    data->ActiveFlagLock->Unlock();

    if (activeFlag == 0)
      {
      break;
      }

    vtksys::SystemTools::Delay(static_cast<unsigned int>(remaining * 1000.0));
    }

  return 0;
}

//----------------------------------------------------------------------------
// this function runs in an alternate thread to asyncronously grab frames
static void *vtkVideoSourceRecordThread(vtkMultiThreader::ThreadInfo *data)
{
  vtkVideoSource *self = (vtkVideoSource *)(data->UserData);

  double startTime = vtkTimerLog::GetUniversalTime();
  double rate = self->GetFrameRate();
  int frame = 0;

  do
    {
    self->InternalGrab();
    frame++;
    }
  while (vtkThreadSleep(data, startTime + frame/rate));

  return NULL;
}

//----------------------------------------------------------------------------
// Set the source to grab frames continuously.
// You should override this as appropriate for your device.
void vtkVideoSource::Record()
{
  if (this->Playing)
    {
    this->Stop();
    }

  if (!this->Recording)
    {
    this->Initialize();

    this->Recording = 1;
    this->FrameCount = 0;
    this->Modified();
    this->PlayerThreadId =
      this->PlayerThreader->SpawnThread((vtkThreadFunctionType)\
                                &vtkVideoSourceRecordThread,this);
    }
}

//----------------------------------------------------------------------------
// this function runs in an alternate thread to 'play the tape' at the
// specified frame rate.
static void *vtkVideoSourcePlayThread(vtkMultiThreader::ThreadInfo *data)
{
  vtkVideoSource *self = (vtkVideoSource *)(data->UserData);

  double startTime = vtkTimerLog::GetUniversalTime();
  double rate = self->GetFrameRate();
  int frame = 0;

  do
    {
    self->Seek(1);
    frame++;
    }
  while (vtkThreadSleep(data, startTime + frame/rate));

  return NULL;
}

//----------------------------------------------------------------------------
// Set the source to play back recorded frames.
// You should override this as appropriate for your device.
void vtkVideoSource::Play()
{
  if (this->Recording)
    {
    this->Stop();
    }

  if (!this->Playing)
    {
    this->Initialize();

    this->Playing = 1;
    this->Modified();
    this->PlayerThreadId =
      this->PlayerThreader->SpawnThread((vtkThreadFunctionType)\
                                        &vtkVideoSourcePlayThread,this);
    }
}

//----------------------------------------------------------------------------
// Stop continuous grabbing or playback.  You will have to override this
// if your class overrides Play() and Record()
void vtkVideoSource::Stop()
{
  if (this->Playing || this->Recording)
    {
    this->PlayerThreader->TerminateThread(this->PlayerThreadId);
    this->PlayerThreadId = -1;
    this->Playing = 0;
    this->Recording = 0;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Rewind back to the frame with the earliest timestamp.
void vtkVideoSource::Rewind()
{
  this->FrameBufferMutex->Lock();

  double *stamp = this->FrameBufferTimeStamps;
  double lowest = 0;
  int i, j;

  if (this->FrameBufferSize)
    {
    lowest = stamp[this->FrameBufferIndex];
    }
  for (i = 0; i < this->FrameBufferSize; i++)
    {
    j = (this->FrameBufferIndex + i + 1) % this->FrameBufferSize;
    if (stamp[j] != 0.0 && stamp[j] <= lowest)
      {
      lowest = stamp[j];
      }
    else
      {
      break;
      }
    }
  j = (this->FrameBufferIndex + i) % this->FrameBufferSize;
  if (stamp[j] != 0.0 && stamp[j] < 980000000.0)
    {
    vtkWarningMacro("Rewind: bogus time stamp!");
    }
  else
    {
    this->AdvanceFrameBuffer(-i);
    this->FrameIndex = (this->FrameIndex - i) % this->FrameBufferSize;
    while (this->FrameIndex < 0)
      {
      this->FrameIndex += this->FrameBufferSize;
      }
    }

  this->FrameBufferMutex->Unlock();
}

//----------------------------------------------------------------------------
// Fast-forward to the frame with the latest timestamp.
void vtkVideoSource::FastForward()
{
  this->FrameBufferMutex->Lock();

  double *stamp = this->FrameBufferTimeStamps;
  double highest = 0;
  int i, j;

  if (this->FrameBufferSize)
    {
    highest = stamp[this->FrameBufferIndex];
    }
  for (i = 0; i < this->FrameBufferSize; i++)
    {
    j = (this->FrameBufferIndex - i - 1) % this->FrameBufferSize;
    while (j < 0)
      {
      j += this->FrameBufferSize;
      }
    if (stamp[j] != 0.0 && stamp[j] >= highest)
      {
      highest = stamp[j];
      }
    else
      {
      break;
      }
    }
  j = (this->FrameBufferIndex - i) % this->FrameBufferSize;
  while (j < 0)
    {
    j += this->FrameBufferSize;
    }
  if (stamp[j] != 0.0 && stamp[j] < 980000000.0)
    {
    vtkWarningMacro("FastForward: bogus time stamp!");
    }
  else
    {
    this->AdvanceFrameBuffer(i);
    this->FrameIndex = (this->FrameIndex + i) % this->FrameBufferSize;
    while (this->FrameIndex < 0)
      {
      this->FrameIndex += this->FrameBufferSize;
      }
    }

  this->FrameBufferMutex->Unlock();
}

//----------------------------------------------------------------------------
// Rotate the buffers
void vtkVideoSource::Seek(int n)
{
  this->FrameBufferMutex->Lock();
  this->AdvanceFrameBuffer(n);
  this->FrameIndex = (this->FrameIndex + n) % this->FrameBufferSize;
  while (this->FrameIndex < 0)
    {
    this->FrameIndex += this->FrameBufferSize;
    }
  this->FrameBufferMutex->Unlock();
  this->Modified();
}

//----------------------------------------------------------------------------
// The grab function, which should (of course) be overridden to do
// the appropriate hardware stuff.  This function should never be
// called asynchronously.
void vtkVideoSource::Grab()
{
  // ensure that the hardware is initialized.
  this->Initialize();

  this->InternalGrab();
}

//----------------------------------------------------------------------------
// Override this and provide checks to ensure an appropriate number
// of components was asked for (i.e. 1 for greyscale, 3 for RGB,
// or 4 for RGBA)
void vtkVideoSource::SetOutputFormat(int format)
{
  if (format == this->OutputFormat)
    {
    return;
    }

  this->OutputFormat = format;

  // convert color format to number of scalar components
  int numComponents = 1;

  switch (this->OutputFormat)
    {
    case VTK_RGBA:
      numComponents = 4;
      break;
    case VTK_RGB:
      numComponents = 3;
      break;
    case VTK_LUMINANCE_ALPHA:
      numComponents = 2;
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
      }
    this->FrameBufferMutex->Unlock();
    }

  this->Modified();
}

//----------------------------------------------------------------------------
// set or change the circular buffer size
// you will have to override this if you want the buffers
// to be device-specific (i.e. something other than vtkDataArray)
void vtkVideoSource::SetFrameBufferSize(int bufsize)
{
  int i;
  void **framebuffer;
  double *timestamps;

  if (bufsize < 0)
    {
    vtkErrorMacro(<< "SetFrameBufferSize: There must be at least one framebuffer");
    }

  if (bufsize == this->FrameBufferSize && bufsize != 0)
    {
    return;
    }

  this->FrameBufferMutex->Lock();

  if (this->FrameBuffer == 0)
    {
    if (bufsize > 0)
      {
      this->FrameBufferIndex = 0;
      this->FrameIndex = -1;
      this->FrameBuffer = new void *[bufsize];
      this->FrameBufferTimeStamps = new double[bufsize];
      for (i = 0; i < bufsize; i++)
        {
        this->FrameBuffer[i] = vtkUnsignedCharArray::New();
        this->FrameBufferTimeStamps[i] = 0.0;
        }
      this->FrameBufferSize = bufsize;
      this->Modified();
      }
    }
  else
    {
    if (bufsize > 0)
      {
      framebuffer = new void *[bufsize];
      timestamps = new double[bufsize];
      }
    else
      {
      framebuffer = NULL;
      timestamps = NULL;
      }

    // create new image buffers if necessary
    for (i = 0; i < bufsize - this->FrameBufferSize; i++)
      {
      framebuffer[i] = vtkUnsignedCharArray::New();
      timestamps[i] = 0.0;
      }
    // copy over old image buffers
    for (; i < bufsize; i++)
      {
      framebuffer[i] = this->FrameBuffer[i-(bufsize-this->FrameBufferSize)];
      }

    // delete image buffers we no longer need
    for (i = 0; i < this->FrameBufferSize-bufsize; i++)
      {
      reinterpret_cast<vtkDataArray *>(this->FrameBuffer[i])->Delete();
      }

    delete [] this->FrameBuffer;
    this->FrameBuffer = framebuffer;
    delete [] this->FrameBufferTimeStamps;
    this->FrameBufferTimeStamps = timestamps;

    // make sure that frame buffer index is within the buffer
    if (bufsize > 0)
      {
      this->FrameBufferIndex = this->FrameBufferIndex % bufsize;
      if (this->FrameIndex >= bufsize)
        {
        this->FrameIndex = bufsize - 1;
        }
      }
    else
      {
      this->FrameBufferIndex = 0;
      this->FrameIndex = -1;
      }

    this->FrameBufferSize = bufsize;
    this->Modified();
    }

  if (this->Initialized)
    {
    this->UpdateFrameBuffer();
    }

  this->FrameBufferMutex->Unlock();
}

//----------------------------------------------------------------------------
// This function MUST be called only from within a FrameBufferMutex->Lock()
void vtkVideoSource::AdvanceFrameBuffer(int n)
{
  int i = (this->FrameBufferIndex - n) % this->FrameBufferSize;
  while (i < 0)
    {
    i += this->FrameBufferSize;
    }
  this->FrameBufferIndex = i;
}

//----------------------------------------------------------------------------
double vtkVideoSource::GetFrameTimeStamp(int frame)
{
  double timeStamp;

  this->FrameBufferMutex->Lock();

  if (this->FrameBufferSize <= 0)
    {
    return 0.0;
    }

  timeStamp = this->FrameBufferTimeStamps[(this->FrameBufferIndex + frame) \
                                         % this->FrameBufferSize];
  this->FrameBufferMutex->Unlock();

  return timeStamp;
}

//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
int vtkVideoSource::RequestInformation(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  int extent[6];

  // ensure that the hardware is initialized.
  this->Initialize();

  for (i = 0; i < 3; i++)
    {
    // initially set extent to the OutputWholeExtent
    extent[2*i] = this->OutputWholeExtent[2*i];
    extent[2*i+1] = this->OutputWholeExtent[2*i+1];
    // if 'flag' is set in output extent, use the FrameBufferExtent instead
    if (extent[2*i+1] < extent[2*i])
      {
      extent[2*i] = 0;
      extent[2*i+1] = \
        this->FrameBufferExtent[2*i+1] - this->FrameBufferExtent[2*i];
      }
    this->FrameOutputExtent[2*i] = extent[2*i];
    this->FrameOutputExtent[2*i+1] = extent[2*i+1];
    }

  int numFrames = this->NumberOfOutputFrames;
  if (numFrames < 1)
    {
    numFrames = 1;
    }
  if (numFrames > this->FrameBufferSize)
    {
    numFrames = this->FrameBufferSize;
    }

  // multiply Z extent by number of frames to output
  extent[5] = extent[4] + (extent[5]-extent[4]+1) * numFrames - 1;

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);

  // set the spacing
  outInfo->Set(vtkDataObject::SPACING(),this->DataSpacing,3);

  // set the origin.
  outInfo->Set(vtkDataObject::ORIGIN(),this->DataOrigin,3);

  // set default data type (8 bit greyscale)
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR,
    this->NumberOfScalarComponents);

  return 1;
}

//----------------------------------------------------------------------------
// The UnpackRasterLine method should be overridden if the framebuffer uses
// unusual pixel packing formats, such as XRGB XBRG BGRX BGR etc.
// The version below assumes that the packing of the framebuffer is
// identical to that of the output.
void vtkVideoSource::UnpackRasterLine(char *outPtr, char *rowPtr,
                                      int start, int count)
{
  char *inPtr = rowPtr + start*this->NumberOfScalarComponents;
  memcpy(outPtr,inPtr,count*this->NumberOfScalarComponents);
  if (this->OutputFormat == VTK_RGBA)
    { // RGBA image: need to copy in the opacity
    unsigned char alpha = (unsigned char)(this->Opacity*255);
    int k;
    outPtr += 3;
    for (k = 0; k < count; k++)
      {
      outPtr[4*k] = alpha;
      }
    }
}

//----------------------------------------------------------------------------
// The Execute method is fairly complex, so I would not recommend overriding
// it unless you have to.  Override the UnpackRasterLine() method instead.
// You should only have to override it if you are using something other
// than 8-bit vtkUnsignedCharArray for the frame buffer.
int vtkVideoSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData *data = this->AllocateOutputData(
    vtkImageData::GetData(outInfo),
    outInfo);
  int i,j;

  int outputExtent[6];     // will later be clipped in Z to a single frame
  int saveOutputExtent[6]; // will possibly contain multiple frames
  data->GetExtent(outputExtent);
  for (i = 0; i < 6; i++)
    {
    saveOutputExtent[i] = outputExtent[i];
    }
  // clip to extent to the Z size of one frame
  outputExtent[4] = this->FrameOutputExtent[4];
  outputExtent[5] = this->FrameOutputExtent[5];

  int frameExtentX = this->FrameBufferExtent[1]-this->FrameBufferExtent[0]+1;
  int frameExtentY = this->FrameBufferExtent[3]-this->FrameBufferExtent[2]+1;
  int frameExtentZ = this->FrameBufferExtent[5]-this->FrameBufferExtent[4]+1;

  int extentX = outputExtent[1]-outputExtent[0]+1;
  int extentY = outputExtent[3]-outputExtent[2]+1;
  int extentZ = outputExtent[5]-outputExtent[4]+1;

  // if the output is more than a single frame,
  // then the output will cover a partial or full first frame,
  // several full frames, and a partial or full last frame

  // index and Z size of the first frame in the output extent
  int firstFrame = (saveOutputExtent[4]-outputExtent[4])/extentZ;
  int firstOutputExtent4 = saveOutputExtent[4] - extentZ*firstFrame;

  // index and Z size of the final frame in the output extent
  int finalFrame = (saveOutputExtent[5]-outputExtent[4])/extentZ;
  int finalOutputExtent5 = saveOutputExtent[5] - extentZ*finalFrame;

  char *outPtr = (char *)data->GetScalarPointer();
  char *outPtrTmp;

  int inIncY = (frameExtentX*this->FrameBufferBitsPerPixel + 7)/8;
  inIncY = ((inIncY + this->FrameBufferRowAlignment - 1)/
            this->FrameBufferRowAlignment)*this->FrameBufferRowAlignment;
  int inIncZ = inIncY*frameExtentY;

  int outIncX = this->NumberOfScalarComponents;
  int outIncY = outIncX*extentX;
  int outIncZ = outIncY*extentY;

  int inPadX = 0;
  int inPadY = 0;
  int inPadZ; // do inPadZ later

  int outPadX = -outputExtent[0];
  int outPadY = -outputExtent[2];
  int outPadZ;  // do outPadZ later

  if (outPadX < 0)
    {
    inPadX -= outPadX;
    outPadX = 0;
    }

  if (outPadY < 0)
    {
    inPadY -= outPadY;
    outPadY = 0;
    }

  int outX = frameExtentX - inPadX;
  int outY = frameExtentY - inPadY;
  int outZ; // do outZ later

  if (outX > extentX - outPadX)
    {
    outX = extentX - outPadX;
    }
  if (outY > extentY - outPadY)
    {
    outY = extentY - outPadY;
    }

  // if output extent has changed, need to initialize output to black
  for (i = 0; i < 3; i++)
    {
    if (saveOutputExtent[i] != this->LastOutputExtent[i])
      {
      this->LastOutputExtent[i] = saveOutputExtent[i];
      this->OutputNeedsInitialization = 1;
      }
    }

  // ditto for number of scalar components
  if (data->GetNumberOfScalarComponents() !=
      this->LastNumberOfScalarComponents)
    {
    this->LastNumberOfScalarComponents = data->GetNumberOfScalarComponents();
    this->OutputNeedsInitialization = 1;
    }

  // initialize output to zero only when necessary
  if (this->OutputNeedsInitialization)
    {
    memset(outPtr,0,
           (saveOutputExtent[1]-saveOutputExtent[0]+1)*
           (saveOutputExtent[3]-saveOutputExtent[2]+1)*
           (saveOutputExtent[5]-saveOutputExtent[4]+1)*outIncX);
    this->OutputNeedsInitialization = 0;
    }

  // we have to modify the outputExtent of the first frame,
  // because it might be complete (it will be restored after
  // the first frame has been copied to the output)
  int saveOutputExtent4 = outputExtent[4];
  outputExtent[4] = firstOutputExtent4;

  this->FrameBufferMutex->Lock();

  int index = this->FrameBufferIndex;
  this->FrameTimeStamp =
    this->FrameBufferTimeStamps[index % this->FrameBufferSize];

  int frame;
  for (frame = firstFrame; frame <= finalFrame; frame++)
    {
    if (frame == finalFrame)
      {
      outputExtent[5] = finalOutputExtent5;
      }

    vtkDataArray *frameBuffer = reinterpret_cast<vtkDataArray *>(this->FrameBuffer[(index + frame) % this->FrameBufferSize]);

    char *inPtr = reinterpret_cast<char*>(frameBuffer->GetVoidPointer(0));
    char *inPtrTmp ;

    extentZ = outputExtent[5]-outputExtent[4]+1;
    inPadZ = 0;
    outPadZ = -outputExtent[4];

    if (outPadZ < 0)
      {
      inPadZ -= outPadZ;
      outPadZ = 0;
      }

    outZ = frameExtentZ - inPadZ;

    if (outZ > extentZ - outPadZ)
      {
      outZ = extentZ - outPadZ;
      }

    if (this->FlipFrames)
      { // apply a vertical flip while copying to output
      outPtr += outIncZ*outPadZ+outIncY*outPadY+outIncX*outPadX;
      inPtr += inIncZ*inPadZ+inIncY*(frameExtentY-inPadY-outY);

      for (i = 0; i < outZ; i++)
        {
        inPtrTmp = inPtr;
        outPtrTmp = outPtr + outIncY*outY;
        for (j = 0; j < outY; j++)
          {
          outPtrTmp -= outIncY;
          if (outX > 0)
            {
            this->UnpackRasterLine(outPtrTmp,inPtrTmp,inPadX,outX);
            }
          inPtrTmp += inIncY;
          }
        outPtr += outIncZ;
        inPtr += inIncZ;
        }
      }
    else
      { // don't apply a vertical flip
      outPtr += outIncZ*outPadZ+outIncY*outPadY+outIncX*outPadX;
      inPtr += inIncZ*inPadZ+inIncY*inPadY;

      for (i = 0; i < outZ; i++)
        {
        inPtrTmp = inPtr;
        outPtrTmp = outPtr;
        for (j = 0; j < outY; j++)
          {
          if (outX > 0)
            {
            this->UnpackRasterLine(outPtrTmp,inPtrTmp,inPadX,outX);
            }
          outPtrTmp += outIncY;
          inPtrTmp += inIncY;
          }
        outPtr += outIncZ;
        inPtr += inIncZ;
        }
      }
    // restore the output extent once the first frame is done
    outputExtent[4] = saveOutputExtent4;
    }

  this->FrameBufferMutex->Unlock();

  return 1;
}
