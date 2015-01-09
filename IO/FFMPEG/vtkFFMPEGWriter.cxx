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

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkErrorCode.h"
#include "vtkFFMPEGConfig.h"

extern "C" {
#ifdef VTK_FFMPEG_HAS_OLD_HEADER
# include <ffmpeg/avformat.h>
#else
# include <libavformat/avformat.h>
#endif

#ifndef VTK_FFMPEG_HAS_IMG_CONVERT
# ifdef VTK_FFMPEG_HAS_OLD_HEADER
#  include <ffmpeg/swscale.h>
# else
#  include <libswscale/swscale.h>
# endif
#endif
}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#if LIBAVCODEC_VERSION_MAJOR < 55
# define AV_CODEC_ID_MJPEG CODEC_ID_MJPEG
# define AV_CODEC_ID_RAWVIDEO CODEC_ID_RAWVIDEO
#endif

//---------------------------------------------------------------------------
class vtkFFMPEGWriterInternal
{
public:
  vtkFFMPEGWriterInternal(vtkFFMPEGWriter *creator);
  ~vtkFFMPEGWriterInternal();

  int Start();
  int Write(vtkImageData *id);
  void End();

  int Dim[2];
  int FrameRate;

private:

  vtkFFMPEGWriter *Writer;

  AVFormatContext *avFormatContext;

  AVOutputFormat *avOutputFormat;

  AVStream *avStream;

#if LIBAVFORMAT_VERSION_MAJOR < 54
  unsigned char *codecBuf;
  int codecBufSize;
#endif

  AVFrame *rgbInput;
  AVFrame *yuvOutput;

  int openedFile;
  int closedFile;
};

