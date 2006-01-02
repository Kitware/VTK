/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTIFFReader.h"

#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkObjectFactory.h"

#include <sys/stat.h>

extern "C" {
#include "vtk_tiff.h"
}

//-------------------------------------------------------------------------
vtkStandardNewMacro(vtkTIFFReader);
vtkCxxRevisionMacro(vtkTIFFReader, "1.52");

class vtkTIFFReaderInternal
{
public:
  vtkTIFFReaderInternal();
  int Initialize();
  void Clean();
  int CanRead();
  int Open( const char *filename );
  TIFF *Image;
  unsigned int Width;
  unsigned int Height;
  unsigned short SamplesPerPixel;
  unsigned short Compression;
  unsigned short BitsPerSample;
  unsigned short Photometrics;
  unsigned short PlanarConfig;
  unsigned long int TileDepth;
  static void ErrorHandler(const char* module, const char* fmt, va_list ap);
};


extern "C" {
void vtkTIFFReaderInternalErrorHandler(const char* vtkNotUsed(module),
                                          const char* vtkNotUsed(fmt),
                                          va_list vtkNotUsed(ap))
{
    // Do nothing
    // Ignore errors
}
}

//-------------------------------------------------------------------------
int vtkTIFFReaderInternal::Open( const char *filename )
{
  this->Clean();
  struct stat fs;
  if ( stat(filename, &fs) )
    {
    return 0;
    }
  this->Image = TIFFOpen(filename, "r");
  if ( !this->Image)
    {
    this->Clean();
    return 0;
    }
  if ( !this->Initialize() )
    {
    this->Clean();
    return 0;
    }
  return 1;
}

//-------------------------------------------------------------------------
void vtkTIFFReaderInternal::Clean()
{
  if ( this->Image ) 
    {
    TIFFClose(this->Image);
    }
  this->Image=NULL;
  this->Width = 0;
  this->Height = 0;
  this->SamplesPerPixel = 0;
  this->Compression = 0;
  this->BitsPerSample = 0;
  this->Photometrics = 0;
  this->PlanarConfig = 0;
  this->TileDepth = 0;
}

//-------------------------------------------------------------------------
vtkTIFFReaderInternal::vtkTIFFReaderInternal()
{
  this->Image           = NULL;
  TIFFSetErrorHandler(&vtkTIFFReaderInternalErrorHandler);
  TIFFSetWarningHandler(&vtkTIFFReaderInternalErrorHandler);
  this->Clean();
}

//-------------------------------------------------------------------------
int vtkTIFFReaderInternal::Initialize()
{
  if ( this->Image ) 
    {
    if ( !TIFFGetField(this->Image, TIFFTAG_IMAGEWIDTH, &this->Width) ||
         !TIFFGetField(this->Image, TIFFTAG_IMAGELENGTH, &this->Height) )
      {
      return 0;
      }
    TIFFGetField(this->Image, TIFFTAG_SAMPLESPERPIXEL,
                 &this->SamplesPerPixel);
    TIFFGetField(this->Image, TIFFTAG_COMPRESSION, &this->Compression);
    TIFFGetField(this->Image, TIFFTAG_BITSPERSAMPLE,
                 &this->BitsPerSample);
    TIFFGetField(this->Image, TIFFTAG_PHOTOMETRIC, &this->Photometrics);
    TIFFGetField(this->Image, TIFFTAG_PLANARCONFIG, &this->PlanarConfig);
    if ( !TIFFGetField(this->Image, TIFFTAG_TILEDEPTH, &this->TileDepth) ) 
      {
      this->TileDepth = 0;
      }
    }
  return 1;
}

//-------------------------------------------------------------------------
int vtkTIFFReaderInternal::CanRead()
{
  return ( this->Image && ( this->Width > 0 ) && ( this->Height > 0 ) &&
           ( this->SamplesPerPixel > 0 ) &&
           ( this->Compression == COMPRESSION_NONE ) &&
           ( this->Photometrics == PHOTOMETRIC_RGB ||
             this->Photometrics == PHOTOMETRIC_MINISWHITE ||
             this->Photometrics == PHOTOMETRIC_MINISBLACK ||
             this->Photometrics == PHOTOMETRIC_PALETTE ) &&
           this->PlanarConfig == PLANARCONFIG_CONTIG &&
           ( !this->TileDepth ) &&
           ( this->BitsPerSample == 8 ) || this->BitsPerSample == 32 );
}

