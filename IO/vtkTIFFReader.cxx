/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReader.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTIFFReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkObjectFactory.h"

#include <sys/stat.h>

#include <vtkstd/string>


extern "C" {
#include "vtk_tiff.h"
}

//-------------------------------------------------------------------------
vtkStandardNewMacro(vtkTIFFReader);
vtkCxxRevisionMacro(vtkTIFFReader, "1.53");

class vtkTIFFReaderInternal
{
public:
  vtkTIFFReaderInternal();
  int Initialize();
  void Clean();
  int CanRead();
  int Open( const char *filename );
  TIFF *Image;
  bool IsOpen;
  unsigned int Width;
  unsigned int Height;
  unsigned short NumberOfPages;
  unsigned short CurrentPage;
  unsigned short SamplesPerPixel;
  unsigned short Compression;
  unsigned short BitsPerSample;
  unsigned short Photometrics;
  bool HasValidPhotometricInterpretation;
  unsigned short PlanarConfig;
  unsigned short Orientation;
  unsigned long int TileDepth;
  unsigned int TileRows;
  unsigned int TileColumns;
  unsigned int TileWidth;
  unsigned int TileHeight;
  unsigned short NumberOfTiles;
  unsigned int SubFiles;
  unsigned int ResolutionUnit;
  float XResolution;
  float YResolution;
  short SampleFormat;
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

  this->IsOpen = true;
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
  this->HasValidPhotometricInterpretation = false;
  this->PlanarConfig = 0;
  this->TileDepth = 0;
  this->CurrentPage = 0;
  this->NumberOfPages = 0;
  this->NumberOfTiles = 0;
  this->TileRows = 0;
  this->TileColumns = 0;
  this->TileWidth = 0;
  this->TileHeight = 0;
  this->XResolution = 1;
  this->YResolution = 1;
  this->SubFiles = 0;
  this->SampleFormat = 1;
  this->ResolutionUnit = 1; // none
  this->IsOpen = false;
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

    // Get the resolution in each direction
    TIFFGetField(this->Image, TIFFTAG_XRESOLUTION, &this->XResolution);
    TIFFGetField(this->Image, TIFFTAG_YRESOLUTION, &this->YResolution);
    TIFFGetField(this->Image, TIFFTAG_RESOLUTIONUNIT, &this->ResolutionUnit);

    // Check the number of pages. First by looking at the number of directories
    this->NumberOfPages = TIFFNumberOfDirectories(this->Image);

    if(this->NumberOfPages == 0)
      {
      if ( !TIFFGetField(this->Image,TIFFTAG_PAGENUMBER,&this->CurrentPage,
          &this->NumberOfPages))
        {
        // Check the Image Description tag to know the number of images
        // This is used by ImageJ
        char** description = new char*[255];
        if (TIFFGetField(this->Image,TIFFTAG_IMAGEDESCRIPTION,description))
          {
          // look for the number of images
          vtkstd::string desc = description[0];
          int pos = desc.find("images=");
          int pos2 = desc.find("\n");
          if( (pos != -1) && (pos2 != -1))
            {
            this->NumberOfPages = atoi(desc.substr(pos+7,pos2-pos-7).c_str());
            }
          }
        }
      }

    // If the number of pages is still zero we look if the image is tiled
    if(this->NumberOfPages == 0 && TIFFIsTiled(this->Image))
      {
      this->NumberOfTiles = TIFFNumberOfTiles(this->Image);

      if ( !TIFFGetField(this->Image,TIFFTAG_TILEWIDTH,&this->TileWidth)
        || !TIFFGetField(this->Image,TIFFTAG_TILELENGTH,&this->TileHeight)
        )
        {
        cerr << "Cannot read tile width and tile length from file" << endl;
        }
      else
        {
        TileRows = this->Height/this->TileHeight;
        TileColumns = this->Width/this->TileWidth;
        }
      }

    // Checking if the TIFF contains subfiles
    if(this->NumberOfPages > 1)
      {
      this->SubFiles = 0;

      for(unsigned int page = 0;page<this->NumberOfPages;page++)
        {
        long subfiletype = 6;
        if(TIFFGetField(this->Image, TIFFTAG_SUBFILETYPE, &subfiletype))
          {
          if(subfiletype == 0)
            {
            this->SubFiles+=1;
            }
          }
        TIFFReadDirectory(this->Image);
        }

      // Set the directory to the first image
      TIFFSetDirectory(this->Image,0);
      }

      // TIFFTAG_ORIENTATION tag from the image 
      // data and use it if available. If the tag is not found in the image data,
      // use ORIENTATION_BOTLEFT by default. 
      int status =  TIFFGetField(this->Image, TIFFTAG_ORIENTATION,
                            &this->Orientation);
      if( ! status )
        {
        this->Orientation = ORIENTATION_BOTLEFT;
        }

    TIFFGetFieldDefaulted(this->Image, TIFFTAG_SAMPLESPERPIXEL, 
                          &this->SamplesPerPixel);
    TIFFGetFieldDefaulted(this->Image, TIFFTAG_COMPRESSION, &this->Compression);
    TIFFGetFieldDefaulted(this->Image, TIFFTAG_BITSPERSAMPLE, 
                          &this->BitsPerSample);
    TIFFGetFieldDefaulted(this->Image, TIFFTAG_PLANARCONFIG, &this->PlanarConfig);
    TIFFGetFieldDefaulted(this->Image, TIFFTAG_SAMPLEFORMAT, &this->SampleFormat);

