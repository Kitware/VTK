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

vtkCxxRevisionMacro(vtkXMLPImageDataReader, "1.4");
vtkStandardNewMacro(vtkXMLPImageDataReader);

//----------------------------------------------------------------------------
vtkXMLPImageDataReader::vtkXMLPImageDataReader()
{
  // Copied from vtkImageDataReader constructor:
  this->SetOutput(vtkImageData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
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
  this->Superclass::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataReader::GetOutput()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkImageData*>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataReader::GetOutput(int idx)
{
  return static_cast<vtkImageData*>(this->Superclass::GetOutput(idx));
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
void vtkXMLPImageDataReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();
  vtkImageData* output = this->GetOutput();  
  
  output->SetOrigin(this->Origin);
  output->SetSpacing(this->Spacing);
  
 // Backward-compatability support for scalar information in output.
  vtkDataArray* scalars = output->GetPointData()->GetScalars();
  if(scalars)
    {
    output->SetScalarType(scalars->GetDataType());
    output->SetNumberOfScalarComponents(scalars->GetNumberOfComponents());
    }  
}

//----------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPImageDataReader::CreatePieceReader()
{
  return vtkXMLImageDataReader::New();
}
