/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNIFTIImageReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNIFTIImageReader.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkByteSwap.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkVersion.h"

#include "vtksys/SystemTools.hxx"
#include "vtksys/ios/sstream"

// Header for NIFTI
#include "vtkNIFTIImageHeader.h"
#include "vtkNIFTIImagePrivate.h"

// Header for zlib
#include "vtk_zlib.h"

#include <ctype.h>
#include <string.h>
#include <string>

vtkStandardNewMacro(vtkNIFTIImageReader);

//----------------------------------------------------------------------------
vtkNIFTIImageReader::vtkNIFTIImageReader()
{
  for (int i = 0; i < 8; i++)
    {
    this->Dim[i] = 0;
    }
  for (int i = 0; i < 8; i++)
    {
    this->PixDim[i] = 1.0;
    }
  this->TimeAsVector = false;
  this->RescaleSlope = 1.0;
  this->RescaleIntercept = 0.0;
  this->QFac = 1.0;
  this->QFormMatrix = 0;
  this->SFormMatrix = 0;
  this->NIFTIHeader = 0;
}

//----------------------------------------------------------------------------
vtkNIFTIImageReader::~vtkNIFTIImageReader()
{
  if (this->QFormMatrix)
    {
    this->QFormMatrix->Delete();
    }
  if (this->SFormMatrix)
    {
    this->SFormMatrix->Delete();
    }
  if (this->NIFTIHeader)
    {
    this->NIFTIHeader->Delete();
    }
}

//----------------------------------------------------------------------------
namespace { // anonymous namespace

void vtkNIFTIImageReaderSwapHeader(nifti_1_header *hdr)
{
  // Common to NIFTI and Analyze 7.5
  vtkByteSwap::SwapVoidRange(&hdr->sizeof_hdr,    1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->extents,       1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->session_error, 1, 2);
  vtkByteSwap::SwapVoidRange(hdr->dim,            8, 2);
  vtkByteSwap::SwapVoidRange(&hdr->intent_p1,     1, 4); // unused in 7.5
  vtkByteSwap::SwapVoidRange(&hdr->intent_p2,     1, 4); // unused in 7.5
  vtkByteSwap::SwapVoidRange(&hdr->intent_p3,     1, 4); // unused in 7.5
  vtkByteSwap::SwapVoidRange(&hdr->intent_code,   1, 2); // unused in 7.5
  vtkByteSwap::SwapVoidRange(&hdr->datatype,      1, 2);
  vtkByteSwap::SwapVoidRange(&hdr->bitpix,        1, 2);
  vtkByteSwap::SwapVoidRange(&hdr->slice_start,   1, 2); // dim_un0 in 7.5
  vtkByteSwap::SwapVoidRange(hdr->pixdim,         8, 4);
  vtkByteSwap::SwapVoidRange(&hdr->vox_offset,    1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->scl_slope,     1, 4); // unused in 7.5
  vtkByteSwap::SwapVoidRange(&hdr->scl_inter,     1, 4); // unused in 7.5
  vtkByteSwap::SwapVoidRange(&hdr->slice_end,     1, 2); // unused in 7.5
  vtkByteSwap::SwapVoidRange(&hdr->cal_max,       1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->cal_min,       1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->slice_duration,1, 4); // compressed in 7.5
  vtkByteSwap::SwapVoidRange(&hdr->toffset,       1, 4); // verified in 7.5
  vtkByteSwap::SwapVoidRange(&hdr->glmax,         1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->glmin,         1, 4);

  // All NIFTI-specific (meaning is totally different in Analyze 7.5)
  if (strncmp(hdr->magic, "ni1", 4) == 0 ||
      strncmp(hdr->magic, "n+1", 4) == 0)
    {
    vtkByteSwap::SwapVoidRange(&hdr->qform_code,    1, 2);
    vtkByteSwap::SwapVoidRange(&hdr->sform_code,    1, 2);
    vtkByteSwap::SwapVoidRange(&hdr->quatern_b,     1, 4);
    vtkByteSwap::SwapVoidRange(&hdr->quatern_c,     1, 4);
    vtkByteSwap::SwapVoidRange(&hdr->quatern_d,     1, 4);
    vtkByteSwap::SwapVoidRange(&hdr->qoffset_x,     1, 4);
    vtkByteSwap::SwapVoidRange(&hdr->qoffset_y,     1, 4);
    vtkByteSwap::SwapVoidRange(&hdr->qoffset_z,     1, 4);
    vtkByteSwap::SwapVoidRange(hdr->srow_x,         4, 4);
    vtkByteSwap::SwapVoidRange(hdr->srow_y,         4, 4);
    vtkByteSwap::SwapVoidRange(hdr->srow_z,         4, 4);
    }
}

void vtkNIFTIImageReaderSwapHeader(nifti_2_header *hdr)
{
  vtkByteSwap::SwapVoidRange(&hdr->sizeof_hdr,    1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->datatype,      1, 2);
  vtkByteSwap::SwapVoidRange(&hdr->bitpix,        1, 2);
  vtkByteSwap::SwapVoidRange(hdr->dim,            8, 8);
  vtkByteSwap::SwapVoidRange(&hdr->intent_p1,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->intent_p2,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->intent_p3,     1, 8);
  vtkByteSwap::SwapVoidRange(hdr->pixdim,         8, 8);
  vtkByteSwap::SwapVoidRange(&hdr->vox_offset,    1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->scl_slope,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->scl_inter,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->cal_max,       1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->cal_min,       1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->slice_duration,1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->toffset,       1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->slice_start,   1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->slice_end,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->qform_code,    1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->sform_code,    1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->quatern_b,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->quatern_c,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->quatern_d,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->qoffset_x,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->qoffset_y,     1, 8);
  vtkByteSwap::SwapVoidRange(&hdr->qoffset_z,     1, 8);
  vtkByteSwap::SwapVoidRange(hdr->srow_x,         4, 8);
  vtkByteSwap::SwapVoidRange(hdr->srow_y,         4, 8);
  vtkByteSwap::SwapVoidRange(hdr->srow_z,         4, 8);
  vtkByteSwap::SwapVoidRange(&hdr->slice_code,    1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->xyzt_units,    1, 4);
  vtkByteSwap::SwapVoidRange(&hdr->intent_code,   1, 4);
}

} // end anonymous namespace