//-------------------------------------------------------------------------
vtkTIFFReader::vtkTIFFReader()
{
  this->InitializeColors();
  this->InternalImage = new vtkTIFFReaderInternal;
  this->InternalExtents = 0;
}

//-------------------------------------------------------------------------
vtkTIFFReader::~vtkTIFFReader()
{
  delete this->InternalImage;
}

//-------------------------------------------------------------------------
void vtkTIFFReader::ExecuteInformation()
{
  this->InitializeColors();
  this->ComputeInternalFileName(this->DataExtent[4]);
  if (this->InternalFileName == NULL)
    {
    return;
    }

  if ( !this->InternalImage->Open(this->InternalFileName) )
    {
    vtkErrorMacro("Unable to open file " << this->InternalFileName );
    this->DataExtent[0] = 0;
    this->DataExtent[1] = 0;
    this->DataExtent[2] = 0;
    this->DataExtent[3] = 0;
    this->DataExtent[4] = 0;
    this->DataExtent[5] = 0;
    this->SetNumberOfScalarComponents(1);
    this->vtkImageReader2::ExecuteInformation();
    return;
    }

  // pull out the width/height, etc.
  this->DataExtent[0] = 0;
  this->DataExtent[1] = this->GetInternalImage()->Width - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = this->GetInternalImage()->Height - 1;

  switch (this->GetInternalImage()->BitsPerSample)
    {
    case 32:
      this->SetDataScalarTypeToFloat();
      break;
    default:
      this->SetDataScalarTypeToUnsignedChar();
    }

  switch ( this->GetFormat() )
    {
    case vtkTIFFReader::GRAYSCALE:
    case vtkTIFFReader::PALETTE_GRAYSCALE:
      this->SetNumberOfScalarComponents( 1 );
      break;
    case vtkTIFFReader::RGB:
      this->SetNumberOfScalarComponents(
        this->GetInternalImage()->SamplesPerPixel );
      break;
    case vtkTIFFReader::PALETTE_RGB:
      this->SetNumberOfScalarComponents( 3 );
      break;
    default:
      this->SetNumberOfScalarComponents( 4 );
    }

  if ( !this->GetInternalImage()->CanRead() )
    {
    this->SetNumberOfScalarComponents( 4 );
    }

  this->vtkImageReader2::ExecuteInformation();

  // close the file
  this->GetInternalImage()->Clean();
}


//-------------------------------------------------------------------------
template <class OT>
void vtkTIFFReaderUpdate2(vtkTIFFReader *self, OT *outPtr,
                          int *outExt, vtkIdType* vtkNotUsed(outInc), long) 
{
  if ( !self->GetInternalImage()->Open(self->GetInternalFileName()) )
    {
    return;
    }
  self->InitializeColors();
  self->ReadImageInternal(self->GetInternalImage()->Image,
                          outPtr, outExt, sizeof(OT) );

  // close the file
  self->GetInternalImage()->Clean();
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void vtkTIFFReaderUpdate(vtkTIFFReader *self, vtkImageData *data, OT *outPtr) 
{
  vtkIdType outIncr[3];
  int outExtent[6];
  OT *outPtr2;

  data->GetExtent(outExtent);
  data->GetIncrements(outIncr);

  long pixSize = data->GetNumberOfScalarComponents()*sizeof(OT);

  outPtr2 = outPtr;
  int idx2;
  for (idx2 = outExtent[4]; idx2 <= outExtent[5]; ++idx2)
    {
    self->ComputeInternalFileName(idx2);
    // read in a TIFF file
    vtkTIFFReaderUpdate2(self, outPtr2, outExtent, outIncr, pixSize);
    self->UpdateProgress((idx2 - outExtent[4])/
                         (outExtent[5] - outExtent[4] + 1.0));
    outPtr2 += outIncr[2];
    }
}


//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkTIFFReader::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);

  if (this->InternalFileName == NULL)
    {
    vtkErrorMacro("Either a FileName or FilePrefix must be specified.");
    return;
    }

  this->ComputeDataIncrements();

  // Call the correct templated function for the output
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  switch (data->GetScalarType()) 
    {
    vtkTemplateMacro(vtkTIFFReaderUpdate(this, data, (VTK_TT *)(outPtr)));
    default:
      vtkErrorMacro("UpdateFromFile: Unknown data type");
    }
  data->GetPointData()->GetScalars()->SetName("Tiff Scalars");
}