    // If TIFFGetField returns false, there's no Photometric Interpretation 
    // set for this image, but that's a required field so we set a warning flag.
    // (Because the "Photometrics" field is an enum, we can't rely on setting 
    // this->Photometrics to some signal value.)
    if (TIFFGetField(this->Image, TIFFTAG_PHOTOMETRIC, &this->Photometrics))
      {
      this->HasValidPhotometricInterpretation = true;
      }
    else
      {
      this->HasValidPhotometricInterpretation = false;
      }
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
           ( this->Compression == COMPRESSION_NONE || 
             this->Compression == COMPRESSION_PACKBITS || 
             this->Compression == COMPRESSION_LZW
             ) &&
           ( this->HasValidPhotometricInterpretation ) &&
           ( this->Photometrics == PHOTOMETRIC_RGB ||
             this->Photometrics == PHOTOMETRIC_MINISWHITE ||
             this->Photometrics == PHOTOMETRIC_MINISBLACK ||
             this->Photometrics == PHOTOMETRIC_PALETTE ) &&
           ( this->PlanarConfig == PLANARCONFIG_CONTIG ) &&
           ( !this->TileDepth ) &&
           ( this->BitsPerSample == 8 || this->BitsPerSample == 16 ) );
}

//-------------------------------------------------------------------------
vtkTIFFReader::vtkTIFFReader()
{

  this->InitializeColors();
  this->InternalImage = new vtkTIFFReaderInternal;
  this->OutputExtent = 0;
  this->OutputIncrements = 0;

  this->OrientationTypeSpecifiedFlag = false;
  this->OriginSpecifiedFlag = false;
  this->SpacingSpecifiedFlag = false;

  //Make the default orientation type to be ORIENTATION_BOTLEFT
  this->OrientationType = 4;
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

  // if orientation information is provided, overwrite the value
  // read from the tiff image
  if( this->OrientationTypeSpecifiedFlag )
    {
    this->GetInternalImage()->Orientation = OrientationType;
    }

  if( !SpacingSpecifiedFlag )
    {
    this->DataSpacing[0] = 1.0; 
    this->DataSpacing[1] = 1.0;

  // If we have some spacing information we use it
    if(this->GetInternalImage()->ResolutionUnit>0
      && this->GetInternalImage()->XResolution>0
      && this->GetInternalImage()->YResolution>0
      )
      {
      if(this->GetInternalImage()->ResolutionUnit == 2) // inches
        {
        this->DataSpacing[0] = 25.4/this->GetInternalImage()->XResolution; 
        this->DataSpacing[1] = 25.4/this->GetInternalImage()->YResolution; 
        }
      else if(this->GetInternalImage()->ResolutionUnit == 3) // cm
        {
        this->DataSpacing[0] = 10.0/this->GetInternalImage()->XResolution; 
        this->DataSpacing[1] = 10.0/this->GetInternalImage()->YResolution; 
        }
      }
    }

  if( !OriginSpecifiedFlag )
    {
    this->DataOrigin[0] = 0.0;
    this->DataOrigin[1] = 0.0;
    }

  // pull out the width/height, etc.
  this->DataExtent[0] = 0;
  this->DataExtent[1] = this->GetInternalImage()->Width - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = this->GetInternalImage()->Height - 1;

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

  if (this->GetInternalImage()->BitsPerSample <= 8)
    {
    if(this->GetInternalImage()->SampleFormat == 2)
      {
      this->SetDataScalarType( VTK_CHAR );
      }
    else
      {
      this->SetDataScalarTypeToUnsignedChar();
      }
    }
  else
    {
    if(this->GetInternalImage()->SampleFormat == 2)
      {
      this->SetDataScalarType( VTK_SHORT );
      }
    else
      {
      this->SetDataScalarTypeToUnsignedShort();
      }
    }

  // We check if we have a Zeiss image. 
  // Meaning that the SamplesPerPixel is 2 but the image should be treated as
  // an RGB image.
  if(this->GetInternalImage()->SamplesPerPixel == 2)
    {
    this->SetNumberOfScalarComponents(3);
    }

   // if the tiff file is multi-pages
   // series of tiff images ( 3D volume )
  if(this->GetInternalImage()->NumberOfPages>1)
    {
    if(this->GetInternalImage()->SubFiles>0)
      {
      this->DataExtent[5] = this->GetInternalImage()->SubFiles;
      }
    else
      {
      this->DataExtent[5] = this->GetInternalImage()->NumberOfPages;
      }

    if( !SpacingSpecifiedFlag )
      {
      this->DataSpacing[2] = 1.0;
      }
    if( !OriginSpecifiedFlag )
      {
      this->DataOrigin[2]  = 0.0;
      }
    }


   // if the tiff is tiled
   if(this->GetInternalImage()->NumberOfTiles>1)
     {
     this->DataExtent[1]  = this->GetInternalImage()->TileWidth;
     this->DataExtent[3]  = this->GetInternalImage()->TileHeight;
     this->DataExtent[5]  = this->GetInternalImage()->NumberOfTiles;
     if( !SpacingSpecifiedFlag )
       {
       this->DataSpacing[2] = 1.0;
       }
     if( !OriginSpecifiedFlag )
       {
       this->DataOrigin[2]  = 0.0;
       }
     }

  this->vtkImageReader2::ExecuteInformation();

  // Don't close the file yet, since we need the image internal
  // parameters such as NumberOfPages, NumberOfTiles to decide
  // how to read in the image
}


