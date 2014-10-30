/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOggTheoraWriter.cxx

  Copyright (c) Michael Wild, Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOggTheoraWriter.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkErrorCode.h"

#include "vtk_oggtheora.h"

#include <ctime>

//---------------------------------------------------------------------------
class vtkOggTheoraWriterInternal
{
public:
  vtkOggTheoraWriterInternal(vtkOggTheoraWriter *creator);
  ~vtkOggTheoraWriterInternal();

  int Start();
  int Write(vtkImageData *id);
  void End();

  int Dim[2];
  int FrameRate;

private:

  // Helper function to convert an RGB image into the Y'CbCr color space and
  // into the data structure required by theora (i.e. 4:4:4 or 4:2:0
  // subsampling will be used). Refer to http://www.theora.org/doc/Theora.pdf
  // sections 4.3 and 4.3.2. Actually, the equations are inverted. However, I'm
  // not sure whether VTK uses gamma-corrected RGB or not. I assume they are
  // not, which is what we need here. Assume that the width and height are even
  // numbers.
  void RGB2YCbCr(vtkImageData *id, th_ycbcr_buffer ycbcr);
  // Write the ogg/theora header information
  int WriteHeader();
  // Encode a single frame
  int EncodeFrame(th_ycbcr_buffer ycbcr, int lastFrame);

  vtkOggTheoraWriter *Writer;

  size_t           Off[2];         // offsets of the picture within the frame
  th_enc_ctx      *thEncContext;   // the theora context (has to be freed)
  th_ycbcr_buffer  thImage;        // the Y'CbCr image buffer
  ogg_stream_state oggState;       // the ogg stream state (has to be cleared)
  FILE*            outFile;        // the output file stream
  bool             haveImageData;  // indicater whether a frame has to be encoded
                                   // (for the leap-frogging)

  int openedFile;
  int closedFile;
};

//---------------------------------------------------------------------------
vtkOggTheoraWriterInternal::vtkOggTheoraWriterInternal(vtkOggTheoraWriter *creator)
{
  this->Writer = creator;
  this->Dim[0] = 0;
  this->Dim[1] = 0;

  this->Off[0] = 0;
  this->Off[1] = 0;
  this->thEncContext = NULL;
  this->outFile      = NULL;
  this->thImage[0].data = NULL;
  this->thImage[1].data = NULL;
  this->thImage[2].data = NULL;

  this->openedFile = 0;
  this->closedFile = 1;
  this->haveImageData = false;

  this->FrameRate = 25;
}

//---------------------------------------------------------------------------
vtkOggTheoraWriterInternal::~vtkOggTheoraWriterInternal()
{
  if (!this->closedFile)
    {
    this->End();
    }
}

