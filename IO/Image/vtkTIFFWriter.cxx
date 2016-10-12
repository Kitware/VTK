/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTIFFWriter.h"

#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtk_tiff.h"

vtkStandardNewMacro(vtkTIFFWriter);

//----------------------------------------------------------------------------
vtkTIFFWriter::vtkTIFFWriter()
  : TIFFPtr(NULL), Compression(PackBits), Width(0), Height(0), Pages(0),
    XResolution(-1.0), YResolution(-1.0)
{
}

//----------------------------------------------------------------------------
void vtkTIFFWriter::Write()
{
  // make sure the latest input is available.
  this->GetInputAlgorithm()->Update();
  this->SetErrorCode(vtkErrorCode::NoError);
  // Error checking
  if (this->GetInput() == NULL)
  {
    vtkErrorMacro(<<"Write: Please specify an input!");
    return;
  }
  if (!this->FileName && !this->FilePattern)
  {
    vtkErrorMacro(<<"Write: Please specify either a FileName or a file prefix and pattern");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return;
  }

  // Make sure the file name is allocated - inherited from parent class,
  // would be great to rewrite in more modern C++, but sticking with superclass
  // for now to maintain behavior without rewriting/translating code.
  size_t internalFileNameSize = (this->FileName ? strlen(this->FileName) : 1) +
            (this->FilePrefix ? strlen(this->FilePrefix) : 1) +
            (this->FilePattern ? strlen(this->FilePattern) : 1) + 256;
  this->InternalFileName = new char[internalFileNameSize];
  this->InternalFileName[0] = 0;
  int bytesPrinted = 0;
  // determine the name
  if (this->FileName)
  {
    bytesPrinted = snprintf(this->InternalFileName, internalFileNameSize,
      "%s",this->FileName);
  }
  else
  {
    if (this->FilePrefix)
    {
      bytesPrinted = snprintf(this->InternalFileName, internalFileNameSize,
        this->FilePattern, this->FilePrefix, this->FileNumber);
    }
    else
    {
      bytesPrinted = snprintf(this->InternalFileName, internalFileNameSize,
        this->FilePattern,this->FileNumber);
    }
  }
  if (static_cast<size_t>(bytesPrinted) >= internalFileNameSize)
  {
    // Add null terminating character just to be safe.
    this->InternalFileName[internalFileNameSize - 1] = 0;
    vtkWarningMacro("Filename has been truncated.");
  }

  // Fill in image information.
  this->GetInputExecutive(0, 0)->UpdateInformation();
  int *wExtent;
  wExtent = vtkStreamingDemandDrivenPipeline::GetWholeExtent(
    this->GetInputInformation(0, 0));
  this->FilesDeleted = 0;
  this->UpdateProgress(0.0);

  this->WriteFileHeader(0, this->GetInput(), wExtent);
  this->WriteFile(0, this->GetInput(), wExtent, 0);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    this->DeleteFiles();
  }
  else
  {
    this->WriteFileTrailer(0, 0);
  }

  delete [] this->InternalFileName;
  this->InternalFileName = NULL;
}