// Set orientation type
//
//ORIENTATION_TOPLEFT         1       (row 0 top, col 0 lhs)
//ORIENTATION_TOPRIGHT        2       (row 0 top, col 0 rhs)
//ORIENTATION_BOTRIGHT        3       (row 0 bottom, col 0 rhs)
//ORIENTATION_BOTLEFT         4       (row 0 bottom, col 0 lhs)
//ORIENTATION_LEFTTOP         5       (row 0 lhs, col 0 top)
//ORIENTATION_RIGHTTOP        6       (row 0 rhs, col 0 top)
//ORIENTATION_RIGHTBOT        7       (row 0 rhs, col 0 bottom)
//ORIENTATION_LEFTBOT         8       (row 0 lhs, col 0 bottom) */ 
void vtkTIFFReader::SetOrientationType( unsigned int orientationType )
{
  if ( orientationType < 1 || orientationType > 8 )
    {
    vtkErrorMacro( "Invalid Orientation type specified" );
    return;
    }

  if( this->OrientationType != orientationType )
    {
    this->OrientationType = orientationType;
    this->Modified();
    }
  if( !this->OrientationTypeSpecifiedFlag )
    {
    this->Modified();
    }
  // To preserve backward compatibility OrientationTypeSpecifiedFlag would
  // always be set to true whatever user input...
  this->OrientationTypeSpecifiedFlag = true;
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
  // if orientation information is provided, overwrite the value
  // read from the tiff image
  if( self->GetOrientationTypeSpecifiedFlag())
    {
    self->GetInternalImage()->Orientation = self->GetOrientationType();
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

  // multiple number of pages
  if(self->GetInternalImage()->NumberOfPages>1 )
    {
    self->ReadVolume( outPtr );
    return;
    }

  // tiled image
  if(self->GetInternalImage()->NumberOfTiles>0 )
    {
    self->ReadTiles( outPtr );
    return;
    }

  //The input tiff dataset is neither multiple pages and nor 
  //tiled. Hence close the image and start reading each TIFF
  //file 
  self->GetInternalImage()->Clean();

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
  // Needed deep in reading for finding the correct starting location.
  this->OutputIncrements = data->GetIncrements();

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
        cerr << "Problem reading the row in ReadScalineImage method:" << endl;
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
    cerr << "This TIFF reader can only do PLANARCONFIG_CONTIG" << endl;
    }

  _TIFFfree(buf);
}


