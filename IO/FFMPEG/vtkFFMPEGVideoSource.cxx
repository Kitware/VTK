/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGVideoSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFFMPEGVideoSource.h"

#include "vtkCriticalSection.h"
#include "vtkMultiThreader.h"
#include "vtkMutexLock.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtksys/SystemTools.hxx"

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <cctype>

/////////////////////////////////////////////////////////////////////////
// building ffmpeg on windows
//
// vs2017 shell
//
// inside it run msys_command.cmd
//
// export INCLUDE="C:\ffmpeg\include"\;$INCLUDE
// export LIB="C:\ffmpeg\lib;C:\ffmpeg\lib\x64"\;$LIB
//
// ./configure --target=x86_64-win64-vs15 --enable-vp8 --enable-vp9 --prefix=/c/ffmpeg
// make
// make install
// rename to vpx.lib
//
// ./configure --enable-asm --enable-x86asm --arch=amd64  --disable-avdevice --enable-swscale --disable-doc --disable-ffplay --disable-ffprobe --disable-ffmpeg --disable-shared --enable-static --disable-bzlib --disable-libopenjpeg --disable-iconv --disable-zlib --prefix=/c/ffmpeg --toolchain=msvc --enable-libvpx
// make
// make install
//
// add libs to vtk build, I just add them to the FFMPEG_LIBAVCODEC_LIBRARIES variable
//
// Ws2_32.lib
// Bcrypt.lib
// Secur32.dll
// C:/ffmpeg/lib/x64/vpx.lib
/////////////////////////////////////////////////////////////////////////


class vtkFFMPEGVideoSourceInternal
{
public:
  vtkFFMPEGVideoSourceInternal() {}
  AVFormatContext *FormatContext = nullptr;
  AVCodecContext *VideoDecodeContext = nullptr;
  AVCodecContext *AudioDecodeContext = nullptr;
  AVStream *VideoStream = nullptr;
  AVStream *AudioStream = nullptr;
  int VideoStreamIndex = -1;
  int AudioStreamIndex = -1;
  AVFrame *Frame = nullptr;
  AVPacket Packet;
  struct SwsContext* RGBContext = nullptr;
};

vtkStandardNewMacro(vtkFFMPEGVideoSource);

//----------------------------------------------------------------------------
vtkFFMPEGVideoSource::vtkFFMPEGVideoSource()
{
  this->Internal = new vtkFFMPEGVideoSourceInternal;
  this->Initialized = 0;
  this->FileName = nullptr;

  this->FrameRate = 30;
  this->OutputFormat = VTK_RGB;
  this->NumberOfScalarComponents = 3;
  this->FrameBufferBitsPerPixel = 24;
  this->FlipFrames = 0;
  this->FrameBufferRowAlignment = 4;
  this->EndOfFile = true;
}

//----------------------------------------------------------------------------
vtkFFMPEGVideoSource::~vtkFFMPEGVideoSource()
{
  this->vtkFFMPEGVideoSource::ReleaseSystemResources();
  delete [] this->FileName;
  delete this->Internal;
}