//----------------------------------------------------------------------------
unsigned int vtkTIFFReader::GetFormat()
{
  unsigned int cc;

  if ( this->ImageFormat != vtkTIFFReader::NOFORMAT )
    {
    return this->ImageFormat;
    }


  switch ( this->GetInternalImage()->Photometrics )
    {
    case PHOTOMETRIC_RGB:
    case PHOTOMETRIC_YCBCR:
      this->ImageFormat = vtkTIFFReader::RGB;
      return this->ImageFormat;
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
      this->ImageFormat = vtkTIFFReader::GRAYSCALE;
      return this->ImageFormat;
    case PHOTOMETRIC_PALETTE:
      for( cc=0; cc<256; cc++ )
        {
        unsigned short red, green, blue;
        this->GetColor( cc, &red, &green, &blue );
        if ( red != green || red != blue ) 
          {
          this->ImageFormat = vtkTIFFReader::PALETTE_RGB;
          return this->ImageFormat;
          }
        }
      this->ImageFormat = vtkTIFFReader::PALETTE_GRAYSCALE;
      return this->ImageFormat;
     }
  this->ImageFormat = vtkTIFFReader::OTHER;
  return this->ImageFormat;
}

//----------------------------------------------------------------------------
void vtkTIFFReader::GetColor( int index, unsigned short *red,
                              unsigned short *green, unsigned short *blue )
{
  *red   = 0;
  *green = 0;
  *blue  = 0;
  if ( index < 0 )
    {
    vtkErrorMacro("Color index has to be greater than 0");
    return;
    }
  if ( this->TotalColors > 0 &&
       this->ColorRed && this->ColorGreen && this->ColorBlue )
    {
    if ( index >= this->TotalColors )
      {
      vtkErrorMacro("Color index has to be less than number of colors ("
                    << this->TotalColors << ")");
      return;
      }
    *red   = *(this->ColorRed   + index);
    *green = *(this->ColorGreen + index);
    *blue  = *(this->ColorBlue  + index);
    return;
    }

  unsigned short photometric;

  if (!TIFFGetField(this->GetInternalImage()->Image, TIFFTAG_PHOTOMETRIC, &photometric))
    {
    if ( this->GetInternalImage()->Photometrics != PHOTOMETRIC_PALETTE )
      {
      vtkErrorMacro("You can only access colors for palette images");
      return;
      }
    }

  unsigned short *red_orig, *green_orig, *blue_orig;

  switch (this->GetInternalImage()->BitsPerSample) 
    {
    case 1: case 2: case 4:
    case 8: case 16:
      break;
    default:
      vtkErrorMacro( "Sorry, can not image with "
                     << this->GetInternalImage()->BitsPerSample
                     << "-bit samples" );
      return;
    }
  if (!TIFFGetField(this->GetInternalImage()->Image, TIFFTAG_COLORMAP,
                    &red_orig, &green_orig, &blue_orig))
    {
    vtkErrorMacro("Missing required \"Colormap\" tag");
    return;
    }
  this->TotalColors = (1L << this->GetInternalImage()->BitsPerSample);

  if ( index >= this->TotalColors )
    {
    vtkErrorMacro("Color index has to be less than number of colors ("
                  << this->TotalColors << ")");
    return;
    }
  this->ColorRed   =   red_orig;
  this->ColorGreen = green_orig;
  this->ColorBlue  =  blue_orig;

  *red   = *(red_orig   + index);
  *green = *(green_orig + index);
  *blue  = *(blue_orig  + index);
}

//-------------------------------------------------------------------------
void vtkTIFFReader::InitializeColors()
{
  this->ColorRed    = 0;
  this->ColorGreen  = 0;
  this->ColorBlue   = 0;
  this->TotalColors = -1;
  this->ImageFormat = vtkTIFFReader::NOFORMAT;
}

