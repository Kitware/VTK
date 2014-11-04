/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkNIFTIImageHeader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNIFTIImageHeader.h"
#include "vtkNIFTIImagePrivate.h"

#include "vtkObjectFactory.h"

#include <string.h>
#include <float.h>
#include <math.h>
#include <ctype.h>

vtkStandardNewMacro(vtkNIFTIImageHeader);

//----------------------------------------------------------------------------
namespace {

// utility function to normalize floats that are close to zero
double vtkNIFTINormalizeFloat(double d)
{
  return (fabs(d) < FLT_MIN ? 0.0 : d);
}

double vtkNIFTINormalizeDouble(double d)
{
  return (fabs(d) < DBL_MIN ? 0.0 : d);
}
} // end anonymous namespace

//----------------------------------------------------------------------------
vtkNIFTIImageHeader::vtkNIFTIImageHeader()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkNIFTIImageHeader::~vtkNIFTIImageHeader()
{
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::Initialize()
{
  memset(this->Magic, '\0', sizeof(this->Magic));
  this->VoxOffset = 0;
  this->DataType = 0;
  this->BitPix = 0;
  for (int i = 0; i < 8; i++)
    {
    this->Dim[i] = 0;
    this->PixDim[i] = 0.0;
    }
  this->IntentCode = 0;
  memset(this->IntentName, '\0', sizeof(this->IntentName));
  this->IntentP1 = 0.0;
  this->IntentP2 = 0.0;
  this->IntentP3 = 0.0;
  this->SclSlope = 0.0;
  this->SclInter = 0.0;
  this->CalMin = 0.0;
  this->CalMax = 0.0;
  this->SliceDuration = 0.0;
  this->TOffset = 0.0;
  this->SliceStart = 0;
  this->SliceEnd = 0;
  this->SliceCode = 0;
  this->XYZTUnits = 0;
  this->DimInfo = 0;
  memset(this->Descrip, '\0', sizeof(this->Descrip));
  memset(this->AuxFile, '\0', sizeof(this->AuxFile));
  this->QFormCode = 0;
  this->SFormCode = 0;
  this->QuaternB = 0.0;
  this->QuaternC = 0.0;
  this->QuaternD = 0.0;
  this->QOffsetX = 0.0;
  this->QOffsetY = 0.0;
  this->QOffsetZ = 0.0;
  for (int i = 0; i < 4; i++)
    {
    this->SRowX[i] = 0.0;
    this->SRowY[i] = 0.0;
    this->SRowZ[i] = 0.0;
    }
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::SetHeader(const nifti_1_header *hdr)
{
  // clear all fields (in case supplied header is Analyze 7.5)
  this->Initialize();

  // check if header is NIfTI (vs. Analyze 7.5)
  bool isnifti = (hdr->magic[0] == 'n' &&
                  (hdr->magic[1] == '+' || hdr->magic[1] == 'i') &&
                  hdr->magic[2] == '1' &&
                  hdr->magic[3] == '\0');

  if (isnifti)
    {
    memcpy(this->Magic, hdr->magic, sizeof(hdr->magic));
    }
  this->VoxOffset = static_cast<vtkTypeInt64>(hdr->vox_offset);
  this->DataType = hdr->datatype;
  this->BitPix = hdr->bitpix;
  for (int i = 0; i < 8; i++)
    {
    this->Dim[i] = hdr->dim[i];
    this->PixDim[i] = hdr->pixdim[i];
    }
  if (isnifti)
    {
    this->IntentCode = hdr->intent_code;
    strncpy(this->IntentName, hdr->intent_name, sizeof(hdr->intent_name));
    this->IntentP1 = hdr->intent_p1;
    this->IntentP2 = hdr->intent_p2;
    this->IntentP3 = hdr->intent_p3;
    this->SclSlope = hdr->scl_slope;
    this->SclInter = hdr->scl_inter;
    }
  this->CalMin = hdr->cal_min;
  this->CalMax = hdr->cal_max;
  if (isnifti)
    {
    this->SliceDuration = hdr->slice_duration;
    this->TOffset = hdr->toffset;
    this->SliceStart = hdr->slice_start;
    this->SliceEnd = hdr->slice_end;
    this->SliceCode = hdr->slice_code;
    }
  this->XYZTUnits = hdr->xyzt_units;
  this->DimInfo = hdr->dim_info;
  strncpy(this->Descrip, hdr->descrip, sizeof(hdr->descrip));
  strncpy(this->AuxFile, hdr->aux_file, sizeof(hdr->aux_file));
  if (isnifti)
    {
    this->QFormCode = hdr->qform_code;
    this->SFormCode = hdr->sform_code;
    this->QuaternB = hdr->quatern_b;
    this->QuaternC = hdr->quatern_c;
    this->QuaternD = hdr->quatern_d;
    this->QOffsetX = hdr->qoffset_x;
    this->QOffsetY = hdr->qoffset_y;
    this->QOffsetZ = hdr->qoffset_z;
    for (int i = 0; i < 4; i++)
      {
      this->SRowX[i] = hdr->srow_x[i];
      this->SRowY[i] = hdr->srow_y[i];
      this->SRowZ[i] = hdr->srow_z[i];
      }
    }
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::GetHeader(nifti_1_header *hdr)
{
  hdr->sizeof_hdr = NIFTI1HeaderSize;
  memcpy(hdr->magic, this->Magic, sizeof(hdr->magic));
  memset(hdr->data_type, '\0', 10);
  memset(hdr->db_name, '\0', 18);
  hdr->extents = 0;
  hdr->session_error = 0;
  hdr->regular = 0;
  hdr->dim_info = this->DimInfo;
  hdr->intent_p1 = vtkNIFTINormalizeFloat(this->IntentP1);
  hdr->intent_p2 = vtkNIFTINormalizeFloat(this->IntentP2);
  hdr->intent_p3 = vtkNIFTINormalizeFloat(this->IntentP3);
  hdr->intent_code = static_cast<short>(this->IntentCode);
  hdr->datatype = static_cast<short>(this->DataType);
  hdr->bitpix = static_cast<short>(this->BitPix);
  hdr->slice_start = this->SliceStart;
  for (int i = 0; i < 8; i++)
    {
    hdr->dim[i] = static_cast<short>(this->Dim[i]);
    hdr->pixdim[i] = vtkNIFTINormalizeFloat(this->PixDim[i]);
    }
  hdr->vox_offset = static_cast<float>(this->VoxOffset);
  strncpy(hdr->intent_name, this->IntentName, sizeof(hdr->intent_name));
  hdr->scl_slope = vtkNIFTINormalizeFloat(this->SclSlope);
  hdr->scl_inter = vtkNIFTINormalizeFloat(this->SclInter);
  hdr->cal_min = vtkNIFTINormalizeFloat(this->CalMin);
  hdr->cal_max = vtkNIFTINormalizeFloat(this->CalMax);
  hdr->slice_duration = vtkNIFTINormalizeFloat(this->SliceDuration);
  hdr->toffset = vtkNIFTINormalizeFloat(this->TOffset);
  hdr->glmax = 0;
  hdr->glmin = 0;
  hdr->slice_end = this->SliceEnd;
  hdr->slice_code = this->SliceCode;
  hdr->xyzt_units = this->XYZTUnits;
  strncpy(hdr->descrip, this->Descrip, sizeof(hdr->descrip));
  strncpy(hdr->aux_file, this->AuxFile, sizeof(hdr->aux_file));
  hdr->qform_code = static_cast<short>(this->QFormCode);
  hdr->sform_code = static_cast<short>(this->SFormCode);
  hdr->quatern_b = vtkNIFTINormalizeFloat(this->QuaternB);
  hdr->quatern_c = vtkNIFTINormalizeFloat(this->QuaternC);
  hdr->quatern_d = vtkNIFTINormalizeFloat(this->QuaternD);
  hdr->qoffset_x = vtkNIFTINormalizeFloat(this->QOffsetX);
  hdr->qoffset_y = vtkNIFTINormalizeFloat(this->QOffsetY);
  hdr->qoffset_z = vtkNIFTINormalizeFloat(this->QOffsetZ);
  for (int i = 0; i < 4; i++)
    {
    hdr->srow_x[i] = vtkNIFTINormalizeFloat(this->SRowX[i]);
    hdr->srow_y[i] = vtkNIFTINormalizeFloat(this->SRowY[i]);
    hdr->srow_z[i] = vtkNIFTINormalizeFloat(this->SRowZ[i]);
    }
}


//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::SetHeader(const nifti_2_header *hdr)
{
  memcpy(this->Magic, hdr->magic, sizeof(hdr->magic));
  this->VoxOffset = hdr->vox_offset;
  this->DataType = hdr->datatype;
  this->BitPix = hdr->bitpix;
  for (int i = 0; i < 8; i++)
    {
    this->Dim[i] = hdr->dim[i];
    this->PixDim[i] = hdr->pixdim[i];
    }
  this->IntentCode = hdr->intent_code;
  strncpy(this->IntentName, hdr->intent_name, sizeof(hdr->intent_name));
  this->IntentP1 = hdr->intent_p1;
  this->IntentP2 = hdr->intent_p2;
  this->IntentP3 = hdr->intent_p3;
  this->SclSlope = hdr->scl_slope;
  this->SclInter = hdr->scl_inter;
  this->CalMin = hdr->cal_min;
  this->CalMax = hdr->cal_max;
  this->SliceDuration = hdr->slice_duration;
  this->TOffset = hdr->toffset;
  this->SliceStart = hdr->slice_start;
  this->SliceEnd = hdr->slice_end;
  this->SliceCode = hdr->slice_code;
  this->XYZTUnits = hdr->xyzt_units;
  this->DimInfo = hdr->dim_info;
  strncpy(this->Descrip, hdr->descrip, sizeof(hdr->descrip));
  strncpy(this->AuxFile, hdr->aux_file, sizeof(hdr->aux_file));
  this->QFormCode = hdr->qform_code;
  this->SFormCode = hdr->sform_code;
  this->QuaternB = hdr->quatern_b;
  this->QuaternC = hdr->quatern_c;
  this->QuaternD = hdr->quatern_d;
  this->QOffsetX = hdr->qoffset_x;
  this->QOffsetY = hdr->qoffset_y;
  this->QOffsetZ = hdr->qoffset_z;
  for (int i = 0; i < 4; i++)
    {
    this->SRowX[i] = hdr->srow_x[i];
    this->SRowY[i] = hdr->srow_y[i];
    this->SRowZ[i] = hdr->srow_z[i];
    }
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::GetHeader(nifti_2_header *hdr)
{
  hdr->sizeof_hdr = NIFTI2HeaderSize;
  memcpy(hdr->magic, this->Magic, sizeof(hdr->magic));
  hdr->datatype = static_cast<short>(this->DataType);
  hdr->bitpix = static_cast<short>(this->BitPix);
  for (int i = 0; i < 8; i++)
    {
    hdr->dim[i] = static_cast<short>(this->Dim[i]);
    hdr->pixdim[i] = vtkNIFTINormalizeDouble(this->PixDim[i]);
    }
  hdr->intent_p1 = vtkNIFTINormalizeDouble(this->IntentP1);
  hdr->intent_p2 = vtkNIFTINormalizeDouble(this->IntentP2);
  hdr->intent_p3 = vtkNIFTINormalizeDouble(this->IntentP3);
  hdr->vox_offset = this->VoxOffset;
  hdr->scl_slope = vtkNIFTINormalizeDouble(this->SclSlope);
  hdr->scl_inter = vtkNIFTINormalizeDouble(this->SclInter);
  hdr->cal_min = vtkNIFTINormalizeDouble(this->CalMin);
  hdr->cal_max = vtkNIFTINormalizeDouble(this->CalMax);
  hdr->slice_duration = vtkNIFTINormalizeDouble(this->SliceDuration);
  hdr->toffset = vtkNIFTINormalizeDouble(this->TOffset);
  hdr->slice_start = this->SliceStart;
  hdr->slice_end = this->SliceEnd;
  strncpy(hdr->descrip, this->Descrip, sizeof(hdr->descrip));
  strncpy(hdr->aux_file, this->AuxFile, sizeof(hdr->aux_file));
  hdr->qform_code = static_cast<short>(this->QFormCode);
  hdr->sform_code = static_cast<short>(this->SFormCode);
  hdr->quatern_b = vtkNIFTINormalizeDouble(this->QuaternB);
  hdr->quatern_c = vtkNIFTINormalizeDouble(this->QuaternC);
  hdr->quatern_d = vtkNIFTINormalizeDouble(this->QuaternD);
  hdr->qoffset_x = vtkNIFTINormalizeDouble(this->QOffsetX);
  hdr->qoffset_y = vtkNIFTINormalizeDouble(this->QOffsetY);
  hdr->qoffset_z = vtkNIFTINormalizeDouble(this->QOffsetZ);
  for (int i = 0; i < 4; i++)
    {
    hdr->srow_x[i] = vtkNIFTINormalizeDouble(this->SRowX[i]);
    hdr->srow_y[i] = vtkNIFTINormalizeDouble(this->SRowY[i]);
    hdr->srow_z[i] = vtkNIFTINormalizeDouble(this->SRowZ[i]);
    }
  hdr->slice_code = this->SliceCode;
  hdr->xyzt_units = this->XYZTUnits;
  hdr->intent_code = static_cast<short>(this->IntentCode);
  strncpy(hdr->intent_name, this->IntentName, sizeof(hdr->intent_name));
  hdr->dim_info = static_cast<char>(this->DimInfo);
  memset(hdr->unused_str, '\0', 15);
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::DeepCopy(vtkNIFTIImageHeader *o)
{
  if (o)
    {
    nifti_2_header hdr;
    o->GetHeader(&hdr);
    this->SetHeader(&hdr);
    }
  else
    {
    this->Initialize();
    }
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os.setf(std::ios::hex, std::ios::basefield);
  os << indent << "DimInfo: 0x" << this->DimInfo << "\n";
  os.unsetf(std::ios::hex);
  os << indent << "Dim:";
  for (int i = 0; i < 8; i++)
    {
    os << " " << this->Dim[i];
    }
  os << indent << "\n";
  os << indent << "PixDim:";
  for (int i = 0; i < 8; i++)
    {
    os << " " << this->PixDim[i];
    }
  os << indent << "\n";
  os << indent << "VoxOffset:" << this->VoxOffset << "\n";
  os << indent << "IntentP1: " << this->IntentP1 << "\n";
  os << indent << "IntentP2: " << this->IntentP2 << "\n";
  os << indent << "IntentP3: " << this->IntentP3 << "\n";
  os << indent << "IntentCode: " << this->IntentCode << "\n";
  os << indent << "DataType: " << this->DataType << "\n";
  os << indent << "BitPix: " << this->BitPix << "\n";
  os << indent << "SliceStart: " << this->SliceStart << "\n";
  os << indent << "SclSlope: " << this->SclSlope << "\n";
  os << indent << "SclInter: " << this->SclInter << "\n";
  os << indent << "SliceEnd: " << this->SliceEnd << "\n";
  os << indent << "SliceCode: " << static_cast<int>(this->SliceCode) << "\n";
  os.setf(std::ios::hex, std::ios::basefield);
  os << indent << "XYZTUnits: 0x" << static_cast<int>(this->XYZTUnits) << "\n";
  os.unsetf(std::ios::hex);
  os << indent << "CalMax: " << this->CalMax << "\n";
  os << indent << "CalMin: " << this->CalMin << "\n";
  os << indent << "SliceDuration: " << this->SliceDuration << "\n";
  os << indent << "TOffset: " << this->TOffset << "\n";
  os << indent << "Descrip: \"";
  for (size_t j = 0; j < 80 && this->Descrip[j] != '\0'; j++)
    {
    os << (isprint(this->Descrip[j]) ? this->Descrip[j] : '?');
    }
  os << "\"\n";
  os << indent << "AuxFile: \"";
  for (size_t j = 0; j < 24 && this->AuxFile[j] != '\0'; j++)
    {
    os << (isprint(this->AuxFile[j]) ? this->AuxFile[j] : '?');
    }
  os << "\"\n";
  os << indent << "QFormCode: " << this->QFormCode << "\n";
  os << indent << "SFormCode: " << this->SFormCode << "\n";
  os << indent << "QuaternB: " << this->QuaternB << "\n";
  os << indent << "QuaternC: " << this->QuaternC << "\n";
  os << indent << "QuaternD: " << this->QuaternD << "\n";
  os << indent << "QOffsetX: " << this->QOffsetX << "\n";
  os << indent << "QOffsetY: " << this->QOffsetY << "\n";
  os << indent << "QOffsetZ: " << this->QOffsetZ << "\n";
  os << indent << "SRowX:";
  for (int i = 0; i < 4; i++)
    {
    os << " " << this->SRowX[i];
    }
  os << "\n";
  os << indent << "SRowY:";
  for (int i = 0; i < 4; i++)
    {
    os << " " << this->SRowY[i];
    }
  os << "\n";
  os << indent << "SRowZ:";
  for (int i = 0; i < 4; i++)
    {
    os << " " << this->SRowZ[i];
    }
  os << "\n";
  os << indent << "IntentName: \"";
  for (size_t j = 0; j < 16 && this->IntentName[j] != '\0'; j++)
    {
    os << (isprint(this->IntentName[j]) ? this->IntentName[j] : '?');
    }
  os << "\"\n";
  os << indent << "Magic: \"";
  for (size_t j = 0; j < 4 && this->Magic[j] != '\0'; j++)
    {
    os << (isprint(this->Magic[j]) ? this->Magic[j] : '?');
    }
  os << "\"\n";
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::SetStringValue(char *x, const char *y, size_t n)
{
  if (y == 0)
    {
    y = "";
    }
  if (strncmp(x, y, n) != 0)
    {
    strncpy(x, y, n);
    x[n] = '\0';
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::SetIntentName(const char *val)
{
  this->SetStringValue(this->IntentName, val, 16);
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::SetDescrip(const char *val)
{
  this->SetStringValue(this->Descrip, val, 80);
}

//----------------------------------------------------------------------------
void vtkNIFTIImageHeader::SetAuxFile(const char *val)
{
  this->SetStringValue(this->AuxFile, val, 24);
}
