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
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkXMLDataElement.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformationVector.h"

vtkStandardNewMacro(vtkXMLImageDataReader);

//----------------------------------------------------------------------------
vtkXMLImageDataReader::vtkXMLImageDataReader()
{
}

//----------------------------------------------------------------------------
vtkXMLImageDataReader::~vtkXMLImageDataReader()
{
}

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
  return vtkImageData::SafeDownCast( this->GetOutputDataObject(idx) );
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
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }

  // Get the image's origin.
  if(ePrimary->GetVectorAttribute("Origin", 3, this->Origin) != 3)
    {
    this->Origin[0] = 0;
    this->Origin[1] = 0;
    this->Origin[2] = 0;
    }

  // Get the image's spacing.
  if(ePrimary->GetVectorAttribute("Spacing", 3, this->Spacing) != 3)
    {
    this->Spacing[0] = 1;
    this->Spacing[1] = 1;
    this->Spacing[2] = 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void vtkXMLImageDataReader::SetupOutputInformation(vtkInformation *outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);
  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);

  double bounds[6];
  bounds[0] = this->Origin[0] + this->Spacing[0] * this->WholeExtent[0];
  bounds[1] = this->Origin[0] + this->Spacing[0] * this->WholeExtent[1];
  bounds[2] = this->Origin[1] + this->Spacing[1] * this->WholeExtent[2];
  bounds[3] = this->Origin[1] + this->Spacing[1] * this->WholeExtent[3];
  bounds[4] = this->Origin[2] + this->Spacing[2] * this->WholeExtent[4];
  bounds[5] = this->Origin[2] + this->Spacing[2] * this->WholeExtent[5];
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
               bounds, 6);
}


//----------------------------------------------------------------------------
void vtkXMLImageDataReader::CopyOutputInformation(vtkInformation *outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
  vtkInformation *localInfo =
    this->GetExecutive()->GetOutputInformation( port );
  if ( localInfo->Has(vtkDataObject::ORIGIN()) )
    {
    outInfo->CopyEntry( localInfo, vtkDataObject::ORIGIN() );
    }
  if ( localInfo->Has(vtkDataObject::SPACING()) )
    {
    outInfo->CopyEntry( localInfo, vtkDataObject::SPACING() );
    }
}


//----------------------------------------------------------------------------
int vtkXMLImageDataReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLImageDataReader::SetupUpdateExtentInformation(
  vtkInformation *outInfo)
{
  int pieceNum = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  if (pieceNum > -1)
    {
    int* pieceExtent = this->PieceExtents + pieceNum*6;

    static double bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

    bounds[0] = this->Origin[0]+pieceExtent[0]*this->Spacing[0];
    bounds[1] = this->Origin[0]+pieceExtent[1]*this->Spacing[0];
    bounds[2] = this->Origin[1]+pieceExtent[2]*this->Spacing[1];
    bounds[3] = this->Origin[1]+pieceExtent[3]*this->Spacing[1];
    bounds[4] = this->Origin[2]+pieceExtent[4]*this->Spacing[2];
    bounds[5] = this->Origin[2]+pieceExtent[5]*this->Spacing[2];

    outInfo->Set(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(),
      bounds, 6);
    }

  this->Superclass::SetupUpdateExtentInformation(outInfo);
}