//---------------------------------------------------------------------------
int vtkOggTheoraWriterInternal::Start()
{
  this->closedFile = 0;

  // ogg information
  srand(time(NULL));
  if (ogg_stream_init(&this->oggState,rand())!=0)
    {
    vtkGenericWarningMacro("Could not initialize ogg stream state.");
    return 0;
    }

  // fill in theora information
  th_info thInfo;
  th_info_init(&thInfo);
  // frame_width and frame_height must be multiples of 16
  thInfo.frame_width = (this->Dim[0]+15)&~0xF;
  thInfo.frame_height = (this->Dim[1]+15)&~0xF;
  thInfo.pic_width = this->Dim[0];
  thInfo.pic_height = this->Dim[1];
  // force even offsets of the picture within the frame
  this->Off[0] = (thInfo.frame_width-this->Dim[0])>>1&~1;
  this->Off[1] = (thInfo.frame_height-this->Dim[1])>>1&~1;
  thInfo.pic_x = static_cast<ogg_uint32_t>(this->Off[0]);
  thInfo.pic_y = static_cast<ogg_uint32_t>(this->Off[1]);
  thInfo.colorspace = TH_CS_ITU_REC_470BG;
  if (this->Writer->GetSubsampling())
    {
    // 4:2:0 subsampling is the only implemented option in libtheora-1.0
    thInfo.pixel_fmt = TH_PF_420;
    }
  else
    {
    thInfo.pixel_fmt = TH_PF_444;
    }
  thInfo.target_bitrate = 0; // variable bitrate recording (default)
  // allow a variable quality/size tradeoff
  //! \todo still have to find appropriate quality parameters though...
  //! valid values are in [0,63].
  switch (this->Writer->GetQuality())
    {
    case 0:
      thInfo.quality = 42;
      break;
    case 1:
      thInfo.quality = 52;
      break;
    default:
      thInfo.quality = 63;
      break;
    }
  thInfo.keyframe_granule_shift = 6; // default value
  // the frame rate (as a fraction)
  thInfo.fps_numerator = this->FrameRate;
  thInfo.fps_denominator = 1;
  // pixel ascpect ratio
  thInfo.aspect_numerator = 1;
  thInfo.aspect_denominator = 1;

  // create the theora encoder context
  this->thEncContext = th_encode_alloc(&thInfo);
  if (!this->thEncContext)
    {
    vtkGenericWarningMacro(<< "Could not allocate the theora context.");
    return 0;
    }

  // create the theora buffer (do not cheat with the frame padding,
  // allocate the whole thing!)
  for (size_t i=0; i<3; ++i)
    {
    this->thImage[i].width  = thInfo.frame_width;
    this->thImage[i].height = thInfo.frame_height;
    if (this->Writer->GetSubsampling() && i>0)
      {
      // Chroma planes are subsampled by a factor of 2
      this->thImage[i].width  /= 2;
      this->thImage[i].height /= 2;
      }
    // the stride is in bytes
    this->thImage[i].stride = this->thImage[i].width*sizeof(unsigned char);
    // make sure there's nothing left laying around...
    delete[] this->thImage[i].data;
    // allocate the image plane
    size_t siz = this->thImage[i].width * this->thImage[i].height;
    this->thImage[i].data   = new unsigned char[siz];
    }

  // thInfo is no longer needed
  th_info_clear(&thInfo);

  // Finally, open the file and start it off.
  this->outFile = fopen(this->Writer->GetFileName(),"wb");
  if (!this->outFile)
    {
    vtkGenericWarningMacro(<< "Could not open " << this->Writer->GetFileName() << "." );
    return 0;
    }
  this->openedFile = 1;

  return this->WriteHeader();

}

//---------------------------------------------------------------------------
// ripped from libtheora-1.0/examples/encoder_example.c
int vtkOggTheoraWriterInternal::WriteHeader()
{
  th_comment       thComment;
  ogg_packet       oggPacket;
  ogg_page         oggPage;

  th_comment_init(&thComment);

  // first packet will get its own page automatically
  if (th_encode_flushheader(this->thEncContext,&thComment,&oggPacket)<=0)
    {
    vtkGenericWarningMacro("Internal Theora library error.");
    return 0;
    }
  ogg_stream_packetin(&this->oggState,&oggPacket);
  if (ogg_stream_pageout(&this->oggState,&oggPage)!=1)
    {
    vtkGenericWarningMacro("Internal Theora library error.");
    return 0;
    }
  fwrite(oggPage.header,1,oggPage.header_len,this->outFile);
  fwrite(oggPage.body,1,oggPage.body_len,this->outFile);
  // remaining theora headers
  int ret;
  while (true)
    {
    ret=th_encode_flushheader(this->thEncContext,&thComment,&oggPacket);
    if (ret<0)
      {
      vtkGenericWarningMacro("Internal Theora library error.");
      return 0;
      }
    else if (!ret)
      break;
    ogg_stream_packetin(&this->oggState,&oggPacket);
    }
  // Flush the rest of our headers. This ensures
  // the actual data in each stream will start
  // on a new page, as per spec.
  while (true)
    {
    ret = ogg_stream_flush(&this->oggState,&oggPage);
    if (ret<0)
      {
      vtkGenericWarningMacro("Internal Theora library error.");
      return 0;
      }
    if (ret==0)break;
    fwrite(oggPage.header,1,oggPage.header_len,this->outFile);
    fwrite(oggPage.body,1,oggPage.body_len,this->outFile);
  }

  th_comment_clear(&thComment);

  return 1;
}

