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

vtkCxxRevisionMacro(vtkXMLPStructuredGridReader, "1.7");
vtkStandardNewMacro(vtkXMLPStructuredGridReader);

//----------------------------------------------------------------------------
vtkXMLPStructuredGridReader::vtkXMLPStructuredGridReader()
{
  // Copied from vtkStructuredGridReader constructor:
  this->SetOutput(vtkStructuredGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
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
vtkStructuredGrid* vtkXMLPStructuredGridReader::GetOutput(int idx)
{
  return static_cast<vtkStructuredGrid*>(this->Superclass::GetOutput(idx));
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetOutput(vtkStructuredGrid *output)
{
  this->Superclass::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLPStructuredGridReader::GetOutput()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkStructuredGrid*>(this->Outputs[0]);
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
  this->GetOutput()->SetExtent(extent);
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
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }
  
  // Find the PPoints element.
  this->PPointsElement = 0;
  int i;
  int numNested = ePrimary->GetNumberOfNestedElements();
  for(i=0;i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if((strcmp(eNested->GetName(), "PPoints") == 0) &&
       (eNested->GetNumberOfNestedElements() == 1))
      {
      this->PPointsElement = eNested;
      }
    }
  
  if(!this->PPointsElement)
    {
    int extent[6];
    this->GetOutput()->GetWholeExtent(extent);
    if((extent[0] <= extent[1]) && (extent[2] <= extent[3]) &&
       (extent[4] <= extent[5]))
      {
      vtkErrorMacro("Could not find PPoints element with 1 array.");
      return 0;
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();  
  vtkStructuredGrid* output = this->GetOutput();  
  
  // Create the points array.
  vtkPoints* points = vtkPoints::New();
  vtkXMLDataElement* ePoints = this->PPointsElement;
  if(ePoints)
    {
    // Non-zero volume.
    vtkDataArray* a = this->CreateDataArray(ePoints->GetNestedElement(0));
    if(a)
      {
      points->SetData(a);
      a->Delete();
      }
    else
      {
      this->InformationError = 1;
      }
    }
  output->SetPoints(points);
  points->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  // Allocate the points array.
  vtkDataArray* a = this->GetOutput()->GetPoints()->GetData();
  a->SetNumberOfTuples(this->GetNumberOfPoints());  
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredGridReader::ReadPieceData()
{
  if(!this->Superclass::ReadPieceData()) { return 0; }
  
  // Copy the points.
  vtkStructuredGrid* input = this->GetPieceInput(this->Piece);
  vtkStructuredGrid* output = this->GetOutput();  
  this->CopyArrayForPoints(input->GetPoints()->GetData(),
                           output->GetPoints()->GetData());
  
  return 1;
}

//----------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPStructuredGridReader::CreatePieceReader()
{
  return vtkXMLStructuredGridReader::New();
}
