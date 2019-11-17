/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFFMPEGWriter.h"

#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#if defined(LIBAVFORMAT_VERSION_MAJOR) && LIBAVFORMAT_VERSION_MAJOR >= 57
extern "C"
{
#include <libavutil/imgutils.h>
}
#endif

//---------------------------------------------------------------------------
class vtkFFMPEGWriterInternal
{
public:
  vtkFFMPEGWriterInternal(vtkFFMPEGWriter* creator);
  ~vtkFFMPEGWriterInternal();

  int Start();
  int Write(vtkImageData* id);
  void End();

  int Dim[2];
  int FrameRate;

private:
  vtkFFMPEGWriter* Writer;

  AVFormatContext* avFormatContext;

  AVOutputFormat* avOutputFormat;

  AVStream* avStream;

  AVFrame* rgbInput;
  AVFrame* yuvOutput;

  AVCodecContext* avCodecContext;

  int openedFile;
  int closedFile;
};

//---------------------------------------------------------------------------
vtkFFMPEGWriterInternal::vtkFFMPEGWriterInternal(vtkFFMPEGWriter* creator)
{
  this->Writer = creator;
  this->Dim[0] = 0;
  this->Dim[1] = 0;

  this->avFormatContext = nullptr;

  this->avOutputFormat = nullptr;

  this->avStream = nullptr;

  this->rgbInput = nullptr;
  this->yuvOutput = nullptr;

  this->openedFile = 0;
  this->closedFile = 1;

  this->FrameRate = 25;
}

//---------------------------------------------------------------------------
vtkFFMPEGWriterInternal::~vtkFFMPEGWriterInternal()
{
  if (!this->closedFile)
  {
    this->End();
  }
}

// for newer versions of ffmpeg use the new API as the old has been deprecated
#if defined(LIBAVFORMAT_VERSION_MAJOR) && LIBAVFORMAT_VERSION_MAJOR >= 57