//-------------------------------------------------------------------------
void vtkTIFFReader::ReadVolume(void* buffer)
{

  if ( this->GetInternalImage()->Compression == COMPRESSION_OJPEG ) 
    {
    vtkErrorMacro("This reader cannot read old JPEG compression");
    return;
    }


  int width  = this->GetInternalImage()->Width;
  int height = this->GetInternalImage()->Height;

  for(unsigned int page = 0;page<this->GetInternalImage()->NumberOfPages;page++)
    {
    if(this->GetInternalImage()->SubFiles>0)
      {
      long subfiletype = 6;
      if(TIFFGetField(this->GetInternalImage()->Image, TIFFTAG_SUBFILETYPE, &subfiletype))
        {
        if(subfiletype != 0)
          {
          TIFFReadDirectory(this->GetInternalImage()->Image);
          continue;
          }
        }
      }

    // if we have a Zeiss image meaning that the SamplesPerPixel is 2
    if(this->GetInternalImage()->SamplesPerPixel == 2)
      {
      if(this->GetDataScalarType() == VTK_UNSIGNED_SHORT)
        {
        unsigned short* volume = reinterpret_cast<unsigned short*>(buffer);
        volume += width*height*this->GetInternalImage()->SamplesPerPixel*page;
        this->ReadTwoSamplesPerPixelImage( volume, width, height );
        }
      else if(this->GetDataScalarType() == VTK_SHORT)
        {
        short* volume = reinterpret_cast<short*>(buffer);
        volume += width*height*this->GetInternalImage()->SamplesPerPixel*page;
        this->ReadTwoSamplesPerPixelImage( volume, width, height );
        }
      else if(this->GetDataScalarType() == VTK_CHAR)
        {
        char* volume = reinterpret_cast<char*>(buffer);
        volume += width*height*this->GetInternalImage()->SamplesPerPixel*page;
        this->ReadTwoSamplesPerPixelImage( volume, width, height );
        }
      else
        {
        unsigned char* volume = reinterpret_cast<unsigned char*>(buffer);
        volume += width*height*this->GetInternalImage()->SamplesPerPixel*page;
        this->ReadTwoSamplesPerPixelImage( volume, width, height );
        }
      break;
      }
    else if ( !this->GetInternalImage()->CanRead() )
      {
      uint32 *tempImage;
      tempImage = new uint32[ width * height ];

      if ( !TIFFReadRGBAImage(this->GetInternalImage()->Image,
                              width, height,
                              tempImage, 1 ) )
        {
        vtkErrorMacro( << "Cannot read TIFF image or as a TIFF RGBA image" );
        if ( tempImage != buffer )
          {
          delete [] tempImage;
          }
        return;
        }
      int xx, yy;
      uint32* ssimage;

      if(this->GetDataScalarType() == VTK_UNSIGNED_SHORT)
        {
        unsigned short *fimage = (unsigned short *)buffer;
        fimage += width*height*4*page;
        for ( yy = 0; yy < height; yy ++ )
          {
          ssimage = tempImage + (height - yy - 1) * width;
          for ( xx = 0; xx < width; xx++ )
            {
            unsigned short red   = static_cast<unsigned short>(TIFFGetR(*ssimage));
            unsigned short green = static_cast<unsigned short>(TIFFGetG(*ssimage));
            unsigned short blue  = static_cast<unsigned short>(TIFFGetB(*ssimage));
            unsigned short alpha = static_cast<unsigned short>(TIFFGetA(*ssimage));

            *(fimage  ) = red;
            *(fimage+1) = green;
            *(fimage+2) = blue;
            *(fimage+3) = alpha;
            fimage += 4;
            ssimage ++;
            }
          }
        }
      else if(this->GetDataScalarType() == VTK_SHORT)
        {
        short *fimage = (short *)buffer;
        fimage += width*height*4*page;
        for ( yy = 0; yy < height; yy ++ )
          {
          ssimage = tempImage + (height - yy - 1) * width;
          for ( xx = 0; xx < width; xx++ )
            {
            short red   = static_cast<short>(TIFFGetR(*ssimage));
            short green = static_cast<short>(TIFFGetG(*ssimage));
            short blue  = static_cast<short>(TIFFGetB(*ssimage));
            short alpha = static_cast<short>(TIFFGetA(*ssimage));

            *(fimage  ) = red;
            *(fimage+1) = green;
            *(fimage+2) = blue;
            *(fimage+3) = alpha;
            fimage += 4;
            ssimage ++;
            }
          }
        }      
      else if(this->GetDataScalarType() == VTK_CHAR)
        {
        char *fimage = (char *)buffer;
        fimage += width*height*4*page;
        for ( yy = 0; yy < height; yy ++ )
          {
          ssimage = tempImage + (height - yy - 1) * width;
          for ( xx = 0; xx < width; xx++ )
            {
            char red   = static_cast<char>(TIFFGetR(*ssimage));
            char green = static_cast<char>(TIFFGetG(*ssimage));
            char blue  = static_cast<char>(TIFFGetB(*ssimage));
            char alpha = static_cast<char>(TIFFGetA(*ssimage));

            *(fimage  ) = red;
            *(fimage+1) = green;
            *(fimage+2) = blue;
            *(fimage+3) = alpha;
            fimage += 4;
            ssimage ++;
            }
          }
        }
      else
        {
        unsigned char *fimage = (unsigned char *)buffer;
        fimage += width*height*4*page/2;
        for ( yy = 0; yy < height; yy ++ )
          {
          ssimage = tempImage + (height - yy - 1) * width;
          for ( xx = 0; xx < width; xx++ )
            {
            unsigned char red   = static_cast<unsigned char>(TIFFGetR(*ssimage));
            unsigned char green = static_cast<unsigned char>(TIFFGetG(*ssimage));
            unsigned char blue  = static_cast<unsigned char>(TIFFGetB(*ssimage));
            unsigned char alpha = static_cast<unsigned char>(TIFFGetA(*ssimage));

            *(fimage  ) = red;
            *(fimage+1) = green;
            *(fimage+2) = blue;
            *(fimage+3) = alpha;
            fimage += 4;
            ssimage ++;
            }
          }
        }
      if ( tempImage != 0 && tempImage != buffer )
        {
        delete [] tempImage;
        }
      }
    else
      {
      unsigned int format = this->GetFormat();

      switch ( format )
        {
        case vtkTIFFReader::GRAYSCALE:
        case vtkTIFFReader::RGB:
        case vtkTIFFReader::PALETTE_RGB:
        case vtkTIFFReader::PALETTE_GRAYSCALE:
          if(this->GetDataScalarType() == VTK_UNSIGNED_SHORT)
            {
            unsigned short* volume = reinterpret_cast<unsigned short*>(buffer);
            volume += width*height*this->GetInternalImage()->SamplesPerPixel*page;
            this->ReadGenericImage( volume, width, height );
            }
          else if(this->GetDataScalarType() == VTK_SHORT)
            {
            short* volume = reinterpret_cast<short*>(buffer);
            volume += width*height*this->GetInternalImage()->SamplesPerPixel*page;
            this->ReadGenericImage( volume, width, height );
            }
          else if(this->GetDataScalarType() == VTK_CHAR)
            {
            char* volume = reinterpret_cast<char*>(buffer);
            volume += width*height*this->GetInternalImage()->SamplesPerPixel*page;
            this->ReadGenericImage( volume, width, height );
            }
          else
            {
            unsigned char* volume = reinterpret_cast<unsigned char*>(buffer);
            volume += width*height*this->GetInternalImage()->SamplesPerPixel*page;
            this->ReadGenericImage( volume, width, height );
            }
          break;
        default:
          return;
        }
      }
    TIFFReadDirectory(this->GetInternalImage()->Image);
    }
}

