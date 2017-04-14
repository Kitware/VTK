/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNIFTIImageWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNIFTIImageWriter.h"
#include "vtkObjectFactory.h"
#include "vtkNIFTIImageReader.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkErrorCode.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkCommand.h"
#include "vtkVersion.h"

#include "vtksys/SystemTools.hxx"
#include <sstream>

// Header for NIFTI
#include "vtkNIFTIImageHeader.h"
#include "vtkNIFTIImagePrivate.h"

// Header for zlib
#include "vtk_zlib.h"

#include <cstdio>
#include <cstring>
#include <cfloat>
#include <cmath>

vtkStandardNewMacro(vtkNIFTIImageWriter);
vtkCxxSetObjectMacro(vtkNIFTIImageWriter,QFormMatrix,vtkMatrix4x4);
vtkCxxSetObjectMacro(vtkNIFTIImageWriter,SFormMatrix,vtkMatrix4x4);
vtkCxxSetObjectMacro(vtkNIFTIImageWriter,NIFTIHeader,vtkNIFTIImageHeader);

//----------------------------------------------------------------------------
vtkNIFTIImageWriter::vtkNIFTIImageWriter()
{
  this->FileLowerLeft = 1;
  this->FileDimensionality = 3;
  this->TimeDimension = 0;
  this->TimeSpacing = 1.0;
  // If slope,inter are 0,0 then default slope,inter of 1,0 is used
  this->RescaleSlope = 0.0;
  this->RescaleIntercept = 0.0;
  this->QFac = 0.0;
  this->QFormMatrix = 0;
  this->SFormMatrix = 0;
  this->OwnHeader = 0;
  this->NIFTIHeader = 0;
  this->NIFTIVersion = 0;
  // Default description is "VTKX.Y.Z"
  const char *version = vtkVersion::GetVTKVersion();
  size_t l = strlen(version);
  this->Description = new char[l + 4];
  strncpy(this->Description, "VTK", 3);
  strncpy(&this->Description[3], version, l);
  this->Description[l + 3] = '\0';
  // Planar RGB (NIFTI doesn't allow this, it's here for Analyze)
  this->PlanarRGB = false;
}

//----------------------------------------------------------------------------
vtkNIFTIImageWriter::~vtkNIFTIImageWriter()
{
  if (this->QFormMatrix)
  {
    this->QFormMatrix->Delete();
  }
  if (this->SFormMatrix)
  {
    this->SFormMatrix->Delete();
  }
  if (this->OwnHeader)
  {
    this->OwnHeader->Delete();
  }
  if (this->NIFTIHeader)
  {
    this->NIFTIHeader->Delete();
  }
  delete [] this->Description;
}

//----------------------------------------------------------------------------
vtkNIFTIImageHeader *vtkNIFTIImageWriter::GetNIFTIHeader()
{
  if (!this->NIFTIHeader)
  {
    this->NIFTIHeader = vtkNIFTIImageHeader::New();
  }
  return this->NIFTIHeader;
}

