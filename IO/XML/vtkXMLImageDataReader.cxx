/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLImageDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLImageDataReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLImageDataReader);

//----------------------------------------------------------------------------
vtkXMLImageDataReader::vtkXMLImageDataReader() = default;

//----------------------------------------------------------------------------
vtkXMLImageDataReader::~vtkXMLImageDataReader() = default;

//----------------------------------------------------------------------------
void vtkXMLImageDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLImageDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLImageDataReader::GetOutput(int idx)
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLImageDataReader::GetDataSetName()
{
  return "ImageData";
}

//----------------------------------------------------------------------------
void vtkXMLImageDataReader::SetOutputExtent(int* extent)
{
  vtkImageData::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
int vtkXMLImageDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Get the image's origin.
  if (ePrimary->GetVectorAttribute("Origin", 3, this->Origin) != 3)
  {
    this->Origin[0] = 0;
    this->Origin[1] = 0;
    this->Origin[2] = 0;
  }

  // Get the image's spacing.
  if (ePrimary->GetVectorAttribute("Spacing", 3, this->Spacing) != 3)
  {
    this->Spacing[0] = 1;
    this->Spacing[1] = 1;
    this->Spacing[2] = 1;
  }

  // Get the image's direction.
  if (ePrimary->GetVectorAttribute("Direction", 9, this->Direction) != 9)
  {
    this->Direction[0] = 1;
    this->Direction[1] = 0;
    this->Direction[2] = 0;
    this->Direction[3] = 0;
    this->Direction[4] = 1;
    this->Direction[5] = 0;
    this->Direction[6] = 0;
    this->Direction[7] = 0;
    this->Direction[8] = 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void vtkXMLImageDataReader::SetupOutputInformation(vtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);
  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);
  outInfo->Set(vtkDataObject::DIRECTION(), this->Direction, 9);
}

//----------------------------------------------------------------------------
void vtkXMLImageDataReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
  vtkInformation* localInfo = this->GetExecutive()->GetOutputInformation(port);
  if (localInfo->Has(vtkDataObject::ORIGIN()))
  {
    outInfo->CopyEntry(localInfo, vtkDataObject::ORIGIN());
  }
  if (localInfo->Has(vtkDataObject::SPACING()))
  {
    outInfo->CopyEntry(localInfo, vtkDataObject::SPACING());
  }
  if (localInfo->Has(vtkDataObject::DIRECTION()))
  {
    outInfo->CopyEntry(localInfo, vtkDataObject::DIRECTION());
  }
}

//----------------------------------------------------------------------------
int vtkXMLImageDataReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