/** Read a tiled tiff */
void vtkTIFFReader::ReadTiles(void* buffer)
{
  if ( this->GetInternalImage()->Compression == COMPRESSION_OJPEG ) 
    {
    vtkErrorMacro("This reader cannot read old JPEG compression");
    return;
    }

  unsigned char* volume = reinterpret_cast<unsigned char*>(buffer);

  for(unsigned int col = 0;col<this->GetInternalImage()->Width;col+=this->GetInternalImage()->TileWidth)
    {
    for(unsigned int row = 0;row<this->GetInternalImage()->Height;row+=this->GetInternalImage()->TileHeight)
      {
      unsigned char *tempImage;
      tempImage = new unsigned char[ this->GetInternalImage()->TileWidth * this->GetInternalImage()->TileHeight * this->GetInternalImage()->SamplesPerPixel];

      if(TIFFReadTile(this->GetInternalImage()->Image,tempImage, col,row,0,0)<0)
        {
        vtkErrorMacro( << "Cannot read tile : "<< row << "," << col << " from file" );
        if ( tempImage != buffer )
          {
          delete [] tempImage;
          }

        return;
        }

      unsigned int xx, yy;
      for ( yy = 0; yy < this->GetInternalImage()->TileHeight; yy++ )
        {
        for ( xx = 0; xx <  this->GetInternalImage()->TileWidth; xx++ )
          {
          for(unsigned int i=0;i< this->GetInternalImage()->SamplesPerPixel;i++)
            {
            *volume = *(tempImage++);
            volume++;
            }
          }
        }
      }
    }
}

/** To Support Zeiss images that contains only 2 samples per pixel but are actually
 *  RGB images */