//-------------------------------------------------------------------------
template <typename T>
void ReadTiledImage(vtkTIFFReader *self, void *out,
                    unsigned int width,
                    unsigned int height,
                    unsigned int vtkNotUsed(size),
                    int *internalExtents)
{
  TIFF *tiff;
  uint32 tileWidth, tileLength, x, y, yi, rows, cols, tileSize;
  int xx,yy;
  int pixelDepth = self->GetInternalImage()->SamplesPerPixel;
  T *image;
  uint32 imagepos;

  image = (T*)out;
  tiff = self->GetInternalImage()->Image;
  TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tileWidth);
  TIFFGetField(tiff, TIFFTAG_TILELENGTH, &tileLength);
  tileSize = TIFFTileSize(tiff);
  tdata_t buffer = _TIFFmalloc(tileSize);

  for(yi=0;yi<height;yi+=tileLength)
    {
    for(x=0;x<width;x+=tileWidth)
      {
      y = yi;
      TIFFReadTile(tiff, buffer, x,y,0,0);
      if (tileWidth > width - x)
        {
        cols = width-x;
        } 
      else
        {
        cols = tileWidth;
        }
      if(tileLength > height - y)
        {
        rows = height - y;
        }
      else
        {
        rows = tileLength;
        }
      for(uint32 j = 0;j< rows;j++)
        {
        for(uint32 i=0;i<cols;i++)
          {
          uint32 tilepos = (i+j*cols)*pixelDepth;
          imagepos  = (((height -1) - y)*width+(x) + i - (j)*width)*pixelDepth;
          xx = x + i;
          yy = (height-1-y-j);
          if ( xx >= internalExtents[0] &&
               xx <= internalExtents[1] &&
               yy >= internalExtents[2] &&
               yy <= internalExtents[3] )
            {
            imagepos = (xx + width*yy)*pixelDepth;
            self->EvaluateImageAt(image+imagepos, static_cast<T*>(buffer)+tilepos);
            }
          }
        }
      }
    }
  _TIFFfree(buffer);
}

//-------------------------------------------------------------------------
template<typename T>
void ReadScanlineImage(vtkTIFFReader *self, void *out,
                       unsigned int vtkNotUsed(width),
                       unsigned height,
                       unsigned int vtkNotUsed(size),
                       int *internalExtents)
{
  unsigned int isize = TIFFScanlineSize(self->GetInternalImage()->Image);
  unsigned int cc;
  int row, inc;
  int xx=0, yy=0;
  tdata_t buf = _TIFFmalloc(isize);
  T *image = (T *)out;
  unsigned int c_inc = 
    self->GetInternalImage()->SamplesPerPixel; // * self->GetInternalImage()->BitsPerSample;
  if ( self->GetInternalImage()->PlanarConfig == PLANARCONFIG_CONTIG )
    {
    for ( row = height-1; row >= 0; row -- )
      {
      if (TIFFReadScanline(self->GetInternalImage()->Image, buf, row, 0) <= 0)
        {
        cout << "Problem reading the row: " << row << endl;
        break;
        }

      for (cc = 0; cc < isize; cc += c_inc )
        {
        if ( xx >= internalExtents[0] &&
             xx <= internalExtents[1] &&
             yy >= internalExtents[2] &&
             yy <= internalExtents[3] )
          {
          //unsigned char *c = static_cast<unsigned char *>(buf)+cc;
          inc = self->EvaluateImageAt( image,
                                       static_cast<T*>(buf) + cc
                                       );
          image += inc;
          }
        xx++;
        }
      xx=0;
      yy++;
      }
    }
  else
    {
    cout << "This reader can only do PLANARCONFIG_CONTIG" << endl;
    }

  _TIFFfree(buf);
}