int vtkFFMPEGVideoSource::DecodePacket(int *got_frame)
{
  int ret = 0;
  AVPacket pkt = this->Internal->Packet;

  *got_frame = 0;
  if (pkt.stream_index == this->Internal->VideoStreamIndex)
  {
    ret = avcodec_send_packet(this->Internal->VideoDecodeContext, &pkt);
    if (ret == AVERROR_EOF)
    {
      return ret;
    }
    if (ret < 0)
    {
      vtkErrorMacro("codec did not send packet");
      return ret;
    }
    ret = avcodec_receive_frame(this->Internal->VideoDecodeContext,
            this->Internal->Frame);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    {
      vtkErrorMacro("codec did not receive video frame");
      return ret;
    }
    *got_frame = 1;
  }
  else if (pkt.stream_index == this->Internal->AudioStreamIndex)
  {
    /* decode audio frame */
    ret = avcodec_send_packet(this->Internal->AudioDecodeContext, &pkt);
    if (ret < 0)
    {
      vtkErrorMacro("codec did not send packet");
      return ret;
    }
    ret = avcodec_receive_frame(this->Internal->AudioDecodeContext,
            this->Internal->Frame);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    {
      vtkErrorMacro("codec did not receive audio frame");
      return ret;
    }
    // do something with audio...
  }
  return pkt.size;
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::Initialize()
{
  if (this->Initialized)
  {
    return;
  }

  // Preliminary update of frame buffer, just in case we don't get
  // though the initialization but need the framebuffer for Updates
  this->UpdateFrameBuffer();

  /* open input file, and allocate format context */
  if (avformat_open_input(&this->Internal->FormatContext, this->FileName, nullptr, nullptr) < 0)
  {
    vtkErrorMacro("Could not open source file " << this->FileName);
    return;
  }

  // local var to keep the code shorter
  AVFormatContext *fcontext = this->Internal->FormatContext;

  /* retrieve stream information */
  if (avformat_find_stream_info(fcontext, nullptr) < 0)
  {
    vtkErrorMacro("Could not find stream information");
    return;
  }

  this->Internal->VideoStreamIndex =
    av_find_best_stream(fcontext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (this->Internal->VideoStreamIndex < 0)
  {
    vtkErrorMacro("Could not find video stream in input file ");
    return;
  }

  this->Internal->VideoStream =
    fcontext->streams[this->Internal->VideoStreamIndex];

  AVCodec *dec =
    avcodec_find_decoder(this->Internal->VideoStream->codecpar->codec_id);
  if (!dec)
  {
    vtkErrorMacro("Failed to find codec for video");
    return;
  }
  this->Internal->VideoDecodeContext = avcodec_alloc_context3(dec);
  avcodec_parameters_to_context(
    this->Internal->VideoDecodeContext,
    this->Internal->VideoStream->codecpar);
  avcodec_open2(this->Internal->VideoDecodeContext, dec, nullptr);

  AVDictionary *opts = nullptr;
  /* Init the decoders, with or without reference counting */
  av_dict_set(&opts, "refcounted_frames", "1", 0);
  if (avcodec_open2(this->Internal->VideoDecodeContext, dec, &opts) < 0)
  {
    vtkErrorMacro("Failed to open codec for video");
    return;
  }

  this->SetFrameRate(static_cast<double>(this->Internal->VideoStream->r_frame_rate.num)/
    this->Internal->VideoStream->r_frame_rate.den);

  this->SetFrameSize(
    this->Internal->VideoDecodeContext->width,
    this->Internal->VideoDecodeContext->height,
    1);

  // create a something to RGB converter
  this->Internal->RGBContext = sws_getContext(
     this->Internal->VideoDecodeContext->width,
     this->Internal->VideoDecodeContext->height,
     this->Internal->VideoDecodeContext->pix_fmt,
     this->Internal->VideoDecodeContext->width,
     this->Internal->VideoDecodeContext->height,
     AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
  if (!this->Internal->RGBContext)
  {
    vtkErrorMacro("Failed to create RGB context");
  }

  // no audio yet

  this->Internal->Frame = av_frame_alloc();
  if (!this->Internal->Frame)
  {
    vtkErrorMacro("Could not allocate frame");
    return;
  }

  /* initialize packet, set data to NULL, let the demuxer fill it */
  av_init_packet(&this->Internal->Packet);
  this->Internal->Packet.data = NULL;
  this->Internal->Packet.size = 0;

  // update framebuffer again to reflect any changes which
  // might have occurred
  this->UpdateFrameBuffer();

  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// copy the frame into the
// vtkVideoSource framebuffer
void vtkFFMPEGVideoSource::InternalGrab()
{

  int got_frame = 0;
  int ret = 0;
  if (this->Internal->Packet.size <= 0)
  {
    ret = av_read_frame(this->Internal->FormatContext, &this->Internal->Packet);
  }
  if (ret == 0)
  {
    ret = this->DecodePacket(&got_frame);
    if (got_frame)
    {
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

      int index = this->FrameBufferIndex;

      this->FrameCount++;

      unsigned char *ptr = (unsigned char *)
        ((reinterpret_cast<vtkUnsignedCharArray*>(this->FrameBuffer[index])) \
          ->GetPointer(0));

      // the DIB has rows which are multiples of 4 bytes
      int outBytesPerRow = ((this->FrameBufferExtent[1]-
                             this->FrameBufferExtent[0]+1)
                            * this->FrameBufferBitsPerPixel + 7)/8;
      outBytesPerRow += outBytesPerRow % this->FrameBufferRowAlignment;
      outBytesPerRow += outBytesPerRow % 4;
      int rows = this->FrameBufferExtent[3]-this->FrameBufferExtent[2]+1;

      // update frame time
      this->FrameBufferTimeStamps[index] =
        this->StartTimeStamp + this->FrameCount / this->FrameRate;

      uint8_t * dst[4];
      int dstStride[4];
      // we flip y axis here
      dstStride[0] = -outBytesPerRow;
      dst[0] = ptr + outBytesPerRow*(rows-1);
      sws_scale(this->Internal->RGBContext,
        this->Internal->Frame->data,
        this->Internal->Frame->linesize,
        0,
        this->Internal->Frame->height,
        dst, dstStride);

      this->FrameBufferMutex->Unlock();

      this->Modified();
    }
    this->Internal->Packet.data += ret;
    this->Internal->Packet.size -= ret;
  }
  else
  {
    this->EndOfFile = true;
  }
}


//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::ReleaseSystemResources()
{
  if (this->Initialized)
  {
    avcodec_close(this->Internal->VideoDecodeContext);
    avcodec_close(this->Internal->AudioDecodeContext);
    avformat_close_input(&this->Internal->FormatContext);
    av_frame_free(&this->Internal->Frame);
    sws_freeContext(this->Internal->RGBContext);
    this->Initialized = 0;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::Grab()
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

  this->InternalGrab();
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::Play()
{
  this->vtkVideoSource::Play();
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
        // vtkGenericWarningMacro("Dropped a video frame.");
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
// this function runs in an alternate thread to asynchronously grab frames
void *vtkFFMPEGVideoSource::RecordThread(vtkMultiThreader::ThreadInfo *data)
{
  vtkFFMPEGVideoSource *self = static_cast<vtkFFMPEGVideoSource *>(data->UserData);

  double startTime = vtkTimerLog::GetUniversalTime();
  double rate = self->GetFrameRate();
  int frame = 0;

  do
  {
    self->InternalGrab();
    frame++;
  }
  while (!self->EndOfFile && vtkThreadSleep(data, startTime + frame/rate));

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::Record()
{
  if (this->Playing)
  {
    this->Stop();
  }

  if (!this->Recording)
  {
    this->Initialize();

    this->EndOfFile = false;
    this->Recording = 1;
    this->FrameCount = 0;
    this->Modified();
    this->PlayerThreadId =
      this->PlayerThreader->SpawnThread((vtkThreadFunctionType)\
                                &vtkFFMPEGVideoSource::RecordThread,this);
  }
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::Stop()
{
  if (this->Recording)
  {
    this->Recording = 0;
    this->Modified();
  }
  else if (this->Playing)
  {
    this->vtkVideoSource::Stop();
  }
}

//----------------------------------------------------------------------------
// try for the specified frame size
void vtkFFMPEGVideoSource::SetFrameSize(int x, int y, int z)
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
    this->FrameBufferMutex->Unlock();
  }
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::SetFrameRate(float rate)
{
  if (rate == this->FrameRate)
  {
    return;
  }

  this->FrameRate = rate;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::SetOutputFormat(int format)
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
      numComponents = 0;
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
