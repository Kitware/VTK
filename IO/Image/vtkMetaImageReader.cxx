/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMetaImageReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifdef _MSC_VER
#pragma warning(disable:4018)
#endif

#include "vtkMetaImageReader.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <string>
#include "vtkmetaio/metaTypes.h"
#include "vtkmetaio/metaUtils.h"
#include "vtkmetaio/metaEvent.h"
#include "vtkmetaio/metaObject.h"
#include "vtkmetaio/metaImageTypes.h"
#include "vtkmetaio/metaImageUtils.h"
#include "vtkmetaio/metaImage.h"

#include <sys/stat.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMetaImageReader);

//----------------------------------------------------------------------------
vtkMetaImageReader::vtkMetaImageReader()
{
  GantryAngle = 0;
  strcpy(PatientName, "?");
  strcpy(PatientID, "?");
  strcpy(Date, "?");
  strcpy(Series, "?");
  strcpy(Study, "?");
  strcpy(ImageNumber, "?");
  strcpy(Modality, "?");
  strcpy(StudyID, "?");
  strcpy(StudyUID, "?");
  strcpy(TransferSyntaxUID, "?");

  RescaleSlope = 1;
  RescaleOffset = 0;
  BitsAllocated = 0;
  strcpy(DistanceUnits, "mm");
  strcpy(AnatomicalOrientation, "RAS");
  this->MetaImagePtr = new vtkmetaio::MetaImage;
  this->FileLowerLeft = 1;
}

//----------------------------------------------------------------------------
vtkMetaImageReader::~vtkMetaImageReader()
{
  delete this->MetaImagePtr;
}