//---------------------------------------------------------------------------
int vtkOggTheoraWriterInternal::Write(vtkImageData *id)
{
  // encode the frame from the last call.
  // have to do leap-frogging, because otherwise we can't
  // write the EOS page with the last frame in End().
  int ret = 1;
  if (this->haveImageData)
    {
    ret = this->EncodeFrame(this->thImage,0);
    this->haveImageData = false;
    }

  this->Writer->GetInputAlgorithm(0, 0)->UpdateWholeExtent();

  // convert current RGB int YCbCr color space
  this->RGB2YCbCr(id,this->thImage);
  this->haveImageData = true;

  return ret;
}

//---------------------------------------------------------------------------
// ripped from libtheora-1.0/examples/encoder_example.c
int vtkOggTheoraWriterInternal::EncodeFrame(th_ycbcr_buffer, int lastFrame)
{
  if (th_encode_ycbcr_in(this->thEncContext,this->thImage)<0)
    {
    vtkGenericWarningMacro("Error encoding frame.");
    return 0;
    }
  // retrieve and push packets, writing pages as required
  ogg_packet oggPacket;
  ogg_page   oggPage;
  int ret;
  while ((ret=th_encode_packetout(this->thEncContext,lastFrame,&oggPacket)))
    {
    if(ret<0)
      {
      vtkGenericWarningMacro("Error retrieving packet from codec.");
      return 0;
      }
    if (ogg_stream_packetin(&this->oggState,&oggPacket)<0)
      {
      vtkGenericWarningMacro("Error inserting packet into stream.");
      return 0;
      }
    while (ogg_stream_pageout(&this->oggState,&oggPage))
      {
      fwrite(oggPage.header,1,oggPage.header_len,this->outFile);
      fwrite(oggPage.body,1,oggPage.body_len,this->outFile);
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkOggTheoraWriterInternal::End()
{
  // flush remaining frame
  if (this->haveImageData)
    if (!this->EncodeFrame(this->thImage,1))
      vtkGenericWarningMacro("Failed to finish writing movie");
  this->haveImageData = false;

  // clean up
  for (size_t i = 0; i < 3; ++i)
    {
    delete[] this->thImage[i].data;
    this->thImage[i].data = NULL;
    }

  if (this->thEncContext)
    {
    th_encode_free(this->thEncContext);
    this->thEncContext = NULL;
    }

  ogg_stream_clear(&this->oggState);

  if (this->openedFile)
    {
    fclose(this->outFile);
    this->openedFile = 0;
    }
  this->closedFile = 1;
}

//---------------------------------------------------------------------------
void vtkOggTheoraWriterInternal::RGB2YCbCr(vtkImageData *id,
                                           th_ycbcr_buffer ycbcr)
{
  // convenience
  typedef unsigned char uchar;

  //
  // constant coefficiens
  //

  static const uchar OffY = 16, OffCr = 128, OffCb = 128;
  // divide by 255, because the formulas use normalized RGB, i.e in [0,1]
  static const double ExcurY = 219.0/255, ExcurCr = 224.0/255, ExcurCb = 224.0/255,
                      Kr = 0.299, Kb = 0.114;
  // derived constants
  static const double Kg = 1 - Kr - Kb,
                      Krm1 = Kr - 1,
                      Kbm1 = Kb - 1;
  // stride between rows in the YCbCr image planes, since
  // pixels in a row are contiguous, but rows need not be
  static const int strideRGB = this->Dim[0]*3,
                   strideY   = ycbcr[0].stride/sizeof(uchar), // th_image_plane strides are in bytes
                   strideCb  = ycbcr[1].stride/sizeof(uchar),
                   strideCr  = ycbcr[2].stride/sizeof(uchar);
  //
  // computation
  //

  // the first pixel in the RGB image
  uchar *rgbStart = (uchar*)id->GetScalarPointer();
  // pointers to iterate through the RGB image an the Y, Cb and Cr planes
  uchar *rgb, *Y, *Cb=0, *Cr=0;
  // indicators whether we have to handle chroma planes.
  bool isXCPlane = false,
       isYCPlane = true; // y-flipping
  // loop over rows
  size_t x, y, yC;
  for (y = 0; y < static_cast<size_t>(this->Dim[1]); ++y)
    {
    if (this->Writer->GetSubsampling())
      {
      // reset x indicator and flip y indicator
      isXCPlane = false;
      isYCPlane = !isYCPlane;
      }
    // compute pointers to the first pixel in row y,
    // flipping y coordinate
    rgb = rgbStart + (this->Dim[1]-y-1) * strideRGB;
    Y  = ycbcr[0].data + (y+this->Off[1]) * strideY + this->Off[0];
    if (!this->Writer->GetSubsampling())
      {
      Cb = ycbcr[1].data + (y+this->Off[1]) * strideY + this->Off[0];
      Cr = ycbcr[2].data + (y+this->Off[1]) * strideY + this->Off[0];
      }
    else if (isYCPlane)
      {
        // compute y on chroma planes
        yC = (y+this->Off[1])/2;
        // pointers to first pixel in row yC of chroma planes
        Cb = ycbcr[1].data + yC * strideCb + this->Off[0]/2;
        Cr = ycbcr[2].data + yC * strideCr + this->Off[0]/2;
      }
    // loop over columns in row y
    for (x = 0; x < static_cast<size_t>(this->Dim[0]); ++x)
      {
      // do the actual transformation
      *Y  = uchar((Kr * rgb[0] + Kg * rgb[1] + Kb *rgb[2]) *ExcurY  + OffY);
      if (!this->Writer->GetSubsampling())
        {
        *Cb = uchar((Kr   * rgb[0] + Kg * rgb[1] + Kbm1 * rgb[2]) /
            (2 * Kbm1) * ExcurCb + OffCb);
        *Cr = uchar((Krm1 * rgb[0] + Kg * rgb[1] + Kb   * rgb[2]) /
            (2 * Krm1) * ExcurCr + OffCr);
        }
      else
        {
        // flip indicator
        isXCPlane = !isXCPlane;
        if (isYCPlane && isXCPlane)
          {
          /* REMARK: actually, interpolation seems to give worse results...
           * just use the associated RGB pixel (a.k.a nearest neighbor).
           */
#if 0
          // interpolate surrounding rgb (subsampling)
          // use double in order to not lose too much precision...
          double irgb[3];
          for (size_t i = 0; i < 3; ++i)
            {
            irgb[i] = 0.25 * (rgb[i]           + rgb[i+3] +
                              rgb[i+strideRGB] + rgb[i+3+strideRGB]);
            }
          *Cb = uchar((Kr   * irgb[0] + Kg * irgb[1] + Kbm1 * irgb[2]) /
              (2 * Kbm1) * ExcurCb) + OffCb;
          *Cr = uchar((Krm1 * irgb[0] + Kg * irgb[1] + Kb   * irgb[2]) /
              (2 * Krm1) * ExcurCr) + OffCr;
#else
          *Cb = uchar((Kr   * rgb[0] + Kg * rgb[1] + Kbm1 * rgb[2]) /
              (2 * Kbm1) * ExcurCb + OffCb);
          *Cr = uchar((Krm1 * rgb[0] + Kg * rgb[1] + Kb   * rgb[2]) /
              (2 * Krm1) * ExcurCr + OffCr);
#endif
          }
        }
      // advance to next pixel in row y
      rgb += 3;
      ++Y;
      if (!this->Writer->GetSubsampling())
        {
        ++Cb;
        ++Cr;
        }
      else if (isYCPlane && isXCPlane)
        {
        ++Cb;
        ++Cr;
        }
      }
    }
}

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkOggTheoraWriter);

//---------------------------------------------------------------------------
vtkOggTheoraWriter::vtkOggTheoraWriter()
{
  this->Internals = 0;
  this->Quality = 2;
  this->Rate = 25;
  this->Subsampling = 0;
}

//---------------------------------------------------------------------------
vtkOggTheoraWriter::~vtkOggTheoraWriter()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkOggTheoraWriter::Start()
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

  this->Internals = new vtkOggTheoraWriterInternal(this);

  this->Error = 0;

  this->Initialized = 0;
}

//---------------------------------------------------------------------------
void vtkOggTheoraWriter::Write()
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
    this->SetErrorCode(vtkErrorCode::UnknownError);
    }
}

//---------------------------------------------------------------------------
void vtkOggTheoraWriter::End()
{
  this->Internals->End();

  delete this->Internals;
  this->Internals = 0;
}

//---------------------------------------------------------------------------
void vtkOggTheoraWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Quality: " << this->Quality << endl;
  os << indent << "Rate: " << this->Rate << endl;
  os << indent << "Subsampling: " << this->Subsampling << endl;
}