//---------------------------------------------------------------------------
int vtkFFMPEGWriterInternal::Start()
{
  this->closedFile = 0;

#ifdef NDEBUG
  av_log_set_level(AV_LOG_ERROR);
#endif

  // choose avi media file format
  this->avOutputFormat = av_guess_format("avi", nullptr, nullptr);
  if (!this->avOutputFormat)
  {
    vtkGenericWarningMacro(<< "Could not open the avi media file format.");
    return 0;
  }

  if (this->Writer->GetCompression())
  {
    // choose a codec that is easily playable on windows
    this->avOutputFormat->video_codec = AV_CODEC_ID_MJPEG;
  }
  else
  {
    this->avOutputFormat->video_codec = AV_CODEC_ID_RAWVIDEO;
  }

  // create the format context that wraps all of the media output structures
  if (avformat_alloc_output_context2(
        &this->avFormatContext, this->avOutputFormat, nullptr, this->Writer->GetFileName()) < 0)
  {
    vtkGenericWarningMacro(<< "Could not open the format context.");
    return 0;
  }

  AVCodec* codec;
  if (!(codec = avcodec_find_encoder(this->avOutputFormat->video_codec)))
  {
    vtkGenericWarningMacro(<< "Failed to get video codec.");
    return 0;
  }

  // create a stream for that file
  this->avStream = avformat_new_stream(this->avFormatContext, codec);
  if (!this->avStream)
  {
    vtkGenericWarningMacro(<< "Could not create video stream.");
    return 0;
  }

  // Set up the codec.
  if (!(this->avCodecContext = avcodec_alloc_context3(codec)))
  {
    vtkGenericWarningMacro(<< "Failed to allocate codec context.");
    return 0;
  }

  this->avStream->codecpar->codec_id = static_cast<AVCodecID>(this->avOutputFormat->video_codec);
  this->avStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
  this->avStream->codecpar->width = this->Dim[0];
  this->avStream->codecpar->height = this->Dim[1];
  if (this->Writer->GetCompression())
  {
    this->avStream->codecpar->format = AV_PIX_FMT_YUVJ420P;
  }
  else
  {
    this->avStream->codecpar->format = AV_PIX_FMT_BGR24;
  }
  this->avStream->time_base.den = this->FrameRate;
  this->avStream->time_base.num = 1;

  if (!this->Writer->GetBitRate())
  {
    // allow a variable quality/size tradeoff
    switch (this->Writer->GetQuality())
    {
      case 0:
        this->avStream->codecpar->bit_rate = 3 * 1024 * 1024;
        break;
      case 1:
        this->avStream->codecpar->bit_rate = 6 * 1024 * 1024;
        break;
      default:
        this->avStream->codecpar->bit_rate = 12 * 1024 * 1024;
        break;
    }
  }
  else
  {
    this->avStream->codecpar->bit_rate = this->Writer->GetBitRate();
  }

  // to do playback at actual recorded rate, this will need more work see also below
  avcodec_parameters_to_context(this->avCodecContext, this->avStream->codecpar);
  this->avCodecContext->time_base.den = this->FrameRate;
  this->avCodecContext->time_base.num = 1;
  // this->avCodecContext->max_b_frames = 2;
  // about one full frame per second
  this->avCodecContext->gop_size = this->FrameRate;
  if (this->avFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
  {
    this->avCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }
  if (!this->Writer->GetBitRateTolerance())
  {
    this->avCodecContext->bit_rate_tolerance =
      this->avCodecContext->bit_rate; // ffmpeg won't create a codec if brt<br
  }
  else
  {
    this->avCodecContext->bit_rate_tolerance = this->Writer->GetBitRateTolerance();
  }
  avcodec_parameters_from_context(this->avStream->codecpar, this->avCodecContext);

  if (avcodec_open2(this->avCodecContext, codec, nullptr) < 0)
  {
    vtkGenericWarningMacro(<< "Could not open codec.");
    return 0;
  }

  // for the output of the writer's input...
  this->rgbInput = av_frame_alloc();
  if (!this->rgbInput)
  {
    vtkGenericWarningMacro(<< "Could not make rgbInput avframe.");
    return 0;
  }
  this->rgbInput->format = AV_PIX_FMT_RGB24;
  this->rgbInput->width = this->avCodecContext->width;
  this->rgbInput->height = this->avCodecContext->height;
  av_frame_get_buffer(this->rgbInput, 1);

  // and for the output to the codec's input.
  this->yuvOutput = av_frame_alloc();
  if (!this->yuvOutput)
  {
    vtkGenericWarningMacro(<< "Could not make yuvOutput avframe.");
    return 0;
  }
  this->yuvOutput->format = this->avCodecContext->pix_fmt;
  this->yuvOutput->width = this->avCodecContext->width;
  this->yuvOutput->height = this->avCodecContext->height;
  this->yuvOutput->pts = 0;
  av_frame_get_buffer(this->yuvOutput, 1);

  // Finally, open the file and start it off.
  if (!(this->avOutputFormat->flags & AVFMT_NOFILE))
  {
    if (avio_open(&this->avFormatContext->pb, this->Writer->GetFileName(), AVIO_FLAG_WRITE) < 0)
    {
      vtkGenericWarningMacro(<< "Could not open " << this->Writer->GetFileName() << ".");
      return 0;
    }
  }
  this->openedFile = 1;

  if (avformat_write_header(this->avFormatContext, nullptr) < 0)
  {
    vtkGenericWarningMacro(<< "Could not allocate avcodec private data.");
    return 0;
  }
  return 1;
}

//---------------------------------------------------------------------------
int vtkFFMPEGWriterInternal::Write(vtkImageData* id)
{
  this->Writer->GetInputAlgorithm(0, 0)->UpdateWholeExtent();

  // copy the image from the input to the RGB buffer while flipping Y
  unsigned char* rgb = (unsigned char*)id->GetScalarPointer();
  unsigned char* src;
  for (int y = 0; y < this->avCodecContext->height; y++)
  {
    src = rgb + (this->avCodecContext->height - y - 1) * this->avCodecContext->width * 3; // flip Y
    unsigned char* dest = &this->rgbInput->data[0][y * this->rgbInput->linesize[0]];
    memcpy((void*)dest, (void*)src, this->avCodecContext->width * 3);
  }

  // convert that to YUV for input to the codec
  SwsContext* convert_ctx =
    sws_getContext(this->avCodecContext->width, this->avCodecContext->height, AV_PIX_FMT_RGB24,
      this->avCodecContext->width, this->avCodecContext->height, this->avCodecContext->pix_fmt,
      SWS_BICUBIC, nullptr, nullptr, nullptr);

  if (convert_ctx == nullptr)
  {
    vtkGenericWarningMacro(<< "swscale context initialization failed");
    return 0;
  }

  int result = sws_scale(convert_ctx, this->rgbInput->data, this->rgbInput->linesize, 0,
    this->avCodecContext->height, this->yuvOutput->data, this->yuvOutput->linesize);

  sws_freeContext(convert_ctx);

  if (!result)
  {
    vtkGenericWarningMacro(<< "sws_scale() failed");
    return 0;
  }

  int ret = avcodec_send_frame(this->avCodecContext, this->yuvOutput);
  this->yuvOutput->pts++;

  if (ret < 0)
  {
    return 1;
  }

  // run the encoder
  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = nullptr;
  pkt.size = 0;

  while (!ret)
  {
    // dump the compressed result to file
    ret = avcodec_receive_packet(this->avCodecContext, &pkt);
    if (!ret)
    {
      pkt.stream_index = this->avStream->index;
      int wret = av_write_frame(this->avFormatContext, &pkt);
      if (wret < 0)
      {
        vtkGenericWarningMacro(<< "Problem encoding frame.");
        return 0;
      }
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
void vtkFFMPEGWriterInternal::End()
{
  if (this->yuvOutput)
  {
    av_frame_free(&this->yuvOutput);
    this->yuvOutput = nullptr;
  }

  if (this->rgbInput)
  {
    av_frame_free(&this->rgbInput);
    this->rgbInput = nullptr;
  }

  if (this->avFormatContext)
  {
    if (this->openedFile)
    {
      av_write_trailer(this->avFormatContext);
      avio_close(this->avFormatContext->pb);
      this->openedFile = 0;
    }

    avformat_free_context(this->avFormatContext);
    this->avFormatContext = 0;
  }

  if (this->avOutputFormat)
  {
    // Next line was done inside av_free(this->avFormatContext).
    // av_free(this->avOutputFormat);

    this->avOutputFormat = 0;
  }

  if (this->avCodecContext)
  {
    avcodec_close(this->avCodecContext);
    avcodec_free_context(&this->avCodecContext);
    this->avCodecContext = nullptr;
  }

  this->closedFile = 1;
}

// for old versions of ffmpeg use the old API, eventually remove this code
// The new API was introduced around 2016
#else

//---------------------------------------------------------------------------
int vtkFFMPEGWriterInternal::Start()
{
  this->closedFile = 0;

  // initialize libavcodec, and register all codecs and formats
  av_register_all();

  // create the format context that wraps all of the media output structures
  this->avFormatContext = avformat_alloc_context();
  if (!this->avFormatContext)
  {
    vtkGenericWarningMacro(<< "Could not open the format context.");
    return 0;
  }

  // choose avi media file format
  this->avOutputFormat = av_guess_format("avi", nullptr, nullptr);
  if (!this->avOutputFormat)
  {
    vtkGenericWarningMacro(<< "Could not open the avi media file format.");
    return 0;
  }

  if (this->Writer->GetCompression())
  {
    // choose a codec that is easily playable on windows
    this->avOutputFormat->video_codec = AV_CODEC_ID_MJPEG;
  }
  else
  {
    this->avOutputFormat->video_codec = AV_CODEC_ID_RAWVIDEO;
  }

  // assign the format to the context
  this->avFormatContext->oformat = this->avOutputFormat;

  // choose a filename for the output
  strcpy(this->avFormatContext->filename, this->Writer->GetFileName());

  // create a stream for that file
  this->avStream = avformat_new_stream(this->avFormatContext, 0);
  if (!this->avStream)
  {
    vtkGenericWarningMacro(<< "Could not create video stream.");
    return 0;
  }

  // Set up the codec.
  AVCodecContext* c = this->avStream->codec;
  c->codec_id = static_cast<AVCodecID>(this->avOutputFormat->video_codec);
  c->codec_type = AVMEDIA_TYPE_VIDEO;
  c->width = this->Dim[0];
  c->height = this->Dim[1];
  if (this->Writer->GetCompression())
  {
    c->pix_fmt = AV_PIX_FMT_YUVJ422P;
  }
  else
  {
    c->pix_fmt = AV_PIX_FMT_BGR24;
  }

  // to do playback at actual recorded rate, this will need more work see also below
  c->time_base.den = this->FrameRate;
  c->time_base.num = 1;
  // about one full frame per second
  c->gop_size = this->FrameRate;

  if (!this->Writer->GetBitRate())
  {
    // allow a variable quality/size tradeoff
    switch (this->Writer->GetQuality())
    {
      case 0:
        c->bit_rate = 3 * 1024 * 1024;
        break;
      case 1:
        c->bit_rate = 6 * 1024 * 1024;
        break;
      default:
        c->bit_rate = 12 * 1024 * 1024;
        break;
    }
  }
  else
  {
    c->bit_rate = this->Writer->GetBitRate();
  }

  if (!this->Writer->GetBitRateTolerance())
  {
    c->bit_rate_tolerance = c->bit_rate; // ffmpeg won't create a codec if brt<br
  }
  else
  {
    c->bit_rate_tolerance = this->Writer->GetBitRateTolerance();
  }

  // manufacture a codec with the chosen parameters
  AVCodec* codec = avcodec_find_encoder(c->codec_id);
  if (!codec)
  {
    vtkGenericWarningMacro(<< "Codec not found.");
    return 0;
  }
  if (avcodec_open2(c, codec, nullptr) < 0)
  {
    vtkGenericWarningMacro(<< "Could not open codec.");
    return 0;
  }

  // for the output of the writer's input...
  this->rgbInput = av_frame_alloc();
  if (!this->rgbInput)
  {
    vtkGenericWarningMacro(<< "Could not make rgbInput avframe.");
    return 0;
  }
  int RGBsize = avpicture_get_size(AV_PIX_FMT_RGB24, c->width, c->height);
  unsigned char* rgb = (unsigned char*)av_malloc(sizeof(unsigned char) * RGBsize);
  if (!rgb)
  {
    vtkGenericWarningMacro(<< "Could not make rgbInput's buffer.");
    return 0;
  }
  // The rgb buffer should get deleted when this->rgbInput is.
  avpicture_fill((AVPicture*)this->rgbInput, rgb, AV_PIX_FMT_RGB24, c->width, c->height);

  // and for the output to the codec's input.
  this->yuvOutput = av_frame_alloc();
  if (!this->yuvOutput)
  {
    vtkGenericWarningMacro(<< "Could not make yuvOutput avframe.");
    return 0;
  }
  int YUVsize = avpicture_get_size(c->pix_fmt, c->width, c->height);
  unsigned char* yuv = (unsigned char*)av_malloc(sizeof(unsigned char) * YUVsize);
  if (!yuv)
  {
    vtkGenericWarningMacro(<< "Could not make yuvOutput's buffer.");
    return 0;
  }
  // The yuv buffer should get deleted when this->yuv_input is.
  avpicture_fill((AVPicture*)this->yuvOutput, yuv, c->pix_fmt, c->width, c->height);

  // Finally, open the file and start it off.
  if (avio_open(&this->avFormatContext->pb, this->avFormatContext->filename, AVIO_FLAG_WRITE) < 0)
  {
    vtkGenericWarningMacro(<< "Could not open " << this->Writer->GetFileName() << ".");
    return 0;
  }
  this->openedFile = 1;

  if (avformat_write_header(this->avFormatContext, nullptr) < 0)
  {
    vtkGenericWarningMacro(<< "Could not allocate avcodec private data.");
    return 0;
  }
  return 1;
}

//---------------------------------------------------------------------------
int vtkFFMPEGWriterInternal::Write(vtkImageData* id)
{
  this->Writer->GetInputAlgorithm(0, 0)->UpdateWholeExtent();

  AVCodecContext* cc = this->avStream->codec;

  // copy the image from the input to the RGB buffer while flipping Y
  unsigned char* rgb = (unsigned char*)id->GetScalarPointer();
  unsigned char* src;
  for (int y = 0; y < cc->height; y++)
  {
    src = rgb + (cc->height - y - 1) * cc->width * 3; // flip Y
    unsigned char* dest = &this->rgbInput->data[0][y * this->rgbInput->linesize[0]];
    memcpy((void*)dest, (void*)src, cc->width * 3);
  }

  // convert that to YUV for input to the codec
  SwsContext* convert_ctx = sws_getContext(cc->width, cc->height, AV_PIX_FMT_RGB24, cc->width,
    cc->height, cc->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);

  if (convert_ctx == nullptr)
  {
    vtkGenericWarningMacro(<< "swscale context initialization failed");
    return 0;
  }

  int result = sws_scale(convert_ctx, this->rgbInput->data, this->rgbInput->linesize, 0, cc->height,
    this->yuvOutput->data, this->yuvOutput->linesize);

  sws_freeContext(convert_ctx);

  if (!result)
  {
    vtkGenericWarningMacro(<< "sws_scale() failed");
    return 0;
  }

  // run the encoder
  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = nullptr;
  pkt.size = 0;

  int got_frame;
  int ret = avcodec_encode_video2(cc, &pkt, this->yuvOutput, &got_frame);

  // dump the compressed result to file
  if (got_frame)
  {
    pkt.stream_index = this->avStream->index;
    ret = av_write_frame(this->avFormatContext, &pkt);
  }

  if (ret < 0)
  {
    vtkGenericWarningMacro(<< "Problem encoding frame.");
    return 0;
  }

  return 1;
}

//---------------------------------------------------------------------------
void vtkFFMPEGWriterInternal::End()
{
  if (this->yuvOutput)
  {
    av_free(this->yuvOutput->data[0]);
    av_free(this->yuvOutput);
    this->yuvOutput = nullptr;
  }

  if (this->rgbInput)
  {
    av_free(this->rgbInput->data[0]);
    av_free(this->rgbInput);
    this->rgbInput = nullptr;
  }

  if (this->avFormatContext)
  {
    if (this->openedFile)
    {
      av_write_trailer(this->avFormatContext);
      avio_close(this->avFormatContext->pb);
      this->openedFile = 0;
    }

    av_free(this->avFormatContext);
    this->avFormatContext = 0;
  }

  if (this->avStream)
  {
    av_free(this->avStream);
    this->avStream = nullptr;
  }

  if (this->avOutputFormat)
  {
    // Next line was done inside av_free(this->avFormatContext).
    // av_free(this->avOutputFormat);

    this->avOutputFormat = 0;
  }

  this->closedFile = 1;
}

#endif

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFMPEGWriter);

//---------------------------------------------------------------------------
vtkFFMPEGWriter::vtkFFMPEGWriter()
{
  this->Internals = 0;
  this->Quality = 2;
  this->Compression = true;
  this->Rate = 25;
  this->BitRate = 0;
  this->BitRateTolerance = 0;
}

//---------------------------------------------------------------------------
vtkFFMPEGWriter::~vtkFFMPEGWriter()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkFFMPEGWriter::Start()
{
  this->Error = 1;

  if (this->Internals)
  {
    vtkErrorMacro("Movie already started.");
    this->SetErrorCode(vtkGenericMovieWriter::InitError);
    return;
  }
  if (this->GetInput() == nullptr)
  {
    vtkErrorMacro("Please specify an input.");
    this->SetErrorCode(vtkGenericMovieWriter::NoInputError);
    return;
  }
  if (!this->FileName)
  {
    vtkErrorMacro("Please specify a filename.");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return;
  }

  this->Internals = new vtkFFMPEGWriterInternal(this);

  this->Error = 0;

  this->Initialized = 0;
}

//---------------------------------------------------------------------------
void vtkFFMPEGWriter::Write()
{
  if (this->Error)
  {
    return;
  }

  if (!this->Internals)
  {
    vtkErrorMacro("Movie not started.");
    this->Error = 1;
    this->SetErrorCode(vtkGenericMovieWriter::InitError);
    return;
  }

  // get the data
  vtkImageData* input = this->GetImageDataInput(0);
  this->GetInputAlgorithm(0, 0)->UpdateWholeExtent();

  int dim[4];
  input->GetDimensions(dim);
  if (this->Internals->Dim[0] == 0 && this->Internals->Dim[1] == 0)
  {
    this->Internals->Dim[0] = dim[0];
    this->Internals->Dim[1] = dim[1];
  }

  if (this->Internals->Dim[0] != dim[0] || this->Internals->Dim[1] != dim[1])
  {
    vtkErrorMacro("Image not of the same size.");
    this->Error = 1;
    this->SetErrorCode(vtkGenericMovieWriter::ChangedResolutionError);
    return;
  }

  if (!this->Initialized)
  {
    this->Internals->FrameRate = this->Rate;
    if (!this->Internals->Start())
    {
      vtkErrorMacro("Error initializing video stream.");
      this->Error = 1;
      this->SetErrorCode(vtkGenericMovieWriter::InitError);
      return;
    }
    this->Initialized = 1;
  }

  if (!this->Internals->Write(input))
  {
    vtkErrorMacro("Error storing image.");
    this->Error = 1;
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
  }
}

//---------------------------------------------------------------------------
void vtkFFMPEGWriter::End()
{
  this->Internals->End();

  delete this->Internals;
  this->Internals = 0;
}

//---------------------------------------------------------------------------
void vtkFFMPEGWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Quality: " << this->Quality << endl;
  os << indent << "Compression: " << (this->Compression ? "true" : "false") << endl;
  os << indent << "Rate: " << this->Rate << endl;
  os << indent << "BitRate: " << this->BitRate << endl;
  os << indent << "BitRateTolerance: " << this->BitRateTolerance << endl;
}