//----------------------------------------------------------------------------
void vtkMetaImageReader::ExecuteInformation()
{
  if(!this->FileName)
  {
    vtkErrorMacro( << "A filename was not specified." );
    return;
  }

  if(!this->MetaImagePtr->Read(this->FileName, false))
  {
    vtkErrorMacro( << "MetaImage cannot parse file." );
    return;
  }

  this->SetFileDimensionality(this->MetaImagePtr->NDims());
  if ( FileDimensionality <= 0 || FileDimensionality >= 4)
  {
    vtkErrorMacro(
        << "Only understands image data of 1, 2, and 3 dimensions. "
        << "This image has " << FileDimensionality << " dimensions");
    return;
  }
  vtkDebugMacro(<< "* This image has " << FileDimensionality << " dimensions");

  int i;

  switch(this->MetaImagePtr->ElementType())
  {
    case vtkmetaio::MET_NONE:
    case vtkmetaio::MET_ASCII_CHAR:
    case vtkmetaio::MET_LONG_LONG:
    case vtkmetaio::MET_ULONG_LONG:
    case vtkmetaio::MET_STRING:
    case vtkmetaio::MET_LONG_LONG_ARRAY:
    case vtkmetaio::MET_ULONG_LONG_ARRAY:
    case vtkmetaio::MET_FLOAT_ARRAY:
    case vtkmetaio::MET_DOUBLE_ARRAY:
    case vtkmetaio::MET_FLOAT_MATRIX:
    case vtkmetaio::MET_OTHER:
    default:
      vtkErrorMacro(<< "Unknown data type: "
                    << this->MetaImagePtr->ElementType());
      return;
    case vtkmetaio::MET_CHAR:
    case vtkmetaio::MET_CHAR_ARRAY:
      this->DataScalarType = VTK_SIGNED_CHAR;
      break;
    case vtkmetaio::MET_UCHAR:
    case vtkmetaio::MET_UCHAR_ARRAY:
      this->DataScalarType = VTK_UNSIGNED_CHAR;
      break;
    case vtkmetaio::MET_SHORT:
    case vtkmetaio::MET_SHORT_ARRAY:
      this->DataScalarType = VTK_SHORT;
      break;
    case vtkmetaio::MET_USHORT:
    case vtkmetaio::MET_USHORT_ARRAY:
      this->DataScalarType = VTK_UNSIGNED_SHORT;
      break;
    case vtkmetaio::MET_INT:
    case vtkmetaio::MET_INT_ARRAY:
      this->DataScalarType = VTK_INT;
      break;
    case vtkmetaio::MET_UINT:
    case vtkmetaio::MET_UINT_ARRAY:
      this->DataScalarType = VTK_UNSIGNED_INT;
      break;
    case vtkmetaio::MET_LONG:
    case vtkmetaio::MET_LONG_ARRAY:
      this->DataScalarType = VTK_LONG;
      break;
    case vtkmetaio::MET_ULONG:
    case vtkmetaio::MET_ULONG_ARRAY:
      this->DataScalarType = VTK_UNSIGNED_LONG;
      break;
    case vtkmetaio::MET_FLOAT:
      this->DataScalarType = VTK_FLOAT;
      break;
    case vtkmetaio::MET_DOUBLE:
      this->DataScalarType = VTK_DOUBLE;
      break;
  }

  int extent[6]={0,0,0,0,0,0};
  double spacing[3]={1.0, 1.0, 1.0};
  double origin[3]={0.0, 0.0, 0.0};
  for(i=0; i<FileDimensionality; i++)
  {
    extent[2*i] = 0;
    extent[2*i+1] = this->MetaImagePtr->DimSize(i)-1;
    spacing[i] = fabs(this->MetaImagePtr->ElementSpacing(i));
    origin[i] = this->MetaImagePtr->Position(i);
  }
  this->SetNumberOfScalarComponents(
                         this->MetaImagePtr->ElementNumberOfChannels());
  this->SetDataExtent(extent);
  this->SetDataSpacing(spacing);
  this->SetDataOrigin(origin);
  this->SetHeaderSize(this->MetaImagePtr->HeaderSize());
  this->FileLowerLeftOn();

  switch(this->MetaImagePtr->DistanceUnits())
  {
    default:
    case vtkmetaio::MET_DISTANCE_UNITS_UNKNOWN:
    case vtkmetaio::MET_DISTANCE_UNITS_UM:
    {
      strcpy(DistanceUnits, "um");
      break;
    }
    case vtkmetaio::MET_DISTANCE_UNITS_MM:
    {
      strcpy(DistanceUnits, "mm");
      break;
    }
    case vtkmetaio::MET_DISTANCE_UNITS_CM:
    {
      strcpy(DistanceUnits, "cm");
      break;
    }
  }

  strcpy(AnatomicalOrientation,
         this->MetaImagePtr->AnatomicalOrientationAcronym());

  vtkmetaio::MET_SizeOfType(this->MetaImagePtr->ElementType(), &BitsAllocated);

  RescaleSlope = this->MetaImagePtr->ElementToIntensityFunctionSlope();
  RescaleOffset = this->MetaImagePtr->ElementToIntensityFunctionOffset();

  if(this->MetaImagePtr->Modality() == vtkmetaio::MET_MOD_CT)
  {
    strcpy(Modality, "CT");
  }
  else if(this->MetaImagePtr->Modality() == vtkmetaio::MET_MOD_MR)
  {
    strcpy(Modality, "MR");
  }
  else
  {
    strcpy(Modality, "?");
  }

}

void vtkMetaImageReader::ExecuteDataWithInformation(vtkDataObject * output,
                                                    vtkInformation *outInfo)
{

  vtkImageData * data = this->AllocateOutputData(output, outInfo);

  if(!this->FileName)
  {
    vtkErrorMacro( << "A filename was not specified." );
    return;
  }

  data->GetPointData()->GetScalars()->SetName("MetaImage");

  this->ComputeDataIncrements();

  if(!this->MetaImagePtr->Read(this->FileName, true, data->GetScalarPointer()))
  {
    vtkErrorMacro( << "MetaImage cannot read data from file." );
    return;
  }

  this->MetaImagePtr->ElementByteOrderFix();

}