//-------------------------------------------------------------------------
void vtkTIFFReader::ReadImageInternal( void* vtkNotUsed(in), void* outPtr,
                                       int* outExt,
                                       unsigned int size )
{
  if ( this->GetInternalImage()->Compression == COMPRESSION_OJPEG ) 
    {
    vtkErrorMacro("This reader cannot read old JPEG compression");
    return;
    }

  int width  = this->GetInternalImage()->Width;
  int height = this->GetInternalImage()->Height;
  this->InternalExtents = outExt;

  if ( !this->GetInternalImage()->CanRead() )
    {
    uint32 *tempImage = static_cast<uint32*>( outPtr );

    if ( this->InternalExtents[0] != 0 ||
         this->InternalExtents[1] != width -1 ||
         this->InternalExtents[2] != 0 ||
         this->InternalExtents[3] != height-1 )
      {
      tempImage = new uint32[ width * height ];
      }
    if ( !TIFFReadRGBAImage(this->GetInternalImage()->Image,
                            width, height,
                            tempImage, 0 ) )
      {
      vtkErrorMacro("Problem reading RGB image");
      if ( tempImage != outPtr )
        {
        delete [] tempImage;
        }

      return;
    }
    int xx, yy;
    uint32* ssimage = tempImage;
    unsigned char *fimage = (unsigned char *)outPtr;
    for ( yy = 0; yy < height; yy ++ )
      {
      for ( xx = 0; xx < width; xx++ )
        {
        if ( xx >= this->InternalExtents[0] &&
             xx <= this->InternalExtents[1] &&
             yy >= this->InternalExtents[2] &&
             yy <= this->InternalExtents[3] )
          {
          unsigned char red   = static_cast<unsigned char>(TIFFGetR(*ssimage));
          unsigned char green = static_cast<unsigned char>(TIFFGetG(*ssimage));
          unsigned char blue  = static_cast<unsigned char>(TIFFGetB(*ssimage));
          unsigned char alpha = static_cast<unsigned char>(TIFFGetA(*ssimage));

          *(fimage  ) = red;//red;
          *(fimage+1) = green;//green;
          *(fimage+2) = blue;//blue;
          *(fimage+3) = alpha;//alpha;
          fimage += 4;
          }
        ssimage ++;
        }
      }

    if ( tempImage != 0 && tempImage != outPtr )
      {
      delete [] tempImage;
      }
    return;
  }

  unsigned int format = this->GetFormat();


  if ( this->GetInternalImage()->Compression == COMPRESSION_PACKBITS )
    {
    height /= this->GetInternalImage()->BitsPerSample;
    }

  switch ( format ) 
    {
    case vtkTIFFReader::GRAYSCALE:
    case vtkTIFFReader::RGB:
    case vtkTIFFReader::PALETTE_RGB:
    case vtkTIFFReader::PALETTE_GRAYSCALE:
      if (TIFFIsTiled(this->GetInternalImage()->Image))
        {
        switch(this->GetInternalImage()->BitsPerSample) 
          {
          case 32:
            ReadTiledImage<float>(this, outPtr, width, height, size, this->InternalExtents);
            break;
          default:
            ReadTiledImage<unsigned char>(this, outPtr, width, height, size, this->InternalExtents);

          }
        }
      else
        {
        switch(this->GetInternalImage()->BitsPerSample)
          {
          case 32:
            ReadScanlineImage<float>(this, outPtr, width, height, size, this->InternalExtents);
            break;
          default:
            ReadScanlineImage<unsigned char>(this, outPtr, width, height, size, this->InternalExtents);
          }
        }
      break;
    default:
      return;
    }
}

//-------------------------------------------------------------------------
int vtkTIFFReader::EvaluateImageAt( void* out, void* in )
{
  unsigned char *image = (unsigned char *)out;
  unsigned char *source = (unsigned char *)in;
  int increment;
  unsigned short red, green, blue, alpha;
  switch ( this->GetFormat() ) 
    {
    case vtkTIFFReader::GRAYSCALE:
      if ( this->GetInternalImage()->Photometrics == PHOTOMETRIC_MINISBLACK )
        {
        *image = *source;
        } 
      else
        {
        *image = ~( *source );
        }
      increment = 1;
      break;
    case vtkTIFFReader::PALETTE_GRAYSCALE:
      this->GetColor(*source, &red, &green, &blue);
      *image = static_cast<unsigned char>(red >> 8);
      increment = 1;
      break;
    case vtkTIFFReader::RGB:
      red   = *(source);
      green = *(source+1);
      blue  = *(source+2);
      *(image)   = red;
      *(image+1) = green;
      *(image+2) = blue;
      if ( this->GetInternalImage()->SamplesPerPixel == 4 )
        {
        alpha = *(source+3);
        *(image+3) = 255-alpha;
        }
      increment = this->GetInternalImage()->SamplesPerPixel;
      break;
    case vtkTIFFReader::PALETTE_RGB:
      this->GetColor(*source, &red, &green, &blue);
      *(image)   = static_cast<unsigned char>(red >> 8);
      *(image+1) = static_cast<unsigned char>(green >> 8);
      *(image+2) = static_cast<unsigned char>(blue >> 8);
      increment = 3;
      break;
    default:
      return 0;
    }

  return increment;
}

//-------------------------------------------------------------------------
int vtkTIFFReader::CanReadFile(const char* fname)
{
  vtkTIFFReaderInternal tf;
  int res = tf.Open(fname);
  tf.Clean();
  if (res) 
    {
    return 3;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkTIFFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
