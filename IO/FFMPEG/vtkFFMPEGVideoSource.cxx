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

#include "vtkConditionVariable.h"
#include "vtkCriticalSection.h"
#include "vtkMultiThreader.h"
#include "vtkMutexLock.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtksys/SystemTools.hxx"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/stereo3d.h>
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
// if building libvpx make sure to add -enable-libvpx to the ffmpeg configure line
// and do the following
//
// export INCLUDE="C:\ffmpeg\include"\;$INCLUDE
// export LIB="C:\ffmpeg\lib;C:\ffmpeg\lib\x64"\;$LIB
//
// ./configure --target=x86_64-win64-vs15 --enable-vp8 --enable-vp9 --prefix=/c/ffmpeg
// make
// make install
// rename to vpx.lib
//
// Then to build ffmpeg as follows
//
// ./configure --enable-asm --enable-x86asm --arch=amd64  --disable-avdevice --enable-swscale
// --disable-doc --disable-ffplay --disable-ffprobe --disable-ffmpeg --enable-shared
// --disable-static --disable-bzlib --disable-libopenjpeg --disable-iconv --disable-zlib
// --prefix=/c/ffmpeg --toolchain=msvc make make install
//
// add libs to vtk build, I just add them to the FFMPEG_LIBAVCODEC_LIBRARIES variable
//
// Ws2_32.lib
// Bcrypt.lib
// Secur32.dll
//
/////////////////////////////////////////////////////////////////////////

class vtkFFMPEGVideoSourceInternal
{
public:
  vtkFFMPEGVideoSourceInternal() {}
  void ReleaseSystemResources()
  {
    if (this->Frame)
    {
      av_frame_free(&this->Frame);
      this->Frame = nullptr;
    }
    if (this->AudioFrame)
    {
      av_frame_free(&this->AudioFrame);
      this->AudioFrame = nullptr;
    }
    if (this->VideoDecodeContext)
    {
      avcodec_close(this->VideoDecodeContext);
      this->VideoDecodeContext = nullptr;
    }
    if (this->AudioDecodeContext)
    {
      avcodec_close(this->AudioDecodeContext);
      this->AudioDecodeContext = nullptr;
    }
    if (this->FormatContext)
    {
      avformat_close_input(&this->FormatContext);
      this->FormatContext = nullptr;
    }
    if (this->RGBContext)
    {
      sws_freeContext(this->RGBContext);
      this->RGBContext = nullptr;
    }
  }

  AVFormatContext* FormatContext = nullptr;
  AVCodecContext* VideoDecodeContext = nullptr;
  AVCodecContext* AudioDecodeContext = nullptr;
  AVStream* VideoStream = nullptr;
  AVStream* AudioStream = nullptr;
  int VideoStreamIndex = -1;
  int AudioStreamIndex = -1;
  AVFrame* Frame = nullptr;
  AVFrame* AudioFrame = nullptr;
  AVPacket Packet;
  struct SwsContext* RGBContext = nullptr;
};

vtkStandardNewMacro(vtkFFMPEGVideoSource);

//----------------------------------------------------------------------------
vtkFFMPEGVideoSource::vtkFFMPEGVideoSource()
  : AudioCallback(nullptr)
  , AudioCallbackClientData(nullptr)
  , DecodingThreads(4)
  , DrainAudioThreadId(-1)
  , DrainThreadId(-1)
  , EndOfFile(true)
  , FeedThreadId(-1)
  , FileName(nullptr)
{
  // changed from superclass
  this->OutputFormat = VTK_RGB;
  this->FrameBufferBitsPerPixel = 24;
  this->NumberOfScalarComponents = 3;
  this->FrameBufferRowAlignment = 4;

  this->Internal = new vtkFFMPEGVideoSourceInternal;

  this->Stereo3D = false;
}