int vtkMetaImageReader::RequestInformation(vtkInformation *,
                               vtkInformationVector **,
                               vtkInformationVector * outputVector )
{

  this->ExecuteInformation();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->DataExtent, 6);
  outInfo->Set(vtkDataObject::SPACING(), this->DataSpacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(),  this->DataOrigin, 3);

  vtkDataObject::SetPointDataActiveScalarInfo(
                    outInfo,
                    this->DataScalarType,
                    this->NumberOfScalarComponents);

  return 1;
}

//----------------------------------------------------------------------------
int vtkMetaImageReader::CanReadFile(const char* fname)
{

  std::string filename = fname;
  if( filename == "" )
  {
    return false;
  }

  bool extensionFound = false;
  std::string::size_type mhaPos = filename.rfind(".mha");
  if ((mhaPos != std::string::npos)
      && (mhaPos == filename.length() - 4))
  {
    extensionFound = true;
  }
  std::string::size_type mhdPos = filename.rfind(".mhd");
  if ((mhdPos != std::string::npos)
      && (mhdPos == filename.length() - 4))
  {
    extensionFound = true;
  }
  if( !extensionFound )
  {
    return false;
  }

  // Now check the file content
  ifstream inputStream;

  inputStream.open( fname, ios::in | ios::binary );

  if( inputStream.fail() )
  {
    return false;
  }

  char key[8000];

  inputStream >> key;

  if( inputStream.eof() )
  {
    inputStream.close();
    return false;
  }

  if( strcmp(key,"NDims")==0 )
  {
    inputStream.close();
    return 3;
  }
  if( strcmp(key,"ObjectType")==0 )
  {
    inputStream.close();
    return 3;
  }
  if( strcmp(key,"TransformType")==0 )
  {
    inputStream.close();
    return 3;
  }
  if( strcmp(key,"ID")==0 )
  {
    inputStream.close();
    return 3;
  }
  if( strcmp(key,"ParentID")==0 )
  {
    inputStream.close();
    return 3;
  }
  if( strcmp(key,"BinaryData")==0 )
  {
    inputStream.close();
    return 3;
  }
  if( strcmp(key,"Comment")==0 )
  {
    inputStream.close();
    return 3;
  }
  if( strcmp(key,"AcquisitionDate")==0 )
  {
    inputStream.close();
    return 3;
  }
  if( strcmp(key,"Modality")==0 )
  {
    inputStream.close();
    return 3;
  }

  inputStream.close();
  return false;
}

//----------------------------------------------------------------------------
int vtkMetaImageReader::GetDataByteOrder(void)
{
  return vtkmetaio::MET_SystemByteOrderMSB();
}

//----------------------------------------------------------------------------
void vtkMetaImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RescaleSlope: " << this->RescaleSlope << endl;
  os << indent << "RescaleOffset: " << this->RescaleOffset << endl;

  os << indent << "GantryAngle: " << this->GantryAngle << endl;
  os << indent << "PatientName: " << this->PatientName << endl;
  os << indent << "PatientID: " << this->PatientID << endl;
  os << indent << "Date: " << this->Date << endl;
  os << indent << "Series: " << this->Series << endl;
  os << indent << "Study: " << this->Study << endl;
  os << indent << "ImageNumber: " << this->ImageNumber << endl;
  os << indent << "Modality: " << this->Modality << endl;
  os << indent << "StudyID: " << this->StudyID << endl;
  os << indent << "StudyUID: " << this->StudyUID << endl;
  os << indent << "TransferSyntaxUID: " << this->TransferSyntaxUID << endl;

  os << indent << "BitsAllocated: " << this->BitsAllocated << endl;
  os << indent << "DistanceUnits: " << this->DistanceUnits << endl;
  os << indent << "AnatomicalOrientation: " << this->AnatomicalOrientation << endl;
}