void vtkTIFFReader::ReadTwoSamplesPerPixelImage( void *out, 
                                               unsigned int width, 
                                               unsigned int height )
{
  unsigned int isize = TIFFScanlineSize(this->GetInternalImage()->Image);
  unsigned int cc;
  int row;
  tdata_t buf = _TIFFmalloc(isize);

  int inc = 1;

  if(this->GetDataScalarType() == VTK_UNSIGNED_CHAR)
    {
    unsigned char* image;
     if (this->GetInternalImage()->PlanarConfig == PLANARCONFIG_CONTIG) 
      {
      for ( row = 0; row < (int)height; row ++ )
        {
        if (TIFFReadScanline(this->GetInternalImage()->Image, buf, row, 0) <= 0)
          {
          vtkErrorMacro( << "Problem reading the row: " << row );
          break;
          }

        if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
          {
          image = reinterpret_cast<unsigned char*>(out) + row * width * inc;
          }
        else
          {
          image = reinterpret_cast<unsigned char*>(out) + width * inc * (height - (row + 1));
          }

        for (cc = 0; cc < isize;
             cc += this->GetInternalImage()->SamplesPerPixel )
          {
          inc = this->EvaluateImageAt( image,
                                       static_cast<unsigned char *>(buf) +
                                       cc );
          image += inc;
          }
        }
      }
    else if(this->GetInternalImage()->PlanarConfig == PLANARCONFIG_SEPARATE)
      {
      unsigned long s;
      unsigned long nsamples = 0;
      TIFFGetField(this->GetInternalImage()->Image, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
      for (s = 0; s < nsamples; s++)
        {
        for ( row = 0; row < (int)height; row ++ )
          {
          if (TIFFReadScanline(this->GetInternalImage()->Image, buf, row, s) <= 0)
            {
            vtkErrorMacro( << "Problem reading the row: " << row );
            break;
            }

          inc = 3;

          if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
            {
            image = reinterpret_cast<unsigned char*>(out) + row * width * inc;
            }
          else
            {
            image = reinterpret_cast<unsigned char*>(out) + width * inc * (height - (row + 1));
            }

          // We translate the output pixel to be on the right RGB
          image += s;
          for (cc = 0; cc < isize;
               cc += 1)
            {
            (*image) = *(static_cast<unsigned char *>(buf) + cc);
            inc = 3;
            image += inc;
            }
          }
        }
      }
    }
  else if(this->GetDataScalarType() == VTK_UNSIGNED_SHORT)
    {
    isize /= 2;
    unsigned short* image;
    if (this->GetInternalImage()->PlanarConfig == PLANARCONFIG_CONTIG) 
      {
      for ( row = 0; row < (int)height; row ++ )
        {
        if (TIFFReadScanline(this->GetInternalImage()->Image, buf, row, 0) <= 0)
          {
          vtkErrorMacro( << "Problem reading the row: " << row );
          break;
          }

        if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
          {
          image = reinterpret_cast<unsigned short*>(out) + row * width * inc;
          }
        else
          {
          image = reinterpret_cast<unsigned short*>(out) + width * inc * (height - (row + 1));
          }

        for (cc = 0; cc < isize;
             cc += this->GetInternalImage()->SamplesPerPixel )
          {
          inc = this->EvaluateImageAt( image,
                                       static_cast<unsigned short *>(buf) +
                                       cc );
          image += inc;
          }
        }
      }
    else if(this->GetInternalImage()->PlanarConfig == PLANARCONFIG_SEPARATE)
      {
      unsigned long s, nsamples;
      TIFFGetField(this->GetInternalImage()->Image, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
      for (s = 0; s < nsamples; s++)
        {
        for ( row = 0; row < (int)height; row ++ )
          {
          if (TIFFReadScanline(this->GetInternalImage()->Image, buf, row, s) <= 0)
            {
            vtkErrorMacro( << "Problem reading the row: " << row );
            break;
            }

          if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
            {
            image = reinterpret_cast<unsigned short*>(out) + row * width * inc;
            }
          else
            {
            image = reinterpret_cast<unsigned short*>(out) + width * inc * (height - (row + 1));
            }
          // We translate the output pixel to be on the right RGB
          image += s;
          for (cc = 0; cc < isize; 
               cc += 1)
            {
            (*image) = *(static_cast<unsigned short *>(buf) + cc);
            inc = 3;
            image += inc;
            }
          }
        }
      }
    }
  _TIFFfree(buf); 
}


void vtkTIFFReader::ReadGenericImage( void *out, 
                                      unsigned int, 
                                      unsigned int height )
{
  unsigned int isize = TIFFScanlineSize(this->GetInternalImage()->Image);
  unsigned int cc;
  int row, inc, fileRow;
  tdata_t buf = _TIFFmalloc(isize);

  if ( this->GetInternalImage()->PlanarConfig != PLANARCONFIG_CONTIG )
    {
    vtkErrorMacro( << "This reader can only do PLANARCONFIG_CONTIG" );
    return;
    }

  switch ( this->GetFormat() )
    {
    default:
    case vtkTIFFReader::GRAYSCALE:
    case vtkTIFFReader::PALETTE_GRAYSCALE:
      inc = 1;
      break;
    case vtkTIFFReader::RGB: 
      inc = this->GetInternalImage()->SamplesPerPixel;
      break;
    case vtkTIFFReader::PALETTE_RGB:
      inc = 3;
      break;
    }

  // Inc is never used because we use the increment set previously.
  // I do not want to get rid of the varialbe inc completely,
  // so i added this test.  It might be better as a warning.
  if (inc != this->OutputIncrements[0])
    {
    vtkDebugMacro("Computed increment " << inc 
                  << " does not match information increment " 
                  << this->OutputIncrements[0]);
    }
  
  if(this->GetDataScalarType() == VTK_UNSIGNED_CHAR)
    {
    unsigned char* image;
    if (this->GetInternalImage()->PlanarConfig == PLANARCONFIG_CONTIG) 
      {
      for ( row = this->OutputExtent[2]; row <= this->OutputExtent[3]; row ++ )
        {
        // Flip from lower left origin to upper left if necessary.
        if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
          {      
          fileRow = row;
          }
        else
          {
          fileRow = height - row - 1;
          }
        if (TIFFReadScanline(this->GetInternalImage()->Image, buf, fileRow, 0) <= 0)
          {
          vtkErrorMacro( << "Problem reading the row: " << fileRow );
          break;
          }
        image = reinterpret_cast<unsigned char*>(out) 
          + (row-this->OutputExtent[2])*this->OutputIncrements[1];

        // Copy the pixels into the output buffer
        cc = this->OutputExtent[0] * this->GetInternalImage()->SamplesPerPixel;
        for (int ix = this->OutputExtent[0]; ix <= this->OutputExtent[1]; ++ix)
          {
          inc = this->EvaluateImageAt( image, 
                                       static_cast<unsigned char *>(buf) +
                                       cc );
          image += this->OutputIncrements[0];
          cc += this->GetInternalImage()->SamplesPerPixel;
          }
        }
      }
    else if(this->GetInternalImage()->PlanarConfig == PLANARCONFIG_SEPARATE)
      {
      unsigned long s;
      unsigned long nsamples;
      TIFFGetField(this->GetInternalImage()->Image, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
      for (s = 0; s < nsamples; s++)
        {
        for ( row = this->OutputExtent[2]; row <= this->OutputExtent[3]; row ++ )
          {
          if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
            {
            fileRow = row;
            }
          else
            {
            fileRow = height - row - 1;
            }
          if (TIFFReadScanline(this->GetInternalImage()->Image, buf, fileRow, s) <= 0)
            {
            vtkErrorMacro( << "Problem reading the row: " << fileRow );
            break;
            }
          
          inc = 3;
          image = reinterpret_cast<unsigned char*>(out) 
            + (row-this->OutputExtent[2])*this->OutputIncrements[1];
          cc = this->OutputExtent[0] * this->GetInternalImage()->SamplesPerPixel;
          for (int ix = this->OutputExtent[0]; ix <= this->OutputExtent[1]; ++ix)
            {
            inc = this->EvaluateImageAt( image, 
                                         static_cast<unsigned char *>(buf) +
                                         cc );
            image += this->OutputIncrements[0];
            cc += this->GetInternalImage()->SamplesPerPixel;
            }
          }
        }
      }
    }
  else if(this->GetDataScalarType() == VTK_CHAR)
    {
    char* image;
     if (this->GetInternalImage()->PlanarConfig == PLANARCONFIG_CONTIG) 
      {
      for ( row = this->OutputExtent[2]; row <= this->OutputExtent[3]; row ++ )
        {
        if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
          {
          fileRow = row;
          }
        else
          {
          fileRow = height - row - 1;
          }
        if (TIFFReadScanline(this->GetInternalImage()->Image, buf, fileRow, 0) <= 0)
          {
          vtkErrorMacro( << "Problem reading the row: " << fileRow );
          break;
          }

        image = reinterpret_cast<char*>(out) 
          + (row-this->OutputExtent[2])*this->OutputIncrements[1];
        cc = this->OutputExtent[0] * this->GetInternalImage()->SamplesPerPixel;
        for (int ix = this->OutputExtent[0]; ix <= this->OutputExtent[1]; ++ix)
          {
          inc = this->EvaluateImageAt( image, 
                                       static_cast<char *>(buf) +
                                       cc );
          image += this->OutputIncrements[0];
          cc += this->GetInternalImage()->SamplesPerPixel;
          }
        }
      }
    else if(this->GetInternalImage()->PlanarConfig == PLANARCONFIG_SEPARATE)
      {
      unsigned long s;
      unsigned long nsamples;
      TIFFGetField(this->GetInternalImage()->Image, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
      for (s = 0; s < nsamples; s++)
        {
        for ( row = this->OutputExtent[2]; row <= this->OutputExtent[3]; row ++ )
          {
          if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
            {
            fileRow = row;
            }
          else
            {
            fileRow = height - row - 1;
            }
          if (TIFFReadScanline(this->GetInternalImage()->Image, buf, fileRow, s) <= 0)
            {
            vtkErrorMacro( << "Problem reading the row: " << fileRow );
            break;
            }
          
          inc = 3;
          image = reinterpret_cast<char*>(out) 
            + (row-this->OutputExtent[2])*this->OutputIncrements[1];
          cc = this->OutputExtent[0] * this->GetInternalImage()->SamplesPerPixel;
          for (int ix = this->OutputExtent[0]; ix <= this->OutputExtent[1]; ++ix)
            {
            inc = this->EvaluateImageAt( image, 
                                         static_cast<char *>(buf) +
                                         cc );
            image += this->OutputIncrements[0];
            cc += this->GetInternalImage()->SamplesPerPixel;
            }
          }
        }
      }
    }
  else if(this->GetDataScalarType() == VTK_UNSIGNED_SHORT)
    {
    isize /= 2;
    unsigned short* image;
    if (this->GetInternalImage()->PlanarConfig == PLANARCONFIG_CONTIG) 
      {
      for ( row = this->OutputExtent[2]; row <= this->OutputExtent[3]; row ++ )
        {
        if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
          {
          fileRow = row;
          }
        else
          {
          fileRow = height - row - 1;
          }
        if (TIFFReadScanline(this->GetInternalImage()->Image, buf, fileRow, 0) <= 0)
          {
          vtkErrorMacro( << "Problem reading the row: " << fileRow );
          break;
          }

        image = reinterpret_cast<unsigned short*>(out) 
          + (row-this->OutputExtent[2])*this->OutputIncrements[1];
        cc = this->OutputExtent[0] * this->GetInternalImage()->SamplesPerPixel;
        for (int ix = this->OutputExtent[0]; ix <= this->OutputExtent[1]; ++ix)
          {
          inc = this->EvaluateImageAt( image, 
                                       static_cast<unsigned short *>(buf) +
                                       cc );
          image += this->OutputIncrements[0];
          cc += this->GetInternalImage()->SamplesPerPixel;
          }
        }
      }
    else if(this->GetInternalImage()->PlanarConfig == PLANARCONFIG_SEPARATE)
      {
      unsigned long s, nsamples;
      TIFFGetField(this->GetInternalImage()->Image, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
      for (s = 0; s < nsamples; s++)
        {
        for ( row = this->OutputExtent[2]; row <= this->OutputExtent[3]; row ++ )
          {
          if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
            {
            fileRow = row;
            }
          else
            {
            fileRow = height - row - 1;
            }
          if (TIFFReadScanline(this->GetInternalImage()->Image, buf, fileRow, s) <= 0)
            {
            vtkErrorMacro( << "Problem reading the row: " << fileRow );
            break;
            }
          image = reinterpret_cast<unsigned short*>(out) 
            + (row-this->OutputExtent[2])*this->OutputIncrements[1];
          cc = this->OutputExtent[0] * this->GetInternalImage()->SamplesPerPixel;
          for (int ix = this->OutputExtent[0]; ix <= this->OutputExtent[1]; ++ix)
            {
            inc = this->EvaluateImageAt( image, 
                                         static_cast<unsigned short *>(buf) +
                                         cc );
            image += this->OutputIncrements[0];
            cc += this->GetInternalImage()->SamplesPerPixel;
            }
          }
        }
      }
    }
  // Short type
  else if(this->GetDataScalarType() == VTK_SHORT)
    {
    isize /= 2;
    short* image;
    if (this->GetInternalImage()->PlanarConfig == PLANARCONFIG_CONTIG) 
      {
      for ( row = this->OutputExtent[2]; row <= this->OutputExtent[3]; row ++ )
        {
        if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
          {
          fileRow = row;
          }
        else
          {
          fileRow = height - row - 1;
          }
        if (TIFFReadScanline(this->GetInternalImage()->Image, buf, fileRow, 0) <= 0)
          {
          vtkErrorMacro( << "Problem reading the row: " << fileRow );
          break;
          }
          
        image = reinterpret_cast<short*>(out) 
          + (row-this->OutputExtent[2])*this->OutputIncrements[1];
        cc = this->OutputExtent[0] * this->GetInternalImage()->SamplesPerPixel;
        for (int ix = this->OutputExtent[0]; ix <= this->OutputExtent[1]; ++ix)
          {
          inc = this->EvaluateImageAt(image, 
                                      static_cast<short *>(buf) +
                                      cc );
          image += this->OutputIncrements[0];
          cc += this->GetInternalImage()->SamplesPerPixel;
          }
        }
      }
    else if(this->GetInternalImage()->PlanarConfig == PLANARCONFIG_SEPARATE)
      {
      unsigned long s, nsamples;
      TIFFGetField(this->GetInternalImage()->Image, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
      for (s = 0; s < nsamples; s++)
        {
        for ( row = this->OutputExtent[2]; row <= this->OutputExtent[3]; row ++ )
          {
          if (this->GetInternalImage()->Orientation == ORIENTATION_TOPLEFT)
            {
            fileRow = row;
            }
          else
            {
            fileRow = height - row - 1;
            }
          if (TIFFReadScanline(this->GetInternalImage()->Image, buf, fileRow, s) <= 0)
            {
            vtkErrorMacro( << "Problem reading the row: " << fileRow );
            break;
            }

          image = reinterpret_cast<short*>(out) 
            + (row-this->OutputExtent[2])*this->OutputIncrements[1];
          cc = this->OutputExtent[0] * this->GetInternalImage()->SamplesPerPixel;
          for (int ix = this->OutputExtent[0]; ix <= this->OutputExtent[1]; ++ix)
            {
            inc = this->EvaluateImageAt( image, 
                                         static_cast<short *>(buf) +
                                         cc );
            image += this->OutputIncrements[0];
            cc += this->GetInternalImage()->SamplesPerPixel;
            }
          }
        }
      }
    }
  _TIFFfree(buf); 
}


//-------------------------------------------------------------------------
void vtkTIFFReader::ReadImageInternal( void* vtkNotUsed(in), void* outPtr,
                                       int* outExt,
                                       unsigned int vtkNotUsed(size) )
{
  int width  = this->GetInternalImage()->Width;
  int height = this->GetInternalImage()->Height;
  this->OutputExtent = outExt;

  if ( !this->GetInternalImage()->CanRead() )
    {
    // Why do we read the image for the ! CanRead case?
    uint32 *tempImage = static_cast<uint32*>( outPtr );

    if ( this->OutputExtent[0] != 0 ||
         this->OutputExtent[1] != width -1 ||
         this->OutputExtent[2] != 0 ||
         this->OutputExtent[3] != height-1 )
      {
      tempImage = new uint32[ width * height ];
      }
    // This should really be fixed to read only the rows necessary.
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
        if ( xx >= this->OutputExtent[0] &&
             xx <= this->OutputExtent[1] &&
             yy >= this->OutputExtent[2] &&
             yy <= this->OutputExtent[3] )
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

  switch ( format ) 
    {
    case vtkTIFFReader::GRAYSCALE:
    case vtkTIFFReader::RGB:
    case vtkTIFFReader::PALETTE_RGB:
    case vtkTIFFReader::PALETTE_GRAYSCALE:
      this->ReadGenericImage( outPtr, width, height );
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
        if(this->GetDataScalarType() == VTK_UNSIGNED_SHORT)
          {
          unsigned short *image_us = (unsigned short*)out;
          unsigned short *source_us = (unsigned short*)in;
          *image_us = *source_us;
          }
        else if(this->GetDataScalarType() == VTK_SHORT)
          {
          short *image_us = (short*)out;
          short *source_us = (short*)in;
          *image_us = *source_us;
          }
        else if(this->GetDataScalarType() == VTK_CHAR)
          {
          char *image_us = (char*)out;
          char *source_us = (char*)in;
          *image_us = *source_us;
          }
        else
          {
          *image = *source;
          }
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
      if(this->GetDataScalarType() == VTK_UNSIGNED_SHORT)
        {
        unsigned short *image_us = (unsigned short*)out;
        unsigned short *source_us = (unsigned short*)in;
        this->GetColor(*source_us, &red, &green, &blue);
        *(image_us)   = red << 8;
        *(image_us+1) = green << 8;
        *(image_us+2) = blue << 8;
        }
      else if(this->GetDataScalarType() == VTK_SHORT)
        {
        short *image_us = (short*)out;
        short *source_us = (short*)in;
        this->GetColor(*source_us, &red, &green, &blue);
        *(image_us)   = red << 8;
        *(image_us+1) = green << 8;
        *(image_us+2) = blue << 8;
        }
      else if(this->GetDataScalarType() == VTK_CHAR)
        {
        this->GetColor(*source, &red, &green, &blue);
        *(image)   = static_cast<char>(red >> 8);
        *(image+1) = static_cast<char>(green >> 8);
        *(image+2) = static_cast<char>(blue >> 8);
        }
      else
        {
        this->GetColor(*source, &red, &green, &blue);
        *(image)   = static_cast<unsigned char>(red >> 8);
        *(image+1) = static_cast<unsigned char>(green >> 8);
        *(image+2) = static_cast<unsigned char>(blue >> 8);
        }   
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
  os << indent << "OrientationType: " << this->OrientationType << endl;
  os << indent << "OrientationTypeSpecifiedFlag: " << this->OrientationTypeSpecifiedFlag << endl;
  os << indent << "OriginSpecifiedFlag: " << this->OriginSpecifiedFlag << endl;
  os << indent << "SpacingSpecifiedFlag: " << this->SpacingSpecifiedFlag << endl;
}