//----------------------------------------------------------------------------
vtkNIFTIImageHeader *vtkNIFTIImageReader::GetNIFTIHeader()
{
  if (!this->NIFTIHeader)
    {
    this->NIFTIHeader = vtkNIFTIImageHeader::New();
    }
  return this->NIFTIHeader;
}

//----------------------------------------------------------------------------
void vtkNIFTIImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TimeAsVector: "
     << (this->TimeAsVector ? "On\n" : "Off\n");
  os << indent << "TimeDimension: " << this->GetTimeDimension() << "\n";
  os << indent << "TimeSpacing: " << this->GetTimeSpacing() << "\n";
  os << indent << "RescaleSlope: " << this->RescaleSlope << "\n";
  os << indent << "RescaleIntercept: " << this->RescaleIntercept << "\n";
  os << indent << "QFac: " << this->QFac << "\n";

  os << indent << "QFormMatrix:";
  if (this->QFormMatrix)
    {
    double mat[16];
    vtkMatrix4x4::DeepCopy(mat, this->QFormMatrix);
    for (int i = 0; i < 16; i++)
      {
      os << " " << mat[i];
      }
    os << "\n";
    }
  else
    {
    os << " (none)\n";
    }

  os << indent << "SFormMatrix:";
  if (this->SFormMatrix)
    {
    double mat[16];
    vtkMatrix4x4::DeepCopy(mat, this->SFormMatrix);
    for (int i = 0; i < 16; i++)
      {
      os << " " << mat[i];
     }
    os << "\n";
    }
  else
    {
    os << " (none)\n";
    }

  os << indent << "NIFTIHeader:" << (this->NIFTIHeader ? "\n" : " (none)\n");
}