//---------------------------------------------------------------------------
vtkFFMPEGWriterInternal::vtkFFMPEGWriterInternal(vtkFFMPEGWriter *creator)
{
  this->Writer = creator;
  this->Dim[0] = 0;
  this->Dim[1] = 0;

  this->avFormatContext = NULL;

  this->avOutputFormat = NULL;

  this->avStream = NULL;

#if LIBAVFORMAT_VERSION_MAJOR < 54
  this->codecBuf = NULL;
#endif
  this->rgbInput = NULL;
  this->yuvOutput = NULL;

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

//---------------------------------------------------------------------------
int vtkFFMPEGWriterInternal::Start()
{
  this->closedFile = 0;

  //initialize libavcodec, and register all codecs and formats
  av_register_all();

  //create the format context that wraps all of the media output structures
#if LIBAVFORMAT_VERSION_MAJOR >= 52
  this->avFormatContext = avformat_alloc_context();
#else
  this->avFormatContext = av_alloc_format_context();
#endif
  if (!this->avFormatContext)
    {
    vtkGenericWarningMacro (<< "Coult not open the format context.");
    return 0;
    }

  //choose avi media file format
#ifdef VTK_FFMPEG_HAS_OLD_HEADER
  this->avOutputFormat = guess_format("avi", NULL, NULL);
#else
  this->avOutputFormat = av_guess_format("avi", NULL, NULL);
#endif
  if (!this->avOutputFormat)
    {
    vtkGenericWarningMacro (<< "Could not open the avi media file format.");
    return 0;
    }

  if (this->Writer->GetCompression())
    {
    //choose a codec that is easily playable on windows
    this->avOutputFormat->video_codec = AV_CODEC_ID_MJPEG;
    }
  else
    {
    this->avOutputFormat->video_codec = AV_CODEC_ID_RAWVIDEO;
    }

  //assign the format to the context
  this->avFormatContext->oformat = this->avOutputFormat;

  //choose a filename for the output
  strcpy(this->avFormatContext->filename, this->Writer->GetFileName());

  //create a stream for that file
#if LIBAVFORMAT_VERSION_MAJOR < 54
  this->avStream = av_new_stream(this->avFormatContext, 0);
#else
  this->avStream = avformat_new_stream(this->avFormatContext, 0);
#endif
  if (!this->avStream)
    {
    vtkGenericWarningMacro (<< "Could not create video stream.");
    return 0;
    }

  //Set up the codec.
  AVCodecContext *c = this->avStream->codec;
#ifdef VTK_FFMPEG_AVCODECID
  c->codec_id = static_cast<AVCodecID>(this->avOutputFormat->video_codec);
#else
  c->codec_id = static_cast<CodecID>(this->avOutputFormat->video_codec);
#endif
#ifdef VTK_FFMPEG_HAS_OLD_HEADER
  c->codec_type = CODEC_TYPE_VIDEO;
#else
 c->codec_type = AVMEDIA_TYPE_VIDEO;
#endif
  c->width = this->Dim[0];
  c->height = this->Dim[1];
  if (this->Writer->GetCompression())
    {
    c->pix_fmt = PIX_FMT_YUVJ422P;
    }
  else
    {
    c->pix_fmt = PIX_FMT_BGR24;
    }

  //to do playback at actual recorded rate, this will need more work see also below
  c->time_base.den = this->FrameRate;
  c->time_base.num = 1;
  //about one full frame per second
  c->gop_size = this->FrameRate;

  if( !this->Writer->GetBitRate() )
    {
    //allow a variable quality/size tradeoff
    switch (this->Writer->GetQuality())
      {
      case 0:
        c->bit_rate = 3*1024*1024;
        break;
      case 1:
        c->bit_rate = 6*1024*1024;
        break;
      default:
        c->bit_rate = 12*1024*1024;
        break;
      }
    }
  else
    {
    c->bit_rate = this->Writer->GetBitRate();
    }

  if(!this->Writer->GetBitRateTolerance())
    {
    c->bit_rate_tolerance = c->bit_rate; //ffmpeg won't create a codec if brt<br
    }
  else
    {
    c->bit_rate_tolerance = this->Writer->GetBitRateTolerance();
    }

#if LIBAVFORMAT_VERSION_MAJOR < 54
  //apply the chosen parameters
  if (av_set_parameters(this->avFormatContext, NULL) < 0)
    {
    vtkGenericWarningMacro (<< "Invalid output format parameters." );
    return 0;
    }
#endif

  //manufacture a codec with the chosen parameters
  AVCodec *codec = avcodec_find_encoder(c->codec_id);
  if (!codec)
    {
    vtkGenericWarningMacro (<< "Codec not found." );
    return 0;
    }
#if LIBAVFORMAT_VERSION_MAJOR < 54
  if (avcodec_open(c, codec) < 0)
#else
  if (avcodec_open2(c, codec, NULL) < 0)
#endif
    {
    vtkGenericWarningMacro (<< "Could not open codec.");
    return 0;
    }

  //create buffers for the codec to work with.

#if LIBAVFORMAT_VERSION_MAJOR < 54
  //working compression space
  this->codecBufSize = 2*c->width*c->height*4; //hopefully this is enough
  this->codecBuf = new unsigned char[this->codecBufSize];
  if (!this->codecBuf)
    {
    vtkGenericWarningMacro (<< "Could not make codec working space." );
    return 0;
    }
#endif

  //for the output of the writer's input...
  this->rgbInput = avcodec_alloc_frame();
  if (!this->rgbInput)
    {
    vtkGenericWarningMacro (<< "Could not make rgbInput avframe." );
    return 0;
    }
  int RGBsize = avpicture_get_size(PIX_FMT_RGB24, c->width, c->height);
  unsigned char *rgb = (unsigned char *)av_malloc(sizeof(unsigned char) * RGBsize);
  if (!rgb)
    {
    vtkGenericWarningMacro (<< "Could not make rgbInput's buffer." );
    return 0;
    }
  //The rgb buffer should get deleted when this->rgbInput is.
  avpicture_fill((AVPicture *)this->rgbInput, rgb, PIX_FMT_RGB24, c->width, c->height);

  //and for the output to the codec's input.
  this->yuvOutput = avcodec_alloc_frame();
  if (!this->yuvOutput)
    {
    vtkGenericWarningMacro (<< "Could not make yuvOutput avframe." );
    return 0;
    }
  int YUVsize = avpicture_get_size(c->pix_fmt, c->width, c->height);
  unsigned char *yuv = (unsigned char *)av_malloc(sizeof(unsigned char) * YUVsize);
  if (!yuv)
    {
    vtkGenericWarningMacro (<< "Could not make yuvOutput's buffer." );
    return 0;
    }
  //The yuv buffer should get deleted when this->yuv_input is.
  avpicture_fill((AVPicture *)this->yuvOutput, yuv, c->pix_fmt, c->width, c->height);


  //Finally, open the file and start it off.
#if LIBAVFORMAT_VERSION_MAJOR < 54
  if (url_fopen(&this->avFormatContext->pb, this->avFormatContext->filename, URL_WRONLY) < 0)
#else
  if (avio_open(&this->avFormatContext->pb, this->avFormatContext->filename, AVIO_FLAG_WRITE) < 0)
#endif
    {
    vtkGenericWarningMacro (<< "Could not open " << this->Writer->GetFileName() << "." );
    return 0;
    }
  this->openedFile = 1;

#if LIBAVFORMAT_VERSION_MAJOR < 54
  av_write_header(this->avFormatContext);
#else
  avformat_write_header(this->avFormatContext, NULL);
#endif
  return 1;
}

//---------------------------------------------------------------------------
int vtkFFMPEGWriterInternal::Write(vtkImageData *id)
{
  this->Writer->GetInputAlgorithm(0, 0)->UpdateWholeExtent();

  AVCodecContext *cc = this->avStream->codec;

  //copy the image from the input to the RGB buffer while flipping Y
  unsigned char *rgb = (unsigned char*)id->GetScalarPointer();
  unsigned char *src;
  for (int y = 0; y < cc->height; y++)
    {
    src = rgb + (cc->height-y-1) * cc->width * 3; //flip Y
    unsigned char *dest =
      &this->rgbInput->data[0][y*this->rgbInput->linesize[0]];
    memcpy((void*)dest, (void*)src, cc->width*3);
    }

  //convert that to YUV for input to the codec
#ifdef VTK_FFMPEG_HAS_IMG_CONVERT
  img_convert((AVPicture *)this->yuvOutput, cc->pix_fmt,
              (AVPicture *)this->rgbInput, PIX_FMT_RGB24,
              cc->width, cc->height);
#else
  //convert that to YUV for input to the codec
  SwsContext* convert_ctx = sws_getContext(
    cc->width, cc->height, PIX_FMT_RGB24,
    cc->width, cc->height, cc->pix_fmt,
    SWS_BICUBIC, NULL, NULL, NULL);

  if(convert_ctx == NULL)
    {
    vtkGenericWarningMacro(<< "swscale context initialization failed");
    return 0;
    }

  int result = sws_scale(convert_ctx,
    this->rgbInput->data, this->rgbInput->linesize,
    0, cc->height,
    this->yuvOutput->data, this->yuvOutput->linesize
    );

  sws_freeContext(convert_ctx);

  if(!result)
    {
    vtkGenericWarningMacro(<< "sws_scale() failed");
    return 0;
    }
#endif

  //run the encoder
  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = NULL;
  pkt.size = 0;

#if LIBAVFORMAT_VERSION_MAJOR < 54
  int toAdd = avcodec_encode_video(cc,
                                   this->codecBuf,
                                   this->codecBufSize,
                                   this->yuvOutput);
  if (toAdd)
    {
    //to do playback at actual recorded rate, this will need more work
    pkt.pts = cc->coded_frame->pts;
    //pkt.dts = ?; not dure what decompression time stamp should be
    pkt.data = this->codecBuf;
    pkt.size = toAdd;
    pkt.stream_index = this->avStream->index;
    if (cc->coded_frame->key_frame) //treat keyframes well
      {
#ifdef VTK_FFMPEG_HAS_OLD_HEADER
      pkt.flags |= PKT_FLAG_KEY;
#else
      pkt.flags |= AV_PKT_FLAG_KEY;
#endif
      }
    pkt.duration = 0; //presentation duration in time_base units or 0 if NA
    pkt.pos = -1; //byte position in stream or -1 if NA

    toAdd = av_write_frame(this->avFormatContext, &pkt);
    }
  if (toAdd) //should not have anything left over
    {
    vtkGenericWarningMacro (<< "Problem encoding frame." );
    return 0;
    }

#else
  int got_frame;
  int ret = avcodec_encode_video2(cc,
                                  &pkt,
                                  this->yuvOutput,
                                  &got_frame);

  //dump the compressed result to file
  if (got_frame)
    {
    pkt.stream_index = this->avStream->index;
    ret = av_write_frame(this->avFormatContext, &pkt);
    }

  if (ret<0)
    {
    vtkGenericWarningMacro (<< "Problem encoding frame." );
    return 0;
    }
#endif

  return 1;
}

//---------------------------------------------------------------------------
void vtkFFMPEGWriterInternal::End()
{
  if (this->yuvOutput)
    {
    av_free(this->yuvOutput->data[0]);
    av_free(this->yuvOutput);
    this->yuvOutput = NULL;
    }

  if (this->rgbInput)
    {
    av_free(this->rgbInput->data[0]);
    av_free(this->rgbInput);
    this->rgbInput = NULL;
    }


#if LIBAVFORMAT_VERSION_MAJOR < 54
  if (this->codecBuf)
    {
    av_free(this->codecBuf);
    this->codecBuf = NULL;
    }
#endif

  if (this->avFormatContext)
    {
    if (this->openedFile)
      {
      av_write_trailer(this->avFormatContext);
#if LIBAVFORMAT_VERSION_MAJOR < 54
      url_fclose(this->avFormatContext->pb);
#else
      avio_close(this->avFormatContext->pb);
#endif
      this->openedFile = 0;
      }

    av_free(this->avFormatContext);
    this->avFormatContext = 0;
    }

  if (this->avStream)
    {
    av_free(this->avStream);
    this->avStream = NULL;
    }

  if (this->avOutputFormat)
    {
    //Next line was done inside av_free(this->avFormatContext).
    //av_free(this->avOutputFormat);

    this->avOutputFormat = 0;
    }

  this->closedFile = 1;
}


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

  if ( this->Internals )
    {
    vtkErrorMacro("Movie already started.");
    this->SetErrorCode(vtkGenericMovieWriter::InitError);
    return;
    }
  if ( this->GetInput() == NULL )
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

  if ( !this->Internals )
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
  if ( this->Internals->Dim[0] == 0 && this->Internals->Dim[1] == 0 )
    {
    this->Internals->Dim[0] = dim[0];
    this->Internals->Dim[1] = dim[1];
    }

  if (this->Internals->Dim[0]!= dim[0] || this->Internals->Dim[1]!= dim[1])
    {
    vtkErrorMacro("Image not of the same size.");
    this->Error = 1;
    this->SetErrorCode(vtkGenericMovieWriter::ChangedResolutionError);
    return;
    }

  if ( !this->Initialized )
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
  os << indent << "Compression: " << (this->Compression?"true":"false") << endl;
  os << indent << "Rate: " << this->Rate << endl;
  os << indent << "BitRate: " << this->BitRate << endl;
  os << indent << "BitRateTolerance: " << this->BitRateTolerance << endl;
}
