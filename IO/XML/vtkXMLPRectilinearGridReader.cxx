/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPRectilinearGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPRectilinearGridReader.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkXMLPRectilinearGridReader);

//----------------------------------------------------------------------------
vtkXMLPRectilinearGridReader::vtkXMLPRectilinearGridReader()
{
}

//----------------------------------------------------------------------------
vtkXMLPRectilinearGridReader::~vtkXMLPRectilinearGridReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLPRectilinearGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLPRectilinearGridReader::GetOutput(int idx)
{
  return vtkRectilinearGrid::SafeDownCast( this->GetOutputDataObject(idx) );
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLPRectilinearGridReader::GetPieceInput(int index)
{
  vtkXMLRectilinearGridReader* reader =
    static_cast<vtkXMLRectilinearGridReader*>(this->PieceReaders[index]);
  return reader->GetOutput();
}

//----------------------------------------------------------------------------
const char* vtkXMLPRectilinearGridReader::GetDataSetName()
{
  return "PRectilinearGrid";
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::SetOutputExtent(int* extent)
{
  vtkRectilinearGrid::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::GetPieceInputExtent(int index, int* extent)
{
  this->GetPieceInput(index)->GetExtent(extent);
}

//----------------------------------------------------------------------------
int
vtkXMLPRectilinearGridReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }

  // Find the PCoordinates element.
  this->PCoordinatesElement = 0;
  int i;
  int numNested = ePrimary->GetNumberOfNestedElements();
  for(i=0;i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if((strcmp(eNested->GetName(), "PCoordinates") == 0) &&
       (eNested->GetNumberOfNestedElements() == 3))
      {
      this->PCoordinatesElement = eNested;
      }
    }

  // If there is any volume, we require a PCoordinates element.
  if(!this->PCoordinatesElement)
    {
    int extent[6];
    vtkInformation* outInfo = this->GetCurrentOutputInformation();
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 extent);
    if((extent[0] <= extent[1]) && (extent[2] <= extent[3]) &&
       (extent[4] <= extent[5]))
      {
      vtkErrorMacro("Could not find PCoordinates element with 3 arrays.");
      return 0;
      }
    }

  return 1;
}


//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  if(!this->PCoordinatesElement)
    {
    // Empty volume.
    return;
    }

  // Allocate the coordinate arrays.
  vtkRectilinearGrid* output = vtkRectilinearGrid::SafeDownCast(
      this->GetCurrentOutput());

  vtkXMLDataElement* xc = this->PCoordinatesElement->GetNestedElement(0);
  vtkXMLDataElement* yc = this->PCoordinatesElement->GetNestedElement(1);
  vtkXMLDataElement* zc = this->PCoordinatesElement->GetNestedElement(2);

  // Create the coordinate arrays (all are data arrays).
  vtkAbstractArray* ax = this->CreateArray(xc);
  vtkAbstractArray* ay = this->CreateArray(yc);
  vtkAbstractArray* az = this->CreateArray(zc);

  vtkDataArray* x = vtkDataArray::SafeDownCast(ax);
  vtkDataArray* y = vtkDataArray::SafeDownCast(ay);
  vtkDataArray* z = vtkDataArray::SafeDownCast(az);
  if(x && y && z)
    {
    x->SetNumberOfTuples(this->PointDimensions[0]);
    y->SetNumberOfTuples(this->PointDimensions[1]);
    z->SetNumberOfTuples(this->PointDimensions[2]);
    output->SetXCoordinates(x);
    output->SetYCoordinates(y);
    output->SetZCoordinates(z);
    x->Delete();
    y->Delete();
    z->Delete();
    }
  else
    {
    if (ax) { ax->Delete(); }
    if (ay) { ay->Delete(); }
    if (az) { az->Delete(); }
    this->DataError = 1;
    }
}

//----------------------------------------------------------------------------
int vtkXMLPRectilinearGridReader::ReadPieceData()
{
  if(!this->Superclass::ReadPieceData()) { return 0; }

  // Copy the coordinates arrays from the input piece.
  vtkRectilinearGrid* input = this->GetPieceInput(this->Piece);
  vtkRectilinearGrid* output = vtkRectilinearGrid::SafeDownCast(
      this->GetCurrentOutput());
  this->CopySubCoordinates(this->SubPieceExtent, this->UpdateExtent,
                           this->SubExtent, input->GetXCoordinates(),
                           output->GetXCoordinates());
  this->CopySubCoordinates(this->SubPieceExtent+2, this->UpdateExtent+2,
                           this->SubExtent+2, input->GetYCoordinates(),
                           output->GetYCoordinates());
  this->CopySubCoordinates(this->SubPieceExtent+4, this->UpdateExtent+4,
                           this->SubExtent+4, input->GetZCoordinates(),
                           output->GetZCoordinates());

  return 1;
}

//----------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPRectilinearGridReader::CreatePieceReader()
{
  return vtkXMLRectilinearGridReader::New();
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridReader::CopySubCoordinates(int* inBounds,
                                                      int* outBounds,
                                                      int* subBounds,
                                                      vtkDataArray* inArray,
                                                      vtkDataArray* outArray)
{
  unsigned int components = inArray->GetNumberOfComponents();
  unsigned int tupleSize = inArray->GetDataTypeSize()*components;

  int destStartIndex = subBounds[0] - outBounds[0];
  int sourceStartIndex = subBounds[0] - inBounds[0];
  int length = subBounds[1] - subBounds[0] + 1;

  memcpy(outArray->GetVoidPointer(destStartIndex*components),
         inArray->GetVoidPointer(sourceStartIndex*components),
         length*tupleSize);
}


int vtkXMLPRectilinearGridReader::FillOutputPortInformation(int, vtkInformation *info)
  {
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkRectilinearGrid");
  return 1;
  }