//----------------------------------------------------------------------------
bool vtkNIFTIImageReader::CheckExtension(
  const char *filename, const char *ext)
{
  if (strlen(ext) == 4 && ext[0] == '.')
    {
    size_t n = strlen(filename);
    if (n > 2 && filename[n-3] == '.' &&
        tolower(filename[n-2]) == 'g' &&
        tolower(filename[n-1]) == 'z')
      {
      n -= 3;
      }
    if (n > 3 && filename[n-4] == '.' &&
        tolower(filename[n-3]) == tolower(ext[1]) &&
        tolower(filename[n-2]) == tolower(ext[2]) &&
        tolower(filename[n-1]) == tolower(ext[3]))
      {
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
char *vtkNIFTIImageReader::ReplaceExtension(
  const char *filename, const char *ext1, const char *ext2)
{
  char *newname = 0;

  if (strlen(ext1) == 4 && ext1[0] == '.' &&
      strlen(ext2) == 4 && ext2[0] == '.')
    {
    size_t n = strlen(filename);
    size_t m = n;
    newname = new char[n+4];
    strcpy(newname, filename);

    // check for trailing .gz
    if (n > 2 && filename[n-3] == '.' &&
        tolower(filename[n-2]) == 'g' &&
        tolower(filename[n-1]) == 'z')
      {
      m = n - 3;
      }

    if (vtkNIFTIImageReader::CheckExtension(filename, ext1))
      {
      // replace the extension
      if (isupper(filename[m-3]))
        {
        newname[m-3] = toupper(ext2[1]);
        newname[m-2] = toupper(ext2[2]);
        newname[m-1] = toupper(ext2[3]);
        }
      else
        {
        newname[m-3] = tolower(ext2[1]);
        newname[m-2] = tolower(ext2[2]);
        newname[m-1] = tolower(ext2[3]);
        }
      }

    // existence of file
    for (int i = 0; i < 2; i++)
      {
      if (vtksys::SystemTools::FileExists(newname))
        {
        return newname;
        }
      if (i == 0)
        {
        if (m < n)
          {
          // try again without the ".gz"
          newname[m] = '\0';
          n = m;
          }
        else
          {
          // try again with the ".gz"
          newname[m] = '.';
          newname[m+1] = (isupper(newname[m-3]) ? 'G' : 'g');
          newname[m+2] = (isupper(newname[m-3]) ? 'Z' : 'z');
          newname[m+3] = '\0';
          }
        }
      }

    delete [] newname;
    newname = 0;
    }

  return newname;
}

//----------------------------------------------------------------------------
int vtkNIFTIImageReader::CheckNIFTIVersion(const nifti_1_header *hdr)
{
  int version = 0;

  // Check for NIFTIv2.  The NIFTIv2 magic number is stored where
  // the data_type appears in the NIFTIv1 header.
  if (hdr->data_type[0] == 'n' &&
      (hdr->data_type[1] == '+' || hdr->data_type[1] == 'i') &&
      (hdr->data_type[2] >= '2' && hdr->data_type[2] <= '9') &&
      hdr->data_type[3] == '\0')
    {
    version = (hdr->data_type[2] - '0');

    if (hdr->data_type[4] != '\r' ||
        hdr->data_type[5] != '\n' ||
        hdr->data_type[6] != '\032' ||
        hdr->data_type[7] != '\n')
      {
      // Indicate that file was corrupted by newline conversion
      version = -version;
      }
    }
  // Check for NIFTIv1
  else if (hdr->magic[0] == 'n' &&
      (hdr->magic[1] == '+' || hdr->magic[1] == 'i') &&
      hdr->magic[2] == '1' &&
      hdr->magic[3] == '\0')
    {
    version = 1;
    }

  return version;
}

//----------------------------------------------------------------------------
bool vtkNIFTIImageReader::CheckAnalyzeHeader(const nifti_1_header *hdr)
{
  if (hdr->sizeof_hdr == 348 || // Analyze 7.5 header size
      hdr->sizeof_hdr == 1543569408) // byte-swapped 348
    {
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
int vtkNIFTIImageReader::CanReadFile(const char *filename)
{
  vtkDebugMacro("Opening NIFTI file " << filename);

  char *hdrname = vtkNIFTIImageReader::ReplaceExtension(
    filename, ".img", ".hdr");

  if (hdrname == 0)
    {
    return 0;
    }

  // try opening file
  gzFile file = gzopen(hdrname, "rb");

  delete [] hdrname;

  if (!file)
    {
    return 0;
    }

  // read and check the header
  bool canRead = false;
  nifti_1_header hdr;
  int hsize = vtkNIFTIImageHeader::NIFTI1HeaderSize; // nifti_1 header size
  int rsize = gzread(file, &hdr, hsize);
  if (rsize == hsize)
    {
    int version = vtkNIFTIImageReader::CheckNIFTIVersion(&hdr);
    if (version > 0)
      {
      // NIFTI file
      canRead = true;
      }
    else if (version == 0)
      {
      // Analyze 7.5 file
      canRead = vtkNIFTIImageReader::CheckAnalyzeHeader(&hdr);
      }
    }

  gzclose(file);

  return canRead;
}

//----------------------------------------------------------------------------
int vtkNIFTIImageReader::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // Clear the error indicator.
  this->SetErrorCode(vtkErrorCode::NoError);

  // Create the header object
  if (!this->NIFTIHeader)
    {
    this->NIFTIHeader = vtkNIFTIImageHeader::New();
    }

  // default byte order is native byte order
#ifdef VTK_WORDS_BIGENDIAN
  bool isLittleEndian = false;
#else
  bool isLittleEndian = true;
#endif

  const char *filename = 0;
  char *hdrname = 0;

  if (this->FileNames)
    {
    vtkIdType n = this->FileNames->GetNumberOfValues();
    int headers = 0;
    for (vtkIdType i = 0; i < n; i++)
      {
      filename = this->FileNames->GetValue(i);
      // this checks for .hdr and .hdr.gz, case insensitive
      if (vtkNIFTIImageReader::CheckExtension(filename, ".hdr"))
        {
        if (++headers < 2)
          {
          hdrname = new char[strlen(filename) + 1];
          strcpy(hdrname, filename);
          }
        }
      }
    if (n != 2 || headers != 1)
      {
      vtkErrorMacro("There must be two files and one must be a .hdr file.");
      delete [] hdrname;
      return 0;
      }
    }
  else
    {
    filename = this->GetFileName();
    }

  if (filename == 0)
    {
    vtkErrorMacro("A FileName must be provided");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
    }

  if (hdrname == 0)
    {
    hdrname = vtkNIFTIImageReader::ReplaceExtension(
      filename, ".img", ".hdr");
    }

  if (hdrname == 0)
    {
    vtkErrorMacro("Unable to locate header for file " << filename);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return 0;
    }

  vtkDebugMacro("Opening NIFTI file " << hdrname);

  // try opening file
  gzFile file = gzopen(hdrname, "rb");

  if (!file)
    {
    vtkErrorMacro("Cannot open file " << hdrname);
    delete [] hdrname;
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return 0;
    }

  // read and check the header
  bool canRead = false;
  int niftiVersion = 0;
  nifti_1_header *hdr1 = new nifti_1_header;
  nifti_2_header hdr2obj;
  nifti_2_header *hdr2 = &hdr2obj;
  const int hsize = vtkNIFTIImageHeader::NIFTI1HeaderSize;
  int rsize = gzread(file, hdr1, hsize);
  if (rsize == hsize)
    {
    niftiVersion = vtkNIFTIImageReader::CheckNIFTIVersion(hdr1);
    if (niftiVersion >= 2)
      {
      // the header was a NIFTIv2 header
      const int h2size = vtkNIFTIImageHeader::NIFTI2HeaderSize;
      // copy what was read into the NIFTIv1 header
      memcpy(hdr2, hdr1, hsize);
      // read the remainder of the NIFTIv2 header
      rsize = gzread(file, reinterpret_cast<char *>(hdr2)+hsize, h2size-hsize);
      if (rsize == h2size-hsize)
        {
        canRead = true;
        }
      }
    else if (niftiVersion == 1)
      {
      // the header was a NIFTIv1 header
      canRead = true;
      }
    else if (niftiVersion == 0)
      {
      // Analyze 7.5 file
      canRead = vtkNIFTIImageReader::CheckAnalyzeHeader(hdr1);
      }
    }

  if (canRead)
    {
    if (niftiVersion >= 2)
      {
      if (NIFTI_NEEDS_SWAP(*hdr2))
        {
        vtkNIFTIImageReaderSwapHeader(hdr2);
        isLittleEndian = !isLittleEndian;
        }
      this->NIFTIHeader->SetHeader(hdr2);
      }
    else
      {
      if (NIFTI_NEEDS_SWAP(*hdr1))
        {
        vtkNIFTIImageReaderSwapHeader(hdr1);
        isLittleEndian = !isLittleEndian;
        }
      // convert NIFTIv1 header into NIFTIv2
      this->NIFTIHeader->SetHeader(hdr1);
      this->NIFTIHeader->GetHeader(hdr2);
      }
    }

  gzclose(file);

  // delete the NIFTIv1 header, use the NIFTIv2 header
  delete hdr1;
  hdr1 = 0;

  if (!canRead)
    {
    const char *message = (niftiVersion <= -2 ?
                           "NIfTI header has newline corruption " :
                           "Bad NIfTI header in file ");
    vtkErrorMacro(<< message << hdrname);
    this->SetErrorCode(vtkErrorCode::UnrecognizedFileTypeError);
    delete [] hdrname;
    return 0;
    }

  delete [] hdrname;

  // number of dimensions
  int ndim = hdr2->dim[0];
  if (ndim < 0 || ndim > 7)
    {
    vtkErrorMacro("NIfTI image has illegal ndim of " << ndim);
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return 0;
    }

  // sanity checks
  for (int i = 0; i < 8; i++)
    {
    // voxel spacing cannot be zero
    if (hdr2->pixdim[i] == 0)
      {
      hdr2->pixdim[i] = 1.0;
      }
    if (i > ndim)
      {
      // dimensions greater than ndim have size of 1
      hdr2->dim[i] = 1;
      }
    else if (hdr2->dim[i] < 0)
      {
      vtkErrorMacro("NIfTI image dimension " << i << " is negative");
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      return 0;
      }
    else if ((hdr2->dim[i] & 0x7fffffff) != hdr2->dim[i])
      {
      // dimension does not fit in signed int
      vtkErrorMacro("NIfTI image dimension " << i << " is larger than int32");
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      return 0;
      }
    }

  if (niftiVersion > 0)
    {
    // pass rescale info to user (do not rescale in the reader)
    this->RescaleSlope = hdr2->scl_slope;
    this->RescaleIntercept = hdr2->scl_inter;
    }
  else
    {
    // rescale information not available for Analyze 7.5
    this->RescaleSlope = 1.0;
    this->RescaleIntercept = 0.0;
    }

  // header might be extended, vox_offset says where data starts
  this->SetHeaderSize(static_cast<unsigned long>(hdr2->vox_offset));

  // endianness of data
  if (isLittleEndian)
    {
    this->SetDataByteOrderToLittleEndian();
    }
  else
    {
    this->SetDataByteOrderToBigEndian();
    }

  // NIFTI images are stored in a single file, not one file per slice
  this->SetFileDimensionality(3);

  // NIFTI uses a lower-left-hand origin
  this->FileLowerLeftOn();

  // dim
  this->SetDataExtent(0, hdr2->dim[1]-1,
                      0, hdr2->dim[2]-1,
                      0, hdr2->dim[3]-1);

  // pixdim
  this->SetDataSpacing(hdr2->pixdim[1],
                       hdr2->pixdim[2],
                       hdr2->pixdim[3]);

  // offset is part of the transform, so set origin to zero
  this->SetDataOrigin(0.0, 0.0, 0.0);

  // map the NIFTI type to a VTK type and number of components
  static const int typeMap[][3] = {
    { NIFTI_TYPE_INT8, VTK_TYPE_INT8, 1},
    { NIFTI_TYPE_UINT8, VTK_TYPE_UINT8, 1 },
    { NIFTI_TYPE_INT16, VTK_TYPE_INT16, 1 },
    { NIFTI_TYPE_UINT16, VTK_TYPE_UINT16, 1 },
    { NIFTI_TYPE_INT32, VTK_TYPE_INT32, 1 },
    { NIFTI_TYPE_UINT32, VTK_TYPE_UINT32, 1 },
    { NIFTI_TYPE_INT64, VTK_TYPE_INT64, 1 },
    { NIFTI_TYPE_UINT64, VTK_TYPE_UINT64, 1 },
    { NIFTI_TYPE_FLOAT32, VTK_TYPE_FLOAT32, 1 },
    { NIFTI_TYPE_FLOAT64, VTK_TYPE_FLOAT64, 1 },
    { NIFTI_TYPE_COMPLEX64, VTK_TYPE_FLOAT32, 2 },
    { NIFTI_TYPE_COMPLEX128, VTK_TYPE_FLOAT64, 2 },
    { NIFTI_TYPE_RGB24, VTK_TYPE_UINT8, 3 },
    { NIFTI_TYPE_RGBA32, VTK_TYPE_UINT8, 4 },
    { 0, 0, 0 }
  };

  int scalarType = 0;
  int numComponents = 0;

  for (int i = 0; typeMap[2] != 0; i++)
    {
    if (hdr2->datatype == typeMap[i][0])
      {
      scalarType = typeMap[i][1];
      numComponents = typeMap[i][2];
      break;
      }
    }

  // if loop finished without finding a match
  if (numComponents == 0)
    {
    vtkErrorMacro("Unrecognized NIFTI data type: " << hdr2->datatype);
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return 0;
    }

  // vector planes become vector components
  if (ndim >= 5)
    {
    numComponents *= hdr2->dim[5];
    }
  if (ndim >= 4 && this->TimeAsVector)
    {
    numComponents *= hdr2->dim[4];
    }

  this->SetDataScalarType(scalarType);
  this->SetNumberOfScalarComponents(numComponents);

  // Set the output information.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject::SetPointDataActiveScalarInfo(
    outInfo, this->DataScalarType, this->NumberOfScalarComponents);

  outInfo->Set(vtkDataObject::SPACING(), this->DataSpacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(),  this->DataOrigin, 3);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->DataExtent, 6);

  // copy dim for when RequestData is called
  for (int j = 0; j < 8; j++)
    {
    this->Dim[j] = hdr2->dim[j];
    this->PixDim[j] = hdr2->pixdim[j];
    }

  // === Image Orientation in NIfTI files ===
  //
  // The vtkImageData class does not provide a way of storing image
  // orientation.  So when we read a NIFTI file, we should also provide
  // the user with a 4x4 matrix that can transform VTK's data coordinates
  // into NIFTI's intended coordinate system for the image.  NIFTI defines
  // these coordinate systems as:
  // 1) NIFTI_XFORM_SCANNER_ANAT - coordinate system of the imaging device
  // 2) NIFTI_XFORM_ALIGNED_ANAT - result of registration to another image
  // 3) NIFTI_XFORM_TALAIRACH - a brain-specific coordinate system
  // 4) NIFTI_XFORM_MNI_152 - a similar brain-specific coordinate system
  //
  // NIFTI images can store orientation in two ways:
  // 1) via a quaternion (orientation and offset, i.e. rigid-body)
  // 2) via a matrix (used to store e.g. the results of registration)
  //
  // A NIFTI file can have both a quaternion (qform) and matrix (xform)
  // stored in the same file.  The NIFTI documentation recommends that
  // the qform be used to record the "scanner anatomical" coordinates
  // and that the sform, if present, be used to define a secondary
  // coordinate system, e.g. a coordinate system derived through
  // registration to a template.
  //
  // -- Quaternion Representation --
  //
  // If the "quaternion" form is used, then the following equation
  // defines the transformation from voxel indices to NIFTI's world
  // coordinates, where R is the rotation matrix computed from the
  // quaternion components:
  //
  //   [ x ]   [ R11 R12 R13 ] [ pixdim[1] * i        ]   [ qoffset_x ]
  //   [ y ] = [ R21 R22 R23 ] [ pixdim[2] * j        ] + [ qoffset_y ]
  //   [ z ]   [ R31 R32 R33 ] [ pixdim[3] * k * qfac ]   [ qoffset_z ]
  //
  // qfac is stored in pixdim[0], if it is equal to -1 then the slices
  // are stacked in reverse: VTK will have to reorder the slices in order
  // to maintain a right-handed coordinate transformation between indices
  // and coordinates.
  //
  // Let's call VTK data coordinates X,Y,Z to distinguish them from
  // the NIFTI coordinates x,y,z.  The relationship between X,Y,Z and
  // x,y,z is expressed by a 4x4 matrix M:
  //
  //   [ x ]   [ M11 M12 M13 M14 ] [ X ]
  //   [ y ] = [ M21 M22 M23 M24 ] [ Y ]
  //   [ z ]   [ M31 M32 M33 M34 ] [ Z ]
  //   [ 1 ]   [ 0   0   0   1   ] [ 1 ]
  //
  // where the VTK data coordinates X,Y,Z are related to the
  // VTK structured coordinates IJK (i.e. point indices) by:
  //
  //   X = I*Spacing[0] + Origin[0]
  //   Y = J*Spacing[1] + Origin[1]
  //   Z = K*Spacing[2] + Origin[2]
  //
  // Now let's consider: when we read a NIFTI image, how should we set
  // the Spacing, the Origin, and the matrix M?  Let's consider the
  // cases:
  //
  // 1) If there is no qform, then R is identity and qoffset is zero,
  //    and qfac will be 1 (never -1).  So:
  //      I,J,K = i,j,k, Spacing = pixdim, Origin = 0, M = Identity
  //
  // 2) If there is a qform, and qfac is 1, then:
  //
  //    I,J,K = i,j,k (i.e. voxel order in VTK same as in NIFTI)
  //
  //    Spacing[0] = pixdim[1]
  //    Spacing[1] = pixdim[2]
  //    Spacing[2] = pixdim[3]
  //
  //    Origin[0] = 0.0
  //    Origin[1] = 0.0
  //    Origin[2] = 0.0
  //
  //        [ R11 R12 R13 qoffset_x ]
  //    M = [ R21 R22 R23 qoffset_y ]
  //        [ R31 R32 R33 qoffset_z ]
  //        [ 0   0   0   1         ]
  //
  //    Note that we cannot store qoffset in the origin.  That would
  //    be mathematically incorrect.  It would only give us the right
  //    offset when R is the identity matrix.
  //
  // 3) If there is a qform and qfac is -1, then the situation is more
  //    compilcated.  We have three choices, each of which is a compromise:
  //    a) we can use Spacing[2] = qfac*pixdim[3], i.e. use a negative
  //       slice spacing, which might cause some VTK algorithms to
  //       misbehave (the VTK tests only use images with positive spacing).
  //    b) we can use M13 = -R13, M23 = -R23, M33 = -R33 i.e. introduce
  //       a flip into the matrix, which is very bad for VTK rendering
  //       algorithms and should definitely be avoided.
  //    c) we can reverse the order of the slices in VTK relative to
  //       NIFTI, which allows us to preserve positive spacing and retain
  //       a well-behaved rotation matrix, by using these equations:
  //
  //         J = number_of_slices - j - 1
  //
  //         M14 = qoffset_x - (number_of_slices - 1)*pixdim[3]*R13
  //         M24 = qoffset_y - (number_of_slices - 1)*pixdim[3]*R23
  //         M34 = qoffset_z - (number_of_slices - 1)*pixdim[3]*R33
  //
  //       This will give us data that will be well-behaved in VTK, at
  //       the expense of making VTK slice numbers not match with
  //       the original NIFTI slice numbers.  NIFTY slice 0 will become
  //       VTK slice N-1, and the order will be reversed.
  //
  // -- Matrix Representation --
  //
  // If the "matrix" form is used, then pixdim[] is ignored, and the
  // voxel spacing is implicitly stored in the matrix.  In addition,
  // the matrix may have a negative determinant, there is no "qfac"
  // flip-factor as there is in the quaternion representation.
  //
  // Let S be the matrix stored in the NIFTI header, and let M be our
  // desired coordinate tranformation from VTK data coordinates X,Y,Z
  // to NIFTI data coordinates x,y,z (see discussion above for more
  // information).  Let's consider the cases where the determinant
  // is positive, or negative.
  //
  // 1) If the determinant is positive, we will factor the spacing
  //    (but not the origin) out of the matrix.
  //
  //    Spacing[0] = pixdim[1]
  //    Spacing[1] = pixdim[2]
  //    Spacing[2] = pixdim[3]
  //
  //    Origin[0] = 0.0
  //    Origin[1] = 0.0
  //    Origin[2] = 0.0
  //
  //         [ S11/pixdim[1] S12/pixdim[2] S13/pixdim[3] S14 ]
  //    M  = [ S21/pixdim[1] S22/pixdim[2] S23/pixdim[3] S24 ]
  //         [ S31/pixdim[1] S32/pixdim[2] S33/pixdim[3] S34 ]
  //         [ 0             0             0             1   ]
  //
  // 2) If the determinant is negative, then we face the same choices
  //    as when qfac is -1 for the quaternion transformation.  We can:
  //    a) use a negative Z spacing and multiply the 3rd column of M by -1
  //    b) keep the matrix as is (with a negative determinant)
  //    c) reorder the slices, multiply the 3rd column by -1, and adjust
  //       the 4th column of the matrix:
  //
  //         M14 = S14 - (number_of_slices - 1)*S13
  //         M24 = S24 - (number_of_slices - 1)*S23
  //         M34 = S34 - (number_of_slices - 1)*S33
  //
  //       The third choice will provide a VTK image that has positive
  //       spacing and a matrix with a positive determinant.
  //
  // -- Analyze 7.5 Orientation --
  //
  // This reader provides only bare-bones backwards compatibility with
  // the Analyze 7.5 file header.  We do not orient these files.

  // Initialize
  this->QFac = 1.0;
  if (this->QFormMatrix)
    {
    this->QFormMatrix->Delete();
    this->QFormMatrix = NULL;
    }
  if (this->SFormMatrix)
    {
    this->SFormMatrix->Delete();
    this->SFormMatrix = NULL;
    }

  // Set the QFormMatrix from the quaternion data in the header.
  // See the long discussion above for more information.
  if (niftiVersion > 0 && hdr2->qform_code > 0)
    {
    double mmat[16];
    double rmat[3][3];
    double quat[4];

    quat[1] = hdr2->quatern_b;
    quat[2] = hdr2->quatern_c;
    quat[3] = hdr2->quatern_d;

    quat[0] = 1.0 - quat[1]*quat[1] - quat[2]*quat[2] - quat[3]*quat[3];
    if (quat[0] > 0.0)
      {
      quat[0] = sqrt(quat[0]);
      }
    else
      {
      quat[0] = 0.0;
      }

    vtkMath::QuaternionToMatrix3x3(quat, rmat);

    // If any matrix values are close to zero, then they should actually
    // be zero but aren't due to limited numerical precision in the
    // quaternion-to-matrix conversion.
    const double tol = 2.384185791015625e-07; // 2**-22
    for (int i = 0; i < 3; i++)
      {
      for (int j = 0; j < 3; j++)
        {
        if (fabs(rmat[i][j]) < tol)
          {
          rmat[i][j] = 0.0;
          }
        }
      vtkMath::Normalize(rmat[i]);
      }

    // first row
    mmat[0] = rmat[0][0];
    mmat[1] = rmat[0][1];
    mmat[2] = rmat[0][2];
    mmat[3] = hdr2->qoffset_x;

    // second row
    mmat[4] = rmat[1][0];
    mmat[5] = rmat[1][1];
    mmat[6] = rmat[1][2];
    mmat[7] = hdr2->qoffset_y;

    // third row
    mmat[8] = rmat[2][0];
    mmat[9] = rmat[2][1];
    mmat[10] = rmat[2][2];
    mmat[11] = hdr2->qoffset_z;

    mmat[12] = 0.0;
    mmat[13] = 0.0;
    mmat[14] = 0.0;
    mmat[15] = 1.0;

    this->QFac = ((hdr2->pixdim[0] < 0) ? -1.0 : 1.0);

    if (this->QFac < 0)
      {
      // We will be reversing the order of the slices, so the first VTK
      // slice will be at the position of the last NIfTI slice, and we
      // must adjust the offset to compensate for this.
      mmat[3] -= rmat[0][2]*hdr2->pixdim[3]*(hdr2->dim[3] - 1);
      mmat[7] -= rmat[1][2]*hdr2->pixdim[3]*(hdr2->dim[3] - 1);
      mmat[11] -= rmat[2][2]*hdr2->pixdim[3]*(hdr2->dim[3] - 1);
      }

    this->QFormMatrix = vtkMatrix4x4::New();
    this->QFormMatrix->DeepCopy(mmat);
    }

  // Set the SFormMatrix from the matrix information in the header.
  // See the long discussion above for more information.
  if (niftiVersion > 0 && hdr2->sform_code > 0)
    {
    double mmat[16];

    // first row
    mmat[0] = hdr2->srow_x[0]/hdr2->pixdim[1];
    mmat[1] = hdr2->srow_x[1]/hdr2->pixdim[2];
    mmat[2] = hdr2->srow_x[2]/hdr2->pixdim[3];
    mmat[3] = hdr2->srow_x[3];

    // second row
    mmat[4] = hdr2->srow_y[0]/hdr2->pixdim[1];
    mmat[5] = hdr2->srow_y[1]/hdr2->pixdim[2];
    mmat[6] = hdr2->srow_y[2]/hdr2->pixdim[3];
    mmat[7] = hdr2->srow_y[3];

    // third row
    mmat[8] = hdr2->srow_z[0]/hdr2->pixdim[1];
    mmat[9] = hdr2->srow_z[1]/hdr2->pixdim[2];
    mmat[10] = hdr2->srow_z[2]/hdr2->pixdim[3];
    mmat[11] = hdr2->srow_z[3];

    mmat[12] = 0.0;
    mmat[13] = 0.0;
    mmat[14] = 0.0;
    mmat[15] = 1.0;

    // Set QFac to -1 if the determinant is negative, unless QFac
    // has already been set by the qform information.
    if (vtkMatrix4x4::Determinant(mmat) < 0 && hdr2->qform_code == 0)
      {
      this->QFac = -1.0;
      }

    if (this->QFac < 0)
      {
      // If QFac is set to -1 then the slices will be reversed, and we must
      // reverse the slice orientation vector (the third column of the matrix)
      // to compensate.

      // reverse the slice orientation vector
      mmat[2] = -mmat[2];
      mmat[6] = -mmat[6];
      mmat[10] = -mmat[10];

      // adjust the offset to compensate for changed slice ordering
      mmat[3] -= hdr2->srow_x[2]*(hdr2->dim[3] - 1);
      mmat[7] -= hdr2->srow_y[2]*(hdr2->dim[3] - 1);
      mmat[11] -= hdr2->srow_z[2]*(hdr2->dim[3] - 1);
      }

    this->SFormMatrix = vtkMatrix4x4::New();
    this->SFormMatrix->DeepCopy(mmat);

    if (this->SFormMatrix->Determinant() < 0)
      {
      vtkWarningMacro("SFormMatrix is flipped compared to QFormMatrix");
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkNIFTIImageReader::RequestData(
  vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // check whether the reader is in an error state
  if (this->GetErrorCode() != vtkErrorCode::NoError)
    {
    return 0;
    }

  // which output port did the request come from
  int outputPort =
    request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

  // for now, this reader has only one output
  if (outputPort > 0)
    {
    return 1;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int extent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);

  // get the data object, allocate memory
  vtkImageData *data =
    static_cast<vtkImageData *>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
#if VTK_MAJOR_VERSION >= 6
  this->AllocateOutputData(data, outInfo, extent);
#else
  this->AllocateOutputData(data, extent);
#endif

  data->GetPointData()->GetScalars()->SetName("NIFTI");

  const char *filename = 0;
  char *imgname = 0;

  if (this->FileNames)
    {
    vtkIdType n = this->FileNames->GetNumberOfValues();
    int headers = 0;
    for (vtkIdType i = 0; i < n; i++)
      {
      filename = this->FileNames->GetValue(i);
      // this checks for .hdr and .hdr.gz, case insensitive
      if (vtkNIFTIImageReader::CheckExtension(filename, ".hdr"))
        {
        headers++;
        }
      else
        {
        imgname = new char[strlen(filename) + 1];
        strcpy(imgname, filename);
        }
      }
    if (n != 2 || headers != 1)
      {
      vtkErrorMacro("There must be two files and one must be a .hdr file.");
      delete [] imgname;
      return 0;
      }
    }
  else
    {
    filename = this->GetFileName();
    }

  if (filename == 0)
    {
    vtkErrorMacro("A FileName must be provided");
    return 0;
    }

  if (imgname == 0)
    {
    imgname = vtkNIFTIImageReader::ReplaceExtension(filename, ".hdr", ".img");
    }

  if (imgname == 0)
    {
    vtkErrorMacro("Unable to locate image for file " << filename);
    return 0;
    }

  vtkDebugMacro("Opening NIFTI file " << imgname);

  data->GetPointData()->GetScalars()->SetName("NIFTI");

  unsigned char *dataPtr =
    static_cast<unsigned char *>(data->GetScalarPointer());

  gzFile file = gzopen(imgname, "rb");

  delete [] imgname;

  if (!file)
    {
    return 0;
    }

  int swapBytes = this->GetSwapBytes();
  int scalarSize = data->GetScalarSize();
  int numComponents = data->GetNumberOfScalarComponents();
  int timeDim = (this->Dim[0] >= 4 ? this->Dim[4] : 1);
  int vectorDim = (this->Dim[0] >= 5 ? this->Dim[5] : 1);
  if (this->TimeAsVector)
    {
    vectorDim *= timeDim;
    }

  int outSizeX = extent[1] - extent[0] + 1;
  int outSizeY = extent[3] - extent[2] + 1;
  int outSizeZ = extent[5] - extent[4] + 1;

  z_off_t fileVoxelIncr = scalarSize*numComponents/vectorDim;
  z_off_t fileRowIncr = fileVoxelIncr*this->Dim[1];
  z_off_t fileSliceIncr = fileRowIncr*this->Dim[2];
  z_off_t fileTimeIncr = fileSliceIncr*this->Dim[3];
  z_off_t fileVectorIncr = fileTimeIncr*this->Dim[4];
  if (this->TimeAsVector)
    {
    fileVectorIncr = fileTimeIncr;
    }

  // add a buffer for planar-vector to packed-vector conversion
  unsigned char *rowBuffer = 0;
  if (vectorDim > 1)
    {
    rowBuffer = new unsigned char[outSizeX*fileVoxelIncr];
    }

  // special increment to reverse the slices if needed
  vtkIdType sliceOffset = 0;

  if (this->GetQFac() < 0)
    {
    // put slices in reverse order
    sliceOffset = scalarSize*numComponents;
    sliceOffset *= outSizeX;
    sliceOffset *= outSizeY;
    dataPtr += sliceOffset*(outSizeZ - 1);
    }

  // report progress every 2% of the way to completion
  this->InvokeEvent(vtkCommand::StartEvent);
  this->UpdateProgress(0.0);
  vtkIdType target =
    static_cast<vtkIdType>(0.02*outSizeY*outSizeZ*vectorDim) + 1;
  vtkIdType count = 0;

  // seek to the start of the data
  z_off_t offset = static_cast<z_off_t>(this->GetHeaderSize());
  offset += extent[0]*fileVoxelIncr;
  offset += extent[2]*fileRowIncr;
  offset += extent[4]*fileSliceIncr;

  // read the data one row at a time, do planar-to-packed conversion
  // of vector components if NIFTI file has a vector dimension
  int rowSize = numComponents/vectorDim*outSizeX;
  int t = 0; // counter for time
  int c = 0; // counter for vector components
  int j = 0; // counter for rows
  int k = 0; // counter for slices
  unsigned char *ptr = dataPtr;

  int errorCode = 0;

  while (!this->AbortExecute)
    {
    if (offset)
      {
      int rval = gzseek(file, offset, SEEK_CUR);
      if (rval == -1)
        {
        errorCode = vtkErrorCode::FileFormatError;
        if (gzeof(file))
          {
          errorCode = vtkErrorCode::PrematureEndOfFileError;
          }
        break;
        }
      }

    if (vectorDim == 1)
      {
      // read directly into the output instead of into a buffer
      rowBuffer = ptr;
      }

    int code = gzread(file, rowBuffer, rowSize*scalarSize);
    if (code != rowSize*scalarSize)
      {
      errorCode = vtkErrorCode::FileFormatError;
      if (gzeof(file))
        {
        errorCode = vtkErrorCode::PrematureEndOfFileError;
        }
      break;
      }

    if (swapBytes != 0 && scalarSize > 1)
      {
      vtkByteSwap::SwapVoidRange(rowBuffer, rowSize, scalarSize);
      }

    if (vectorDim == 1)
      {
      // advance the pointer to the next row
      ptr += outSizeX*numComponents*scalarSize;
      rowBuffer = 0;
      }
    else
      {
      // write vector plane to packed vector component
      unsigned char *tmpPtr = rowBuffer;
      z_off_t skipOther = scalarSize*numComponents - fileVoxelIncr;
      for (int i = 0; i < outSizeX; i++)
        {
        // write one vector component of one voxel
        z_off_t n = fileVoxelIncr;
        do { *ptr++ = *tmpPtr++; } while (--n);
        // skip past the other components
        ptr += skipOther;
        }
      }

    if (++count % target == 0)
      {
      this->UpdateProgress(0.02*count/target);
      }

    // offset to skip unread sections of the file, for when
    // the update extent is less than the whole extent
    offset = fileRowIncr - outSizeX*fileVoxelIncr;
    if (++j == outSizeY)
      {
      j = 0;
      offset += fileSliceIncr - outSizeY*fileRowIncr;
      ptr -= 2*sliceOffset; // for reverse slice order
      if (++k == outSizeZ)
        {
        k = 0;
        offset += fileVectorIncr - outSizeZ*fileSliceIncr;
        if (++t == timeDim)
          {
          t = 0;
          }
        if (++c == vectorDim)
          {
          break;
          }
        // back up the ptr to the beginning of the image,
        // then increment to the next vector component
        ptr = dataPtr + c*fileVoxelIncr;

        if (this->TimeAsVector)
          {
          // if timeDim is included in the vectorDim (and hence in the
          // VTK scalar components) then we have to make sure that
          // the vector components are packed before the time steps
          ptr = dataPtr + (c + t*(vectorDim - 1))/timeDim*fileVoxelIncr;
          }
        }
      }
    }

  if (vectorDim > 1)
    {
    delete [] rowBuffer;
    }

  gzclose(file);

  if (errorCode)
    {
    const char *errorText = "Error in NIFTI file, cannot read.";
    if (errorCode == vtkErrorCode::PrematureEndOfFileError)
      {
      errorText = "NIFTI file is truncated, some data is missing.";
      }
    this->SetErrorCode(errorCode);
    vtkErrorMacro(<< errorText);
    return 0;
    }

  this->UpdateProgress(1.0);
  this->InvokeEvent(vtkCommand::EndEvent);

  return 1;
}
