/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPImageDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPImageDataReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLImageDataReader.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkXMLPImageDataReader, "1.5");
vtkStandardNewMacro(vtkXMLPImageDataReader);

//----------------------------------------------------------------------------
vtkXMLPImageDataReader::vtkXMLPImageDataReader()
{
  vtkImageData *output = vtkImageData::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkXMLPImageDataReader::~vtkXMLPImageDataReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataReader::SetOutput(vtkImageData *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataReader::GetOutput(int idx)
{
  return vtkImageData::SafeDownCast( this->GetOutputDataObject(idx) );
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataReader::GetPieceInput(int index)
{
  vtkXMLImageDataReader* reader =
    static_cast<vtkXMLImageDataReader*>(this->PieceReaders[index]);
  return reader->GetOutput();
}

//----------------------------------------------------------------------------
const char* vtkXMLPImageDataReader::GetDataSetName()
{
  return "PImageData";
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataReader::SetOutputExtent(int* extent)
{
  this->GetOutput()->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataReader::GetPieceInputExtent(int index, int* extent)
{
  this->GetPieceInput(index)->GetExtent(extent);
}

//----------------------------------------------------------------------------
int vtkXMLPImageDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
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
void vtkXMLPImageDataReader::SetupOutputInformation(vtkInformation *outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);
  
  outInfo->Set(vtkDataObject::ORIGIN(),  this->Origin, 3);
  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);

  // Backward-compatability support for scalar information in output.
  if (this->PPointDataElement)
    {
    int i, components, dataType;
    for(i = 0; i < this->PPointDataElement->GetNumberOfNestedElements(); i++)
      {
      vtkXMLDataElement* eNested = this->PPointDataElement->GetNestedElement(i);
      if ( eNested->GetAttribute( "Scalars" ) )
        {
        if (!eNested->GetWordTypeAttribute("type", dataType))
          {
          this->InformationError = 1;
          return;
          }
        if (!eNested->GetScalarAttribute("NumberOfComponents", components))
          {
          this->InformationError = 1;
          return;
          }
        outInfo->Set(vtkDataObject::SCALAR_TYPE(), dataType);
        outInfo->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(), components);
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPImageDataReader::CreatePieceReader()
{
  return vtkXMLImageDataReader::New();
}