//----------------------------------------------------------------------------
void vtkNIFTIImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Description: " << this->Description << "\n";
  os << indent << "TimeDimension: " << this->TimeDimension << "\n";
  os << indent << "TimeSpacing: " << this->TimeSpacing << "\n";
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

  os << indent << "NIFTIHeader: ";
  if (this->NIFTIHeader)
  {
    os << this->NIFTIHeader << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "NIFTIVersion: " << this->NIFTIVersion << "\n";
  os << indent << "PlanarRGB: " << (this->PlanarRGB ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
char *vtkNIFTIImageWriter::ReplaceExtension(
  const char *filename, const char *ext1, const char *ext2)
{
  size_t n = strlen(filename);
  size_t m = n;
  char *newname = new char[n+4];
  strcpy(newname, filename);

  if (n > 2 && filename[n-3] == '.' &&
      tolower(filename[n-2]) == 'g' &&
      tolower(filename[n-1]) == 'z')
  {
    m -= 3;
  }
  if (m > 3 && filename[m-4] == '.' &&
      tolower(filename[m-3]) == tolower(ext1[1]) &&
      tolower(filename[m-2]) == tolower(ext1[2]) &&
      tolower(filename[m-1]) == tolower(ext1[3]))
  {
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

  return newname;
}

//----------------------------------------------------------------------------
namespace {

// Initialize the NIFTI header with only the most basic information:
// - NIFTI data type is set from VTK data type
// - NIFTI pixdim set from VTK spacing
// - dimensionality is:
//  - 5 if number of components is greater than one
//  - 2 if Z dimension is one and number of components is one
//  - 3 if Z dimension is greater than one and number of components is one
// - units are NIFTI_UNITS_UNKNOWN
// - intent is NIFTI_INTENT_NONE
// - magic is "n+1" (i.e. a .nii file, header+image in one file)
// - vox_offset is set to the header size plus 64-bit alignment padding
// - everything else is initialized to zero
// After initialization, the following should be set:
// - if file is ".hdr", set magic to "ni1" and vox_offset to zero
// - intent should be set, if known
// - units should be set, if known
// - qform and sform should be set, if known
// - pixdim[0] should be set to qfac (1 or -1) if qform is known
// - slope and inter should be set, if known
// - descrip and intent_name should be set, if known
// - for RGB and RGBA images, header should be modified as necessary
// - for complex images, header should be modified as necessary

void vtkNIFTIImageWriterSetInformation(
  nifti_2_header *hdr,
  vtkInformation *info)
{
  // get the scalar information
  vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(
    info, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS);

  int extent[6];
  info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  double spacing[3];
  info->Get(vtkDataObject::SPACING(), spacing);

  int scalarType = scalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
  int numComponents = scalarInfo->Get(
    vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());

  // map VTK type to NIFTI type and bits
  static const int typeMap[][3] = {
#if VTK_TYPE_CHAR_IS_SIGNED
    { VTK_CHAR, NIFTI_TYPE_INT8, 8 },
#else
    { VTK_CHAR, NIFTI_TYPE_UINT8, 8 },
#endif
    { VTK_SIGNED_CHAR, NIFTI_TYPE_INT8, 8 },
    { VTK_UNSIGNED_CHAR, NIFTI_TYPE_UINT8, 8 },
    { VTK_SHORT, NIFTI_TYPE_INT16, 16 },
    { VTK_UNSIGNED_SHORT, NIFTI_TYPE_UINT16, 16 },
    { VTK_INT, NIFTI_TYPE_INT32, 32 },
    { VTK_UNSIGNED_INT, NIFTI_TYPE_UINT32, 32 },
#if VTK_SIZEOF_LONG == 4
    { VTK_LONG, NIFTI_TYPE_INT32, 32 },
    { VTK_UNSIGNED_LONG, NIFTI_TYPE_UINT32, 32 },
#else
    { VTK_LONG, NIFTI_TYPE_INT64, 64 },
    { VTK_UNSIGNED_LONG, NIFTI_TYPE_UINT64, 64 },
#endif
    { VTK_LONG_LONG, NIFTI_TYPE_INT64, 64 },
    { VTK_UNSIGNED_LONG_LONG, NIFTI_TYPE_UINT64, 64 },
#if !defined(VTK_LEGACY_REMOVE)
    { VTK___INT64, NIFTI_TYPE_INT64, 64 },
    { VTK_UNSIGNED___INT64, NIFTI_TYPE_UINT64, 64 },
#endif
    { VTK_FLOAT, NIFTI_TYPE_FLOAT32, 32 },
    { VTK_DOUBLE, NIFTI_TYPE_FLOAT64, 64 },
    { 0, 0, 0 }
  };

  short datatype = 0;
  short databits = 0;

  // the end of the typemap has been reached when typeMap[2] is 0
  for (int i = 0; typeMap[2] != 0; i++)
  {
    if (scalarType == typeMap[i][0])
    {
      datatype = typeMap[i][1];
      databits = typeMap[i][2];
      break;
    }
  }

  // number of spatial dimensions
  int spaceDim = (extent[4] == extent[5] ? 2 : 3);

  hdr->dim[0] = (numComponents == 1 ? spaceDim : 5);
  hdr->dim[1] = extent[1] - extent[0] + 1;
  hdr->dim[2] = extent[3] - extent[2] + 1;
  hdr->dim[3] = extent[5] - extent[4] + 1;
  hdr->dim[4] = 1;
  hdr->dim[5] = numComponents;
  hdr->dim[6] = 1;
  hdr->dim[7] = 1;

  hdr->datatype = datatype;
  hdr->bitpix = databits;

  hdr->slice_start = 0;
  hdr->pixdim[0] = 0.0;
  hdr->pixdim[1] = spacing[0];
  hdr->pixdim[2] = spacing[1];
  hdr->pixdim[3] = spacing[2];
  hdr->pixdim[4] = 1.0;
  hdr->pixdim[5] = 1.0;
  hdr->pixdim[6] = 1.0;
  hdr->pixdim[7] = 1.0;
}

// Set the QForm from a 4x4 matrix
void vtkNIFTIImageWriterSetQForm(
  nifti_2_header *hdr, double mmat[16], double qfac)
{
  double rmat[3][3];
  rmat[0][0] = mmat[0];
  rmat[0][1] = mmat[1];
  rmat[0][2] = mmat[2];
  rmat[1][0] = mmat[4];
  rmat[1][1] = mmat[5];
  rmat[1][2] = mmat[6];
  rmat[2][0] = mmat[8];
  rmat[2][1] = mmat[9];
  rmat[2][2] = mmat[10];

  double quat[4];
  vtkMath::Matrix3x3ToQuaternion(rmat, quat);
  if (quat[0] < 0)
  {
    quat[0] = -quat[0];
    quat[1] = -quat[1];
    quat[2] = -quat[2];
    quat[3] = -quat[3];
  }

  if (qfac < 0)
  {
    // We will be reversing the order of the slices, so the first VTK
    // slice will be at the position of the last NIfTI slice, and we
    // must adjust the offset to compensate for this.
    mmat[3] += rmat[0][2] * hdr->pixdim[3] * (hdr->dim[3] - 1);
    mmat[7] += rmat[1][2] * hdr->pixdim[3] * (hdr->dim[3] - 1);
    mmat[11] += rmat[2][2] * hdr->pixdim[3] * (hdr->dim[3] - 1);
  }

  hdr->pixdim[0] = qfac;
  hdr->quatern_b = quat[1];
  hdr->quatern_c = quat[2];
  hdr->quatern_d = quat[3];
  hdr->qoffset_x = mmat[3];
  hdr->qoffset_y = mmat[7];
  hdr->qoffset_z = mmat[11];
}

// Set the SForm from a 4x4 matrix
void vtkNIFTIImageWriterSetSForm(
  nifti_2_header *hdr, double mmat[16], double qfac)
{
  if (qfac < 0)
  {
    // If QFac is set to -1 (which only occurs if qform_code was set)
    // then the slices will be reversed, and we must reverse the slice
    // orientation vector (the third column of the matrix) to compensate.

    // adjust the offset to compensate for changed slice ordering
    mmat[3] += mmat[2] * hdr->pixdim[3] * (hdr->dim[3] - 1);
    mmat[7] += mmat[6] * hdr->pixdim[3] * (hdr->dim[3] - 1);
    mmat[11] += mmat[10] * hdr->pixdim[3] * (hdr->dim[3] - 1);

    // reverse the slice orientation vector
    mmat[2] = -mmat[2];
    mmat[6] = -mmat[6];
    mmat[10] = -mmat[10];
  }

  // first row
  hdr->srow_x[0] = mmat[0] * hdr->pixdim[1];
  hdr->srow_x[1] = mmat[1] * hdr->pixdim[2];
  hdr->srow_x[2] = mmat[2] * hdr->pixdim[3];
  hdr->srow_x[3] = mmat[3];

  // second row
  hdr->srow_y[0] = mmat[4] * hdr->pixdim[1];
  hdr->srow_y[1] = mmat[5] * hdr->pixdim[2];
  hdr->srow_y[2] = mmat[6] * hdr->pixdim[3];
  hdr->srow_y[3] = mmat[7];

  // third row
  hdr->srow_z[0] = mmat[8] * hdr->pixdim[1];
  hdr->srow_z[1] = mmat[9] * hdr->pixdim[2];
  hdr->srow_z[2] = mmat[10] * hdr->pixdim[3];
  hdr->srow_z[3] = mmat[11];
}

void vtkNIFTIImageWriterMatrix(
  double mmat[16], vtkMatrix4x4 *matrix, const double origin[3])
{
  // find new offset by multiplying the origin by the matrix
  double offset[4];
  offset[0] = origin[0];
  offset[1] = origin[1];
  offset[2] = origin[2];
  offset[3] = 1.0;

  if (matrix)
  {
    matrix->MultiplyPoint(offset, offset);
    vtkMatrix4x4::DeepCopy(mmat, matrix);
  }
  else
  {
    vtkMatrix4x4::Identity(mmat);
  }

  mmat[3] = offset[0];
  mmat[7] = offset[1];
  mmat[11] = offset[2];
}

} // end anonymous namespace

//----------------------------------------------------------------------------
int vtkNIFTIImageWriter::GenerateHeader(vtkInformation *info, bool singleFile)
{
  // create the header
  nifti_2_header hdr;
  int version = 0;
  if (this->OwnHeader == 0)
  {
    this->OwnHeader = vtkNIFTIImageHeader::New();
  }
  else
  {
    this->OwnHeader->Initialize();
  }
  if (this->NIFTIHeader)
  {
    // use the header supplied by SetNIFTIHeader()
    this->NIFTIHeader->GetHeader(&hdr);
    version = hdr.magic[2] - '0';
    if (version > 2)
    {
      version = 2;
    }
  }
  else
  {
    // start with a blank header
    this->OwnHeader->GetHeader(&hdr);
    hdr.scl_slope = 1.0;
  }

  // copy the image information into the header
  vtkNIFTIImageWriterSetInformation(&hdr, info);
  if (hdr.datatype == 0)
  {
    vtkErrorMacro("Illegal data type for NIFTI file.");
    return 0;
  }

  // override the version if set via SetNIFTIVersion
  if (this->NIFTIVersion != 0)
  {
    version = this->NIFTIVersion;
  }

  // set the rescale slope/intercept if not (0.0,0.0)
  if (this->RescaleSlope != 0.0 || this->RescaleIntercept != 0.0)
  {
    hdr.scl_slope = this->RescaleSlope;
    hdr.scl_inter = this->RescaleIntercept;
  }

  // set the header size
  hdr.sizeof_hdr = (version == 2 ?
                    vtkNIFTIImageHeader::NIFTI2HeaderSize :
                    vtkNIFTIImageHeader::NIFTI1HeaderSize);

  // modify magic number and voxel offset for .img files
  if (!singleFile)
  {
    strncpy(hdr.magic, (version == 2 ? "ni2" : "ni1"), 4);
    hdr.vox_offset = 0;
  }
  else
  {
    strncpy(hdr.magic, (version == 2 ? "n+2" : "n+1"), 4);
    hdr.vox_offset = (version == 2 ? 544 : 352);
  }
  if (version == 2)
  {
    // version 2 has four bytes for newline transfer checks
    strncpy(&hdr.magic[4], "\r\n\032\n", 4);
  }

  // set the description
  if (this->Description)
  {
    strncpy(hdr.descrip, this->Description, sizeof(hdr.descrip) - 1);
    hdr.descrip[sizeof(hdr.descrip) - 1] = '\0';
  }

  // qfac dictates the slice ordering in the file
  double qfac = (this->QFac < 0 ? -1.0 : 1.0);

  // origin must be incorporated into qform and sform
  double origin[3];
  info->Get(vtkDataObject::ORIGIN(), origin);

  if (this->QFormMatrix ||
      (origin[0] != 0 || origin[1] != 0 || origin[2] != 0))
  {
    hdr.qform_code = 1; // SCANNER_ANAT
    double mat16[16];
    vtkNIFTIImageWriterMatrix(mat16, this->QFormMatrix, origin);
    vtkNIFTIImageWriterSetQForm(&hdr, mat16, qfac);
  }

  if (this->SFormMatrix)
  {
    hdr.sform_code = 2; // ALIGNED_ANAT
    double mat16[16];
    vtkNIFTIImageWriterMatrix(mat16, this->SFormMatrix, origin);
    vtkNIFTIImageWriterSetSForm(&hdr, mat16, qfac);
  }

  // base dimension not counting vector dimension
  int basedim = (hdr.dim[3] == 1 ? 2 : 3);

  if (this->TimeDimension)
  {
    int tdim = this->TimeDimension;
    if (hdr.dim[5] % tdim != 0)
    {
      vtkErrorMacro("Number of components in the image data must be "
                    "divisible by the TimeDimension");
      return 0;
    }
    hdr.pixdim[4] = this->TimeSpacing;
    hdr.dim[4] = tdim;
    hdr.dim[5] /= tdim;
    hdr.dim[0] = (hdr.dim[5] > 1 ? 5 : 4);
    basedim = 4;
  }

  if (hdr.dim[5] == 2 && hdr.datatype == NIFTI_TYPE_FLOAT32)
  {
    // float with 2 components becomes COMPLEX64
    hdr.datatype = NIFTI_TYPE_COMPLEX64;
    hdr.bitpix = 64;
    hdr.dim[0] = basedim;
    hdr.dim[5] = 1;
  }
  else if (hdr.dim[5] == 2 && hdr.datatype == NIFTI_TYPE_FLOAT64)
  {
    // double with 2 components becomes COMPLEX128
    hdr.datatype = NIFTI_TYPE_COMPLEX128;
    hdr.bitpix = 32;
    hdr.dim[0] = basedim;
    hdr.dim[5] = 1;
  }
  else if (hdr.dim[5] == 3 && hdr.datatype == NIFTI_TYPE_UINT8)
  {
    // unsigned char with 3 components becomes RGB24
    hdr.datatype = NIFTI_TYPE_RGB24;
    hdr.bitpix = 24;
    hdr.dim[0] = basedim;
    hdr.dim[5] = 1;
  }
  else if (hdr.dim[5] == 4 && hdr.datatype == NIFTI_TYPE_UINT8)
  {
    // unsigned char with 4 components becomes RGBA32
    hdr.datatype = NIFTI_TYPE_RGBA32;
    hdr.bitpix = 32;
    hdr.dim[0] = basedim;
    hdr.dim[5] = 1;
  }

  this->OwnHeader->SetHeader(&hdr);
  return 1;
}

//----------------------------------------------------------------------------
int vtkNIFTIImageWriter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  this->SetErrorCode(vtkErrorCode::NoError);

  vtkInformation *info = inputVector[0]->GetInformationObject(0);
  vtkImageData *data =
    vtkImageData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  if (data == NULL)
  {
    vtkErrorMacro("No input provided!");
    return 0;
  }

  const char *filename = this->GetFileName();
  if (filename == NULL)
  {
    vtkErrorMacro("A FileName must be provided");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  int extent[6];
  info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  // use compression if name ends in .gz
  bool isCompressed = false;
  size_t n = strlen(filename);
  size_t m = n;
  if (n > 2 && filename[n-3] == '.' &&
      tolower(filename[n-2]) == 'g' &&
      tolower(filename[n-1]) == 'z')
  {
    m = n - 3;
    isCompressed = true;
  }

  // after the optional ".gz" is removed, is it a ".img/.hdr" file?
  bool singleFile = true;
  if (m > 4 && filename[m-4] == '.' &&
      ((tolower(filename[m-3]) == 'h' &&
        tolower(filename[m-2]) == 'd' &&
        tolower(filename[m-1]) == 'r') ||
       (tolower(filename[m-3]) == 'i' &&
        tolower(filename[m-2]) == 'm' &&
        tolower(filename[m-1]) == 'g')))
  {
    singleFile = false;
  }

  // generate the header information
  if (this->GenerateHeader(info, singleFile) == 0)
  {
    return 0;
  }

  // if file is not .nii, then get .hdr and .img filenames
  char *hdrname = vtkNIFTIImageWriter::ReplaceExtension(
    filename, ".img", ".hdr");
  char *imgname = vtkNIFTIImageWriter::ReplaceExtension(
    filename, ".hdr", ".img");

  vtkDebugMacro(<< "Writing NIFTI file " << hdrname);

  // get either a NIFTIv1 or a NIFTIv2 header
  nifti_1_header hdr1;
  nifti_2_header hdr2;
  void *hdrptr = 0;
  size_t hdrsize = 0;
  int version = this->OwnHeader->GetMagic()[2] - '0';
  if (version == 2)
  {
    this->OwnHeader->GetHeader(&hdr2);
    hdrptr = &hdr2;
    hdrsize = hdr2.sizeof_hdr;
  }
  else
  {
    this->OwnHeader->GetHeader(&hdr1);
    hdrptr = &hdr1;
    hdrsize = hdr1.sizeof_hdr;
    if (extent[1] - extent[0] + 1 > VTK_SHORT_MAX ||
        extent[3] - extent[2] + 1 > VTK_SHORT_MAX ||
        extent[5] - extent[4] + 1 > VTK_SHORT_MAX)
    {
      vtkErrorMacro("Image too large to store in NIFTI-1 format");
      delete [] hdrname;
      delete [] imgname;
      return 0;
    }
  }

  // try opening file
  gzFile file = 0;
  FILE *ufile = 0;
  if (isCompressed)
  {
    file = gzopen(hdrname, "wb");
  }
  else
  {
    ufile = fopen(hdrname, "wb");
  }

  if (!file && !ufile)
  {
    vtkErrorMacro("Cannot open file " << hdrname);
    delete [] hdrname;
    delete [] imgname;
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return 0;
  }

  this->InvokeEvent(vtkCommand::StartEvent);
  this->UpdateProgress(0.0);

  // write the header
  size_t bytesWritten = 0;
  if (isCompressed)
  {
    unsigned int hsize = static_cast<unsigned int>(hdrsize);
    int code = gzwrite(file, hdrptr, hsize);
    bytesWritten = (code < 0 ? 0 : code);
  }
  else
  {
    bytesWritten = fwrite(hdrptr, 1, hdrsize, ufile);
  }
  if (bytesWritten < hdrsize)
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
  }

  if (singleFile && !this->ErrorCode)
  {
    // write the padding between the header and the image to the .nii file
    size_t padsize = (static_cast<size_t>(this->OwnHeader->GetVoxOffset()) -
                      hdrsize);
    char *padding = new char[padsize];
    memset(padding, '\0', padsize);
    if (isCompressed)
    {
      int code = gzwrite(file, padding, static_cast<unsigned int>(padsize));
      bytesWritten = (code < 0 ? 0 : code);
    }
    else
    {
      bytesWritten = fwrite(padding, 1, padsize, ufile);
    }
    delete [] padding;
    if (bytesWritten < padsize)
    {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    }
  }
  else if (!this->ErrorCode)
  {
    // close the .hdr file and open the .img file
    if (isCompressed)
    {
      gzclose(file);
      file = gzopen(imgname, "wb");
    }
    else
    {
      fclose(ufile);
      ufile = fopen(imgname, "wb");
    }
  }

  if (!file && !ufile)
  {
    vtkErrorMacro("Cannot open file " << imgname);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return 0;
  }

  // write the image
  unsigned char *dataPtr =
    static_cast<unsigned char *>(data->GetScalarPointer());

  // check if planar RGB is applicable (Analyze only)
  bool planarRGB = (this->PlanarRGB &&
                    (this->OwnHeader->GetDataType() == NIFTI_TYPE_RGB24 ||
                     this->OwnHeader->GetDataType() == NIFTI_TYPE_RGBA32));

  int scalarSize = data->GetScalarSize();
  int numComponents = data->GetNumberOfScalarComponents();
  int outSizeX = static_cast<int>(this->OwnHeader->GetDim(1));
  int outSizeY = static_cast<int>(this->OwnHeader->GetDim(2));
  int outSizeZ = static_cast<int>(this->OwnHeader->GetDim(3));
  int timeDim = static_cast<int>(this->OwnHeader->GetDim(4));
  int vectorDim = static_cast<int>(this->OwnHeader->GetDim(5));

  // for counting, include timeDim in vectorDim
  vectorDim *= timeDim;

  z_off_t fileVoxelIncr = scalarSize*numComponents/vectorDim;
  int planarSize = 1;
  if (planarRGB)
  {
    planarSize = numComponents/vectorDim;
    fileVoxelIncr = scalarSize;
  }

  // add a buffer for planar-vector to packed-vector conversion
  unsigned char *rowBuffer = 0;
  if (vectorDim > 1 || planarRGB)
  {
    rowBuffer = new unsigned char[outSizeX*fileVoxelIncr];
  }

  // special increment to reverse the slices if needed
  vtkIdType sliceOffset = 0;

  if (this->QFac < 0)
  {
    // put slices in reverse order
    sliceOffset = scalarSize*numComponents;
    sliceOffset *= outSizeX;
    sliceOffset *= outSizeY;
    dataPtr += sliceOffset*(outSizeZ - 1);
  }

  // special increment to handle planar RGB
  vtkIdType planarOffset = 0;
  vtkIdType planarEndOffset = 0;
  if (planarRGB)
  {
    planarOffset = scalarSize*numComponents;
    planarOffset *= outSizeX;
    planarOffset *= outSizeY;
    planarOffset -= scalarSize;
    planarEndOffset = planarOffset - scalarSize*(planarSize - 1);
  }

  // report progress every 2% of the way to completion
  vtkIdType target =
    static_cast<vtkIdType>(0.02*planarSize*outSizeY*outSizeZ*vectorDim) + 1;
  vtkIdType count = 0;

  // write the data one row at a time, do planar-to-packed conversion
  // of vector components if NIFTI file has a vector dimension
  int rowSize = fileVoxelIncr/scalarSize*outSizeX;
  int c = 0; // counter for vector components
  int j = 0; // counter for rows
  int p = 0; // counter for planes (planar RGB)
  int k = 0; // counter for slices
  int t = 0; // counter for time

  unsigned char *ptr = dataPtr;

  while (!this->AbortExecute && !this->ErrorCode)
  {
    if (vectorDim == 1 && !planarRGB)
    {
      // write directly from input, instead of using a buffer
      rowBuffer = ptr;
      ptr += outSizeX*numComponents*scalarSize;
    }
    else
    {
      // create a vector plane from packed vector components
      unsigned char *tmpPtr = rowBuffer;
      z_off_t skipOther = scalarSize*numComponents - fileVoxelIncr;
      for (int i = 0; i < outSizeX; i++)
      {
        // write one vector component of one voxel
        z_off_t nn = fileVoxelIncr;
        do { *tmpPtr++ = *ptr++; } while (--nn);
        // skip past the other components
        ptr += skipOther;
      }
    }

    if (isCompressed)
    {
      int code = gzwrite(file, rowBuffer, rowSize*scalarSize);
      bytesWritten = (code < 0 ? 0 : code);
    }
    else
    {
      bytesWritten = fwrite(rowBuffer, scalarSize, rowSize, ufile)*scalarSize;
    }
    if (bytesWritten < static_cast<size_t>(rowSize*scalarSize))
    {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      break;
    }

    if (++count % target == 0)
    {
      this->UpdateProgress(0.02*count/target);
    }

    if (++j == outSizeY)
    {
      j = 0;
      // back up for next plane (R, G, or B) if planar mode
      ptr -= planarOffset;
      if (++p == planarSize)
      {
        p = 0;
        ptr += planarEndOffset; // advance to start of next slice
        ptr -= 2*sliceOffset; // for reverse slice order
        if (++k == outSizeZ)
        {
          k = 0;
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
          ptr = dataPtr + c*fileVoxelIncr*planarSize;

          if (timeDim > 1)
          {
            // if timeDim is included in the vectorDim (and hence in the
            // VTK scalar components) then we have to make sure that
            // the vector components are packed before the time steps
            ptr = dataPtr + (c + t*(vectorDim - 1))/timeDim*
                             fileVoxelIncr*planarSize;
          }
        }
      }
    }
  }

  // only delete this if it was alloced (if it was not alloced, it
  // would have been set directly to a row out the output image)
  if (vectorDim > 1 || planarRGB)
  {
    delete [] rowBuffer;
  }

  if (isCompressed)
  {
    gzclose(file);
  }
  else
  {
    fclose(ufile);
  }

  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    // erase the file, rather than leave a corrupt file on disk
    vtkErrorMacro("Out of disk space, removing incomplete file " << imgname);
    vtksys::SystemTools::RemoveFile(imgname);
    if (!singleFile)
    {
      vtksys::SystemTools::RemoveFile(hdrname);
    }
  }

  this->UpdateProgress(1.0);
  this->InvokeEvent(vtkCommand::EndEvent);

  delete [] hdrname;
  delete [] imgname;

  return 1;
}
