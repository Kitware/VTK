/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLRectilinearGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLRectilinearGridReader.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkXMLRectilinearGridReader);

//----------------------------------------------------------------------------
vtkXMLRectilinearGridReader::vtkXMLRectilinearGridReader()
{
  this->CoordinateElements = 0;
}

//----------------------------------------------------------------------------
vtkXMLRectilinearGridReader::~vtkXMLRectilinearGridReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLRectilinearGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLRectilinearGridReader::GetOutput(int idx)
{
  return vtkRectilinearGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLRectilinearGridReader::GetDataSetName()
{
  return "RectilinearGrid";
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::SetOutputExtent(int* extent)
{
  vtkRectilinearGrid::SafeDownCast(
    this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->CoordinateElements = new vtkXMLDataElement*[numPieces];
  for (int i = 0; i < numPieces; ++i)
  {
    this->CoordinateElements[i] = 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::DestroyPieces()
{
  delete [] this->CoordinateElements;
  this->CoordinateElements = 0;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
int vtkXMLRectilinearGridReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if (!this->Superclass::ReadPiece(ePiece))
  {
    return 0;
  }

  // Find the Coordinates element in the piece.
  this->CoordinateElements[this->Piece] = 0;
  for (int i = 0; i < ePiece->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "Coordinates") == 0)
      && (eNested->GetNumberOfNestedElements() == 3))
    {
      this->CoordinateElements[this->Piece] = eNested;
    }
  }

  // If there is any volume, we require a Coordinates element.
  int* piecePointDimensions = this->PiecePointDimensions + this->Piece * 3;
  if (!this->CoordinateElements[this->Piece] &&
    (piecePointDimensions[0] > 0) &&
    (piecePointDimensions[1] > 0) &&
    (piecePointDimensions[2] > 0))
  {
    vtkErrorMacro("A piece is missing its Coordinates element.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  if (!this->CoordinateElements)
  {
    // Empty volume.
    return;
  }

  // Allocate the coordinate arrays.
  vtkRectilinearGrid* output = vtkRectilinearGrid::SafeDownCast(
      this->GetCurrentOutput());

  vtkXMLDataElement* xc = this->CoordinateElements[0]->GetNestedElement(0);
  vtkXMLDataElement* yc = this->CoordinateElements[0]->GetNestedElement(1);
  vtkXMLDataElement* zc = this->CoordinateElements[0]->GetNestedElement(2);

  // Create the coordinate arrays.
  vtkAbstractArray* xa = this->CreateArray(xc);
  vtkAbstractArray* ya = this->CreateArray(yc);
  vtkAbstractArray* za = this->CreateArray(zc);

  vtkDataArray* x = vtkArrayDownCast<vtkDataArray>(xa);
  vtkDataArray* y = vtkArrayDownCast<vtkDataArray>(ya);
  vtkDataArray* z = vtkArrayDownCast<vtkDataArray>(za);
  if (x && y && z)
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
    if (xa)
    {
      xa->Delete();
    }
    if (ya)
    {
      ya->Delete();
    }
    if (za)
    {
      za->Delete();
    }
    this->DataError = 1;
  }
}

//----------------------------------------------------------------------------
int vtkXMLRectilinearGridReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data (we read point specifications here).
  int dims[3] = { 0, 0, 0 };
  this->ComputePointDimensions(this->SubExtent, dims);
  vtkIdType superclassPieceSize =
    (this->NumberOfPointArrays*dims[0]*dims[1]*dims[2]+
     this->NumberOfCellArrays*(dims[0]-1)*(dims[1]-1)*(dims[2]-1));

  // Total amount of data in this piece comes from point/cell data
  // arrays and the point specifications themselves.
  vtkIdType totalPieceSize =
    superclassPieceSize + dims[0] + dims[1] + dims[2];
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[5] =
    {
    0,
    static_cast<float>(superclassPieceSize) / totalPieceSize,
    (static_cast<float>(superclassPieceSize)+dims[0]) / totalPieceSize,
    (static_cast<float>(superclassPieceSize)+dims[1]+dims[2]) / totalPieceSize,
    1
    };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  int index=this->Piece;
  vtkXMLDataElement* xc = this->CoordinateElements[index]->GetNestedElement(0);
  vtkXMLDataElement* yc = this->CoordinateElements[index]->GetNestedElement(1);
  vtkXMLDataElement* zc = this->CoordinateElements[index]->GetNestedElement(2);
  int* pieceExtent = this->PieceExtents + index*6;
  vtkRectilinearGrid* output =
    vtkRectilinearGrid::SafeDownCast(this->GetCurrentOutput());
  int result = 1;

  // Set the range of progress for the X coordinates array.
  this->SetProgressRange(progressRange, 1, fractions);
  if (result)
  {
    this->ReadSubCoordinates(pieceExtent, this->UpdateExtent,
                             this->SubExtent, xc,
                             output->GetXCoordinates());
  }

  // Set the range of progress for the Y coordinates array.
  this->SetProgressRange(progressRange, 2, fractions);
  if (result)
  {
    this->ReadSubCoordinates(pieceExtent+2, this->UpdateExtent+2,
                             this->SubExtent+2, yc,
                             output->GetYCoordinates());
  }

  // Set the range of progress for the Z coordinates array.
  this->SetProgressRange(progressRange, 3, fractions);
  if (result)
  {
    this->ReadSubCoordinates(pieceExtent+4, this->UpdateExtent+4,
                             this->SubExtent+4, zc,
                             output->GetZCoordinates());
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLRectilinearGridReader::ReadSubCoordinates(
  int* inBounds, int* outBounds, int* subBounds,
  vtkXMLDataElement* da, vtkDataArray* array)
{
  unsigned int components = array->GetNumberOfComponents();

  int destStartIndex = subBounds[0] - outBounds[0];
  int sourceStartIndex = subBounds[0] - inBounds[0];
  int length = subBounds[1] - subBounds[0] + 1;

  return this->ReadArrayValues(
    da, destStartIndex*components, array, sourceStartIndex, length);
}

//----------------------------------------------------------------------------
int vtkXMLRectilinearGridReader::FillOutputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkRectilinearGrid");
  return 1;
}