//----------------------------------------------------------------------------
vtkFFMPEGVideoSource::~vtkFFMPEGVideoSource()
{
  this->vtkFFMPEGVideoSource::ReleaseSystemResources();
  delete[] this->FileName;
  delete this->Internal;
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

#ifdef NDEBUG
  av_log_set_level(AV_LOG_ERROR);
#endif

  /* open input file, and allocate format context */
  if (avformat_open_input(&this->Internal->FormatContext, this->FileName, nullptr, nullptr) < 0)
  {
    vtkErrorMacro("Could not open source file " << this->FileName);
    return;
  }

  // local var to keep the code shorter
  AVFormatContext* fcontext = this->Internal->FormatContext;

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

  this->Internal->VideoStream = fcontext->streams[this->Internal->VideoStreamIndex];

  AVCodec* dec = avcodec_find_decoder(this->Internal->VideoStream->codecpar->codec_id);
  if (!dec)
  {
    vtkErrorMacro("Failed to find codec for video");
    return;
  }
  this->Internal->VideoDecodeContext = avcodec_alloc_context3(dec);

  this->Internal->VideoDecodeContext->thread_count = this->DecodingThreads;
  // this->Internal->VideoDecodeContext->thread_type = FF_THREAD_FRAME;

  // examine the video stream side data for additional information
  this->Stereo3D = false;
  if (this->Internal->VideoStream->nb_side_data > 0)
  {
    for (int i = 0; i < this->Internal->VideoStream->nb_side_data; ++i)
    {
      AVPacketSideData sd = this->Internal->VideoStream->side_data[i];
      if (sd.type == AV_PKT_DATA_STEREO3D)
      {
        AVStereo3D* stereo = reinterpret_cast<AVStereo3D*>(sd.data);
        if (stereo->type == AV_STEREO3D_TOPBOTTOM)
        {
          this->Stereo3D = true;
        }
      }
    }
  }

  avcodec_parameters_to_context(
    this->Internal->VideoDecodeContext, this->Internal->VideoStream->codecpar);
  avcodec_open2(this->Internal->VideoDecodeContext, dec, nullptr);

  AVDictionary* opts = nullptr;
  /* Init the decoders, with or without reference counting */
  av_dict_set(&opts, "refcounted_frames", "1", 0);
  if (avcodec_open2(this->Internal->VideoDecodeContext, dec, &opts) < 0)
  {
    vtkErrorMacro("Failed to open codec for video");
    return;
  }

  this->SetFrameRate(static_cast<double>(this->Internal->VideoStream->r_frame_rate.num) /
    this->Internal->VideoStream->r_frame_rate.den);

  this->SetFrameSize(
    this->Internal->VideoDecodeContext->width, this->Internal->VideoDecodeContext->height, 1);

  // create a something to RGB converter
  this->Internal->RGBContext = sws_getContext(this->Internal->VideoDecodeContext->width,
    this->Internal->VideoDecodeContext->height, this->Internal->VideoDecodeContext->pix_fmt,
    this->Internal->VideoDecodeContext->width, this->Internal->VideoDecodeContext->height,
    AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
  if (!this->Internal->RGBContext)
  {
    vtkErrorMacro("Failed to create RGB context");
  }

  // now handle audio streams, these are optional
  this->Internal->AudioStreamIndex =
    av_find_best_stream(fcontext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  // do we have an audio stream?
  if (this->Internal->AudioStreamIndex >= 0)
  {
    this->Internal->AudioStream = fcontext->streams[this->Internal->AudioStreamIndex];

    AVCodec* adec = avcodec_find_decoder(this->Internal->AudioStream->codecpar->codec_id);
    if (!adec)
    {
      vtkErrorMacro("Failed to find codec for audio");
      return;
    }

    this->Internal->AudioDecodeContext = avcodec_alloc_context3(adec);
    avcodec_parameters_to_context(
      this->Internal->AudioDecodeContext, this->Internal->AudioStream->codecpar);
    avcodec_open2(this->Internal->AudioDecodeContext, adec, nullptr);

    AVDictionary* aopts = nullptr;
    /* Init the decoders, with or without reference counting */
    av_dict_set(&aopts, "refcounted_frames", "1", 0);
    if (avcodec_open2(this->Internal->AudioDecodeContext, adec, &aopts) < 0)
    {
      vtkErrorMacro("Failed to open codec for audio");
      return;
    }

    this->Internal->AudioFrame = av_frame_alloc();
    if (!this->Internal->AudioFrame)
    {
      vtkErrorMacro("Could not allocate audio frame");
      return;
    }
  }

  this->EndOfFile = false;

  this->Internal->Frame = av_frame_alloc();
  if (!this->Internal->Frame)
  {
    vtkErrorMacro("Could not allocate frame");
    return;
  }

  /* initialize packet, set data to nullptr, let the demuxer fill it */
  av_init_packet(&this->Internal->Packet);
  this->Internal->Packet.data = nullptr;
  this->Internal->Packet.size = 0;

  // update framebuffer again to reflect any changes which
  // might have occurred
  this->UpdateFrameBuffer();

  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// Feed frames to the decoder
void* vtkFFMPEGVideoSource::FeedThread(vtkMultiThreader::ThreadInfo* data)
{
  vtkFFMPEGVideoSource* self = static_cast<vtkFFMPEGVideoSource*>(data->UserData);
  return self->Feed(data);
}

//
// based off of https://blogs.gentoo.org/lu_zero/2016/03/29/new-avcodec-api/
//
void* vtkFFMPEGVideoSource::Feed(vtkMultiThreader::ThreadInfo* data)
{
  bool done = false;

  unsigned short count = 0;
  bool retryPacket = false;
  int fret = AVERROR_EOF;

  while (!done)
  {
    // read in the packet
    if (!retryPacket)
    {
      av_packet_unref(&this->Internal->Packet);
      fret = av_read_frame(this->Internal->FormatContext, &this->Internal->Packet);
    }
    retryPacket = false;
    // feed video
    if (fret >= 0 && this->Internal->Packet.stream_index == this->Internal->VideoStreamIndex)
    {
      // lock the decoder
      this->FeedMutex->Lock();

      int sret = avcodec_send_packet(this->Internal->VideoDecodeContext, &this->Internal->Packet);
      if (sret == 0) // good decode
      {
        this->FeedCondition->Signal();
      }
      else if (sret == AVERROR(EAGAIN))
      {
        // Signal the draining loop
        this->FeedCondition->Signal();
        // Wait here
        this->FeedCondition->Wait(this->FeedMutex);
        retryPacket = true;
      }
      else if (sret < 0) // error
      {
        this->FeedMutex->Unlock();
        return nullptr;
      }

      this->FeedMutex->Unlock();
    }

    // feed audio
    if (fret >= 0 && this->Internal->Packet.stream_index == this->Internal->AudioStreamIndex)
    {
      // lock the decoder
      this->FeedAudioMutex->Lock();

      int sret = avcodec_send_packet(this->Internal->AudioDecodeContext, &this->Internal->Packet);
      if (sret == 0) // good decode
      {
        this->FeedAudioCondition->Signal();
      }
      else if (sret == AVERROR(EAGAIN))
      {
        // Signal the draining loop
        this->FeedAudioCondition->Signal();
        // Wait here
        this->FeedAudioCondition->Wait(this->FeedAudioMutex);
        retryPacket = true;
      }
      else if (sret < 0) // error
      {
        this->FeedAudioMutex->Unlock();
        return nullptr;
      }

      this->FeedAudioMutex->Unlock();
    }

    // are we out of data?
    if (fret == AVERROR_EOF)
    {
      done = true;
    }

    // check to see if we are being told to quit every so often
    if (count == 10)
    {
      std::lock_guard<std::mutex>(*data->ActiveFlagLock);
      done = done || (*(data->ActiveFlag) == 0);
      count = 0;
    }
    count++;
  }

  // flush remaining data
  this->FeedMutex->Lock();
  avcodec_send_packet(this->Internal->VideoDecodeContext, nullptr);
  this->FeedCondition->Signal();
  this->FeedMutex->Unlock();

  if (this->Internal->AudioDecodeContext)
  {
    this->FeedAudioMutex->Lock();
    avcodec_send_packet(this->Internal->AudioDecodeContext, nullptr);
    this->FeedAudioCondition->Signal();
    this->FeedAudioMutex->Unlock();
  }

  this->EndOfFile = true;
  return nullptr;
}

//----------------------------------------------------------------------------
// Sleep until the specified absolute time has arrived.
// You must pass a handle to the current thread.
// If '0' is returned, then the thread was aborted before or during the wait.
static void vtkThreadSleep(double time)
{
  static size_t count = 0;

  // loop either until the time has arrived or until the thread is ended
  for (int i = 0;; i++)
  {
    count++;
    double remaining = time - vtkTimerLog::GetUniversalTime();

    // check to see if we are being told to quit
    // data->ActiveFlagLock->Lock();
    // int activeFlag = *(data->ActiveFlag);
    // data->ActiveFlagLock->Unlock();

    // if (activeFlag == 0)
    // {
    //   break;
    // }

    // check to see if we have reached the specified time
    if (remaining <= 0.0)
    {
      if (i == 0 && count % 100 == 0)
      {
        cerr << "dropped frames, now behind by " << remaining << " seconds\n";
      }
      break;
    }

    // do not sleep for more than 0.1 seconds
    if (remaining > 0.1)
    {
      remaining = 0.1;
    }

    vtksys::SystemTools::Delay(static_cast<unsigned int>(remaining * 1000.0));
  }
}

void* vtkFFMPEGVideoSource::DrainThread(vtkMultiThreader::ThreadInfo* data)
{
  vtkFFMPEGVideoSource* self = static_cast<vtkFFMPEGVideoSource*>(data->UserData);
  return self->Drain(data);
}

void* vtkFFMPEGVideoSource::Drain(vtkMultiThreader::ThreadInfo* data)
{
  bool done = false;
  unsigned short count = 0;

  double startTime = vtkTimerLog::GetUniversalTime();
  double rate = this->GetFrameRate();
  int frame = 0;

  while (!done)
  {
    this->FeedMutex->Lock();

    int ret = avcodec_receive_frame(this->Internal->VideoDecodeContext, this->Internal->Frame);
    if (ret == 0)
    {
      this->FeedCondition->Signal();
    }
    else if (ret == AVERROR(EAGAIN))
    {
      // Signal the feeding loop
      this->FeedCondition->Signal();
      // Wait here
      this->FeedCondition->Wait(this->FeedMutex);
    }
    else if (ret == AVERROR_EOF)
    {
      return nullptr;
    }
    else if (ret < 0) // error code
    {
      this->FeedMutex->Unlock();
      cerr << "video drain thread exiting on error!\n";
      return nullptr;
    }

    this->FeedMutex->Unlock();

    if (ret == 0)
    {
      vtkThreadSleep(startTime + frame / rate);
      if (this->VideoCallback)
      {
        vtkFFMPEGVideoSourceVideoCallbackData cbd;
        cbd.Height = this->Internal->Frame->height;
        int p = 0;
        while (this->Internal->Frame->data[p] != nullptr && p < 8)
        {
          cbd.LineSize[p] = this->Internal->Frame->linesize[p];
          cbd.Data[p] = this->Internal->Frame->data[p];
          ++p;
        }
        cbd.Caller = this;
        cbd.ClientData = this->VideoCallbackClientData;
        this->VideoCallback(cbd);
      }
      else
      {
        this->InternalGrab();
      }
      frame++;
    }

    // check to see if we are being told to quit every so often
    if (count == 10)
    {
      std::lock_guard<std::mutex>(*data->ActiveFlagLock);
      done = done || (*(data->ActiveFlag) == 0);
      count = 0;
    }
    count++;
  }

  return nullptr;
}

void* vtkFFMPEGVideoSource::DrainAudioThread(vtkMultiThreader::ThreadInfo* data)
{
  vtkFFMPEGVideoSource* self = static_cast<vtkFFMPEGVideoSource*>(data->UserData);
  return self->DrainAudio(data);
}

void* vtkFFMPEGVideoSource::DrainAudio(vtkMultiThreader::ThreadInfo* data)
{
  bool done = false;
  unsigned short count = 0;

  int frame = 0;

  while (!done)
  {
    this->FeedAudioMutex->Lock();

    int ret = avcodec_receive_frame(this->Internal->AudioDecodeContext, this->Internal->AudioFrame);
    if (ret == 0)
    {
      this->FeedAudioCondition->Signal();
    }
    else if (ret == AVERROR(EAGAIN))
    {
      // Signal the feeding loop
      this->FeedAudioCondition->Signal();
      // Wait here
      this->FeedAudioCondition->Wait(this->FeedAudioMutex);
    }
    else if (ret == AVERROR_EOF)
    {
      return nullptr;
    }
    else if (ret < 0) // error code
    {
      this->FeedAudioMutex->Unlock();
      cerr << "audio drain thread exiting on error!\n";
      return nullptr;
    }

    this->FeedAudioMutex->Unlock();

    if (ret == 0)
    {
      // do something with the data here
      // vtkThreadSleep(startTime + frame/rate);

      int sampleFormat = VTK_UNSIGNED_CHAR;
      bool packed = true;
      switch (this->Internal->AudioDecodeContext->sample_fmt)
      {
        default:
        case AV_SAMPLE_FMT_U8:
          sampleFormat = VTK_UNSIGNED_CHAR;
          break;
        case AV_SAMPLE_FMT_U8P:
          sampleFormat = VTK_UNSIGNED_CHAR;
          packed = false;
          break;
        case AV_SAMPLE_FMT_S16:
          sampleFormat = VTK_SHORT;
          break;
        case AV_SAMPLE_FMT_S16P:
          sampleFormat = VTK_SHORT;
          packed = false;
          break;
        case AV_SAMPLE_FMT_S32:
          sampleFormat = VTK_LONG;
          break;
        case AV_SAMPLE_FMT_S32P:
          sampleFormat = VTK_LONG;
          packed = false;
          break;
        case AV_SAMPLE_FMT_FLT:
          sampleFormat = VTK_FLOAT;
          break;
        case AV_SAMPLE_FMT_FLTP:
          sampleFormat = VTK_FLOAT;
          packed = false;
          break;
        case AV_SAMPLE_FMT_DBL:
          sampleFormat = VTK_DOUBLE;
          break;
        case AV_SAMPLE_FMT_DBLP:
          sampleFormat = VTK_DOUBLE;
          packed = false;
          break;
      }

      if (this->AudioCallback)
      {
        vtkFFMPEGVideoSourceAudioCallbackData cbd;
        cbd.NumberOfSamples = this->Internal->AudioFrame->nb_samples;
        cbd.BytesPerSample =
          av_get_bytes_per_sample(this->Internal->AudioDecodeContext->sample_fmt);
        cbd.NumberOfChannels = this->Internal->AudioDecodeContext->channels;
        cbd.SampleRate = this->Internal->AudioDecodeContext->sample_rate;
        cbd.DataType = sampleFormat;
        cbd.Data = this->Internal->AudioFrame->extended_data;
        cbd.Caller = this;
        cbd.Packed = packed;
        cbd.ClientData = this->AudioCallbackClientData;
        this->AudioCallback(cbd);
      }
      frame++;
    }

    // check to see if we are being told to quit every so often
    if (count == 10)
    {
      std::lock_guard<std::mutex>(*data->ActiveFlagLock);
      done = done || (*(data->ActiveFlag) == 0);
      count = 0;
    }
    count++;
  }

  return nullptr;
}

void vtkFFMPEGVideoSource::ReadFrame()
{
  // first try to grab a frame from data we already have
  bool gotFrame = false;
  while (!gotFrame && (!this->EndOfFile || this->Internal->Packet.size > 0))
  {
    int ret = AVERROR(EAGAIN);
    if (this->Internal->Packet.size > 0)
    {
      ret = avcodec_receive_frame(this->Internal->VideoDecodeContext, this->Internal->Frame);
      if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
      {
        vtkErrorMacro("codec did not receive video frame");
        return;
      }
      if (ret == AVERROR_EOF)
      {
        this->EndOfFile = true;
        return;
      }
      if (ret == 0)
      {
        gotFrame = true;
      }
    }

    // if we are out of data then we must send more
    if (ret == AVERROR(EAGAIN) && !this->EndOfFile)
    {
      // if the packet is empty read more data from the file
      av_packet_unref(&this->Internal->Packet);
      int fret = av_read_frame(this->Internal->FormatContext, &this->Internal->Packet);
      if (fret >= 0 && this->Internal->Packet.stream_index == this->Internal->VideoStreamIndex)
      {
        int sret = avcodec_send_packet(this->Internal->VideoDecodeContext, &this->Internal->Packet);
        if (sret < 0 && sret != AVERROR(EAGAIN) && sret != AVERROR_EOF)
        {
          vtkErrorMacro("codec did not send packet");
          return;
        }
      }
      if (fret == AVERROR_EOF)
      {
        this->EndOfFile = true;
        return;
      }
    }
  }
}

void vtkFFMPEGVideoSource::InternalGrab()
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

  unsigned char* ptr =
    (unsigned char*)((reinterpret_cast<vtkUnsignedCharArray*>(this->FrameBuffer[index]))
                       ->GetPointer(0));

  // the DIB has rows which are multiples of 4 bytes
  int outBytesPerRow =
    ((this->FrameBufferExtent[1] - this->FrameBufferExtent[0] + 1) * this->FrameBufferBitsPerPixel +
      7) /
    8;
  outBytesPerRow += outBytesPerRow % this->FrameBufferRowAlignment;
  outBytesPerRow += outBytesPerRow % 4;
  int rows = this->FrameBufferExtent[3] - this->FrameBufferExtent[2] + 1;

  // update frame time
  this->FrameBufferTimeStamps[index] = this->StartTimeStamp + this->FrameCount / this->FrameRate;

  uint8_t* dst[4];
  int dstStride[4];
  // we flip y axis here
  dstStride[0] = -outBytesPerRow;
  dst[0] = ptr + outBytesPerRow * (rows - 1);
  sws_scale(this->Internal->RGBContext, this->Internal->Frame->data,
    this->Internal->Frame->linesize, 0, this->Internal->Frame->height, dst, dstStride);

  this->FrameBufferMutex->Unlock();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::ReleaseSystemResources()
{
  if (this->Initialized)
  {
    this->Internal->ReleaseSystemResources();
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

  this->ReadFrame();
  this->InternalGrab();
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::Play()
{
  this->vtkVideoSource::Play();
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
    this->FeedThreadId = this->PlayerThreader->SpawnThread(
      (vtkThreadFunctionType)&vtkFFMPEGVideoSource::FeedThread, this);
    this->DrainThreadId = this->PlayerThreader->SpawnThread(
      (vtkThreadFunctionType)&vtkFFMPEGVideoSource::DrainThread, this);
    if (this->Internal->AudioDecodeContext)
    {
      this->DrainAudioThreadId = this->PlayerThreader->SpawnThread(
        (vtkThreadFunctionType)&vtkFFMPEGVideoSource::DrainAudioThread, this);
    }
    else
    {
      this->DrainAudioThreadId = 0;
    }
  }
}

//----------------------------------------------------------------------------
void vtkFFMPEGVideoSource::Stop()
{
  if (this->Playing || this->Recording)
  {
    this->PlayerThreader->TerminateThread(this->FeedThreadId);
    this->PlayerThreader->TerminateThread(this->DrainThreadId);
    if (this->DrainAudioThreadId)
    {
      this->PlayerThreader->TerminateThread(this->DrainAudioThreadId);
    }
    this->Playing = 0;
    this->Recording = 0;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// try for the specified frame size
void vtkFFMPEGVideoSource::SetFrameSize(int x, int y, int z)
{
  if (x == this->FrameSize[0] && y == this->FrameSize[1] && z == this->FrameSize[2])
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

  if (this->FrameBufferBitsPerPixel != numComponents * 8)
  {
    this->FrameBufferMutex->Lock();
    this->FrameBufferBitsPerPixel = numComponents * 8;
    if (this->Initialized)
    {
      this->UpdateFrameBuffer();
    }
    this->FrameBufferMutex->Unlock();
  }

  this->Modified();
}
