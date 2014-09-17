/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPStructuredGridReader.h"

#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkStructuredGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkXMLPStructuredGridReader);

//----------------------------------------------------------------------------
vtkXMLPStructuredGridReader::vtkXMLPStructuredGridReader()
{
}

//----------------------------------------------------------------------------
vtkXMLPStructuredGridReader::~vtkXMLPStructuredGridReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLPStructuredGridReader::GetOutput(int idx)
{
  return vtkStructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLPStructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLPStructuredGridReader::GetPieceInput(int index)
{
  vtkXMLStructuredGridReader* reader =
    static_cast<vtkXMLStructuredGridReader*>(this->PieceReaders[index]);
  return reader->GetOutput();
}

//----------------------------------------------------------------------------
const char* vtkXMLPStructuredGridReader::GetDataSetName()
{
  return "PStructuredGrid";
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetOutputExtent(int* extent)
{
  vtkStructuredGrid::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::GetPieceInputExtent(int index, int* extent)
{
  this->GetPieceInput(index)->GetExtent(extent);
}

//----------------------------------------------------------------------------
int
  vtkXMLPStructuredGridReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
    {
    return 0;
    }

  // Find the PPoints element.
  this->PPointsElement = 0;
  int numNested = ePrimary->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "PPoints") == 0) &&
      (eNested->GetNumberOfNestedElements() == 1))
      {
      this->PPointsElement = eNested;
      }
    }

  if (!this->PPointsElement)
    {
    int extent[6];
    this->GetCurrentOutputInformation()->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
    if ((extent[0] <= extent[1]) &&
      (extent[2] <= extent[3]) &&
      (extent[4] <= extent[5]))
      {
      vtkErrorMacro("Could not find PPoints element with 1 array.");
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  // Create the points array.
  vtkPoints* points = vtkPoints::New();
  if (this->PPointsElement)
    {
    // Non-zero volume.
    vtkAbstractArray* aa =
      this->CreateArray(this->PPointsElement->GetNestedElement(0));
    vtkDataArray* a = vtkDataArray::SafeDownCast(aa);
    if(a)
      {
      a->SetNumberOfTuples(this->GetNumberOfPoints());
      points->SetData(a);
      a->Delete();
      }
    else
      {
      if (aa)
        {
        aa->Delete();
        }
      this->DataError = 1;
      }
    }
  vtkStructuredGrid::SafeDownCast(this->GetCurrentOutput())->SetPoints(points);
  points->Delete();
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredGridReader::ReadPieceData()
{
  if (!this->Superclass::ReadPieceData())
    {
    return 0;
    }

  // Copy the points.
  vtkStructuredGrid* input = this->GetPieceInput(this->Piece);
  vtkStructuredGrid* output =
    vtkStructuredGrid::SafeDownCast(this->GetCurrentOutput());

  this->CopyArrayForPoints(
    input->GetPoints()->GetData(), output->GetPoints()->GetData());

  return 1;
}

//---------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPStructuredGridReader::CreatePieceReader()
{
  return vtkXMLStructuredGridReader::New();
}

//---------------------------------------------------------------------------
int vtkXMLPStructuredGridReader::FillOutputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkStructuredGrid");
  return 1;
}