//----------------------------------------------------------------------------
void vtkTIFFWriter::WriteFileHeader(ofstream *, vtkImageData *data, int wExt[6])
{
  int dims[3];
  data->GetDimensions(dims);
  int scomponents = data->GetNumberOfScalarComponents();
  int stype = data->GetScalarType();
  uint32 rowsperstrip = (uint32) -1;

  int bps;
  switch (stype)
  {
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
    case VTK_UNSIGNED_CHAR:
      bps = 8;
      break;
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      bps = 16;
      break;
    case VTK_FLOAT:
      bps = 32;
      break;
    default:
      vtkErrorMacro(<< "Unsupported data type: " << data->GetScalarTypeAsString());
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      return;
  }

  int predictor;

  // Find the width/height of the images
  this->Width = wExt[1] - wExt[0] + 1;
  this->Height = wExt[3] - wExt[2] + 1;
  // Check if we need to write an image stack (pages > 2).
  this->Pages = wExt[5] - wExt[4] + 1;

  // Check the resolution too, assume we store it in metric (as in reader).
  this->XResolution = 10.0 / data->GetSpacing()[0];
  this->YResolution = 10.0 / data->GetSpacing()[1];

  TIFF* tif = TIFFOpen(this->InternalFileName, "w");

  if (!tif)
  {
    this->TIFFPtr = 0;
    return;
  }
  this->TIFFPtr = tif;

  // Let the volume do its metadata, keep existing for 2D images.
  if (this->Pages > 1)
  {
    return;
  }

  uint32 w = this->Width;
  uint32 h = this->Height;
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, scomponents);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps); // Fix for stype
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  if (stype == VTK_FLOAT)
  {
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
  }

  if ( scomponents > 3 )
  {
    // if number of scalar components is greater than 3, that means we assume
    // there is alpha.
    uint16 extra_samples = scomponents-3;
    uint16 *sample_info = new uint16[scomponents-3];
    sample_info[0]=EXTRASAMPLE_ASSOCALPHA;
    int cc;
    for ( cc = 1; cc < scomponents-3; cc ++ )
    {
      sample_info[cc] = EXTRASAMPLE_UNSPECIFIED;
    }
    TIFFSetField(tif,TIFFTAG_EXTRASAMPLES,extra_samples,
      sample_info);
    delete [] sample_info;
  }

  int compression;
  switch ( this->Compression )
  {
    case vtkTIFFWriter::PackBits: compression = COMPRESSION_PACKBITS; break;
    case vtkTIFFWriter::JPEG:     compression = COMPRESSION_JPEG; break;
    case vtkTIFFWriter::Deflate:  compression = COMPRESSION_DEFLATE; break;
    case vtkTIFFWriter::LZW:      compression = COMPRESSION_LZW; break;
    default: compression = COMPRESSION_NONE;
  }
  //compression = COMPRESSION_JPEG;
  TIFFSetField(tif, TIFFTAG_COMPRESSION, compression); // Fix for compression
  uint16 photometric =
    (scomponents == 1 ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);
  if ( compression == COMPRESSION_JPEG )
  {
    TIFFSetField(tif, TIFFTAG_JPEGQUALITY, 75); // Parameter
    TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
    photometric = PHOTOMETRIC_YCBCR;
  }
  else if ( compression == COMPRESSION_LZW )
  {
    predictor = 2;
    TIFFSetField(tif, TIFFTAG_PREDICTOR, predictor);
    vtkErrorMacro("LZW compression is patented outside US so it is disabled");
  }
  else if ( compression == COMPRESSION_DEFLATE )
  {
    predictor = 2;
    TIFFSetField(tif, TIFFTAG_PREDICTOR, predictor);
  }

  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric); // Fix for scomponents
  TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,
    TIFFDefaultStripSize(tif, rowsperstrip));
  if (this->XResolution > 0.0 && this->YResolution > 0.0)
  {
    TIFFSetField(tif, TIFFTAG_XRESOLUTION, this->XResolution);
    TIFFSetField(tif, TIFFTAG_YRESOLUTION, this->YResolution);
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER);
  }
}

