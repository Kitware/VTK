/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredGridReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLStructuredGridReader.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkFloatArray.h"
#include "vtkStructuredGrid.h"

vtkCxxRevisionMacro(vtkXMLStructuredGridReader, "1.1");
vtkStandardNewMacro(vtkXMLStructuredGridReader);

//----------------------------------------------------------------------------
vtkXMLStructuredGridReader::vtkXMLStructuredGridReader()
{
  // Copied from vtkStructuredGridReader constructor:
  this->SetOutput(vtkStructuredGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
  this->PointElements = 0;
}

//----------------------------------------------------------------------------
vtkXMLStructuredGridReader::~vtkXMLStructuredGridReader()
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridReader::SetOutput(vtkStructuredGrid *output)
{
  this->Superclass::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLStructuredGridReader::GetOutput()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkStructuredGrid*>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
const char* vtkXMLStructuredGridReader::GetDataSetName()
{
  return "StructuredGrid";
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridReader::SetOutputExtent(int* extent)
{
  this->GetOutput()->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->PointElements = new vtkXMLDataElement*[numPieces];
  int i;
  for(i=0;i < numPieces; ++i)
    {
    this->PointElements[i] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridReader::DestroyPieces()
{
  delete [] this->PointElements;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
int vtkXMLStructuredGridReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if(!this->Superclass::ReadPiece(ePiece)) { return 0; }
  
  // Find the Points element in the piece.
  int i;
  this->PointElements[this->Piece] = 0;
  for(i=0; i < ePiece->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if((strcmp(eNested->GetName(), "Points") == 0)
       && (eNested->GetNumberOfNestedElements() == 1))
      {
      this->PointElements[this->Piece] = eNested;
      }
    }
  
  if(!this->PointElements[this->Piece])
    {
    vtkErrorMacro("A piece is missing its Points element.");
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();  
  vtkStructuredGrid* output = this->GetOutput();
  
  // Create the points array.
  vtkPoints* points = vtkPoints::New();
  
  vtkDataArray* a = vtkFloatArray::New();
  a->SetNumberOfComponents(3);
  points->SetData(a);
  a->Delete();
  
  output->SetPoints(points);
  points->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  // Allocate the points array.
  vtkStructuredGrid* output = this->GetOutput();
  output->GetPoints()->GetData()->SetNumberOfTuples(this->GetNumberOfPoints());
}

//----------------------------------------------------------------------------
int vtkXMLStructuredGridReader::ReadPieceData()
{
  if(!this->Superclass::ReadPieceData()) { return 0; }
  
  // Read the points array.
  vtkStructuredGrid* output = this->GetOutput();
  vtkXMLDataElement* ePoints = this->PointElements[this->Piece];  
  return this->ReadArrayForPoints(ePoints->GetNestedElement(0),
                                  output->GetPoints()->GetData());
}
