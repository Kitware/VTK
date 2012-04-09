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
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkXMLStructuredGridReader);

//----------------------------------------------------------------------------
vtkXMLStructuredGridReader::vtkXMLStructuredGridReader()
{
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
vtkStructuredGrid* vtkXMLStructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLStructuredGridReader::GetOutput(int idx)
{
  return vtkStructuredGrid::SafeDownCast( this->GetOutputDataObject(idx) );
}
  

//----------------------------------------------------------------------------
const char* vtkXMLStructuredGridReader::GetDataSetName()
{
  return "StructuredGrid";
}

//----------------------------------------------------------------------------
void vtkXMLStructuredGridReader::SetOutputExtent(int* extent)
{
  vtkStructuredGrid::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
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
void vtkXMLStructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  // Create the points array.
  vtkPoints* points = vtkPoints::New();
  
  // Use the configuration of the first piece since all are the same.
  vtkXMLDataElement* ePoints = this->PointElements[0];
  if (ePoints)
    {
    // Non-zero volume.
    vtkAbstractArray* aa = this->CreateArray(ePoints->GetNestedElement(0));
    vtkDataArray* a = vtkDataArray::SafeDownCast(aa);
    if (a)
      {
      // Allocate the points array.
      a->SetNumberOfTuples( this->GetNumberOfPoints() );
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
int vtkXMLStructuredGridReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data (we read point specifications here).
  int dims[3] = {0,0,0};
  this->ComputePointDimensions(this->SubExtent, dims);
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
  vtkStructuredGrid* output = vtkStructuredGrid::SafeDownCast(
      this->GetCurrentOutput());
  vtkXMLDataElement* ePoints = this->PointElements[this->Piece];
  return this->ReadArrayForPoints(ePoints->GetNestedElement(0),
                                  output->GetPoints()->GetData());
}


int vtkXMLStructuredGridReader::FillOutputPortInformation(int, vtkInformation *info)
  {
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkStructuredGrid");
  return 1;
  }