//----------------------------------------------------------------------------
void vtkTIFFWriter::WriteFile(ofstream *, vtkImageData *data,
                              int extent[6], int*)
{
  // Make sure we actually have data.
  if (!data->GetPointData()->GetScalars())
  {
    vtkErrorMacro(<< "Could not get data from input.");
    return;
  }

  TIFF* tif = reinterpret_cast<TIFF*>(this->TIFFPtr);
  if (!tif)
  {
    vtkErrorMacro("Problem writing file.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
  }

  // take into consideration the scalar type
  if( data->GetScalarType() != VTK_UNSIGNED_CHAR
   && data->GetScalarType() != VTK_UNSIGNED_SHORT
   && data->GetScalarType() != VTK_FLOAT)
  {
    vtkErrorMacro("TIFFWriter only accepts unsigned char/short or float scalars!");
    return;
  }

  if (this->Pages > 1)
  {
    // Call the correct templated function for the input
    void *inPtr = data->GetScalarPointer();

    switch (data->GetScalarType())
    {
      vtkTemplateMacro(this->WriteVolume((VTK_TT *)(inPtr)));
      default:
        vtkErrorMacro("UpdateFromFile: Unknown data type");
    }
  }
  else
  {
    // Now write the image for the current page/directory element.
    int row = 0;
    for (int idx2 = extent[4]; idx2 <= extent[5]; ++idx2)
    {
      for (int idx1 = extent[3]; idx1 >= extent[2]; idx1--)
      {
        void *ptr = data->GetScalarPointer(extent[0], idx1, idx2);
        if (TIFFWriteScanline(tif, static_cast<unsigned char*>(ptr), row, 0) < 0)
        {
          this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
          break;
        }
        ++row;
      }
    }
  }
}

//----------------------------------------------------------------------------
template<typename T>
void vtkTIFFWriter::WriteVolume(T* buffer)
{
  TIFF* tif = reinterpret_cast<TIFF*>(this->TIFFPtr);
  if (!tif)
  {
    vtkErrorMacro("Problem writing volume.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
  }
  int width = this->Width;
  int height = this->Height;
  int pages = this->Pages;

  uint32 w = width;
  uint32 h = height;
  int bitsPerSample = sizeof(T) * 8;

  for (int page = 0; page < pages; ++page)
  {
    this->UpdateProgress(static_cast<double>(page + 1) / pages);

    // TIFF directory set up/tags.
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bitsPerSample);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    int compression;
    switch ( this->Compression )
    {
      case vtkTIFFWriter::PackBits:
        compression = COMPRESSION_PACKBITS;
        break;
      case vtkTIFFWriter::JPEG:
        compression = COMPRESSION_JPEG;
        break;
      case vtkTIFFWriter::Deflate:
        compression = COMPRESSION_DEFLATE;
        break;
      case vtkTIFFWriter::LZW:
        compression = COMPRESSION_LZW;
        break;
      default:
        compression = COMPRESSION_NONE;
    }
    TIFFSetField(tif, TIFFTAG_COMPRESSION, compression); // Fix for compression
    if (compression == COMPRESSION_LZW)
    {
       TIFFSetField(tif, TIFFTAG_PREDICTOR, 2);
       vtkErrorMacro("LZW compression is patented outside US so it is disabled");
    }
     else if (compression == COMPRESSION_DEFLATE)
     {
       TIFFSetField(tif, TIFFTAG_PREDICTOR, 2);
     }

    if (bitsPerSample == 32)
    {
      TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    }

    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    uint32 rowsperstrip = (uint32) -1;
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,
      TIFFDefaultStripSize(tif, rowsperstrip));
    if (this->XResolution > 0.0 && this->YResolution > 0.0)
    {
      TIFFSetField(tif, TIFFTAG_XRESOLUTION, this->XResolution);
      TIFFSetField(tif, TIFFTAG_YRESOLUTION, this->YResolution);
      TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER);
    }

    // Extra pieces for multidirectory files.
    TIFFSetField(tif, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
    TIFFSetField(tif, TIFFTAG_PAGENUMBER, page, pages);

    T* volume = buffer;
    volume += width * height * page;
    for (int i = 0; i < height; ++i)
    {
      T* tmp = volume + i * width;
      if (TIFFWriteScanline(tif, reinterpret_cast<char*>(tmp), i, 0) < 0)
      {
        this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
        return;
      }
    }
    if (!TIFFWriteDirectory(tif))
    {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return;
    }
  }
}

//----------------------------------------------------------------------------
void vtkTIFFWriter::WriteFileTrailer(ofstream *, vtkImageData *)
{
  TIFF* tif = reinterpret_cast<TIFF*>(this->TIFFPtr);
  if( tif)
  {
    TIFFClose(tif);
  }
  else
  {
    vtkErrorMacro("Problem writing trailer.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
  }

  this->TIFFPtr = 0;
}

//----------------------------------------------------------------------------
void vtkTIFFWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Compression: ";
  if ( this->Compression == vtkTIFFWriter::PackBits )
  {
    os << "Pack Bits\n";
  }
  else if ( this->Compression == vtkTIFFWriter::JPEG )
  {
    os << "JPEG\n";
  }
  else if ( this->Compression == vtkTIFFWriter::Deflate )
  {
    os << "Deflate\n";
  }
  else if ( this->Compression == vtkTIFFWriter::LZW )
  {
    os << "LZW\n";
  }
  else //if ( this->Compression == vtkTIFFWriter::NoCompression )
  {
    os << "No Compression\n";
  }
}
