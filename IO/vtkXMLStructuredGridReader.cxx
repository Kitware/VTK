/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLStructuredGridReader.h"

#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"

vtkCxxRevisionMacro(vtkXMLStructuredGridReader, "1.8");
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
vtkStructuredGrid* vtkXMLStructuredGridReader::GetOutput(int idx)
{
  return static_cast<vtkStructuredGrid*>(this->Superclass::GetOutput(idx));
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
  
  // If there is any volume, we require a Points element.
  int* piecePointDimensions = this->PiecePointDimensions + this->Piece*3;
  if(!this->PointElements[this->Piece] &&
     (piecePointDimensions[0] > 0) &&
     (piecePointDimensions[1] > 0) &&
     (piecePointDimensions[2] > 0))
    {
    vtkErrorMacro("A piece is missing its Points element "
                  "or element does not have exactly 1 array.");
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
  
  // Use the configuration of the first piece since all are the same.
  vtkXMLDataElement* ePoints = this->PointElements[0];
  if(ePoints)
    {
    // Non-empty volume.
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
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data (we read point specifications here).
  int dims[3] = {0,0,0};
  this->ComputeDimensions(this->SubExtent, dims, 1);  
  vtkIdType superclassPieceSize =
    (this->NumberOfPointArrays*dims[0]*dims[1]*dims[2]+
     this->NumberOfCellArrays*(dims[0]-1)*(dims[1]-1)*(dims[2]-1));
  
  // Total amount of data in this piece comes from point/cell data
  // arrays and the point specifications themselves.
  vtkIdType totalPieceSize =
    superclassPieceSize + dims[0]*dims[1]*dims[2];
  if(totalPieceSize == 0)
    {
    totalPieceSize = 1;
    }
  
  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[3] =
    {
      0,
      float(superclassPieceSize) / totalPieceSize,
      1
    };
  
  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Let the superclass read its data.
  if(!this->Superclass::ReadPieceData()) { return 0; }
  
  if(!this->PointElements[this->Piece])
    {
    // Empty volume.
    return 1;
    }
  
  // Set the range of progress for the points array.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Read the points array.
  vtkStructuredGrid* output = this->GetOutput();
  vtkXMLDataElement* ePoints = this->PointElements[this->Piece];
  return this->ReadArrayForPoints(ePoints->GetNestedElement(0),
                                  output->GetPoints()->GetData());
}
