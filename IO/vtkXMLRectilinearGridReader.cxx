/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLRectilinearGridReader.cxx
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
#include "vtkXMLRectilinearGridReader.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkFloatArray.h"
#include "vtkRectilinearGrid.h"

vtkCxxRevisionMacro(vtkXMLRectilinearGridReader, "1.1");
vtkStandardNewMacro(vtkXMLRectilinearGridReader);

//----------------------------------------------------------------------------
vtkXMLRectilinearGridReader::vtkXMLRectilinearGridReader()
{
  // Copied from vtkRectilinearGridReader constructor:
  this->SetOutput(vtkRectilinearGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
  this->CoordinateElements = 0;
}

//----------------------------------------------------------------------------
vtkXMLRectilinearGridReader::~vtkXMLRectilinearGridReader()
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::SetOutput(vtkRectilinearGrid *output)
{
  this->Superclass::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLRectilinearGridReader::GetOutput()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkRectilinearGrid*>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
const char* vtkXMLRectilinearGridReader::GetDataSetName()
{
  return "RectilinearGrid";
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::SetOutputExtent(int* extent)
{
  this->GetOutput()->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->CoordinateElements = new vtkXMLDataElement*[numPieces];
  int i;
  for(i=0;i < numPieces; ++i)
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
  if(!this->Superclass::ReadPiece(ePiece)) { return 0; }
  
  // Find the Coordinates element in the piece.
  int i;
  this->CoordinateElements[this->Piece] = 0;
  for(i=0; i < ePiece->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if((strcmp(eNested->GetName(), "Coordinates") == 0)
       && (eNested->GetNumberOfNestedElements() == 3))
      {
      this->CoordinateElements[this->Piece] = eNested;
      }
    }
  
  if(!this->CoordinateElements[this->Piece])
    {
    vtkErrorMacro("A piece is missing its Coordinates element.");
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();  
  vtkRectilinearGrid* output = this->GetOutput();
  
  // Create the coordinate arrays.
  vtkDataArray* x = vtkFloatArray::New();
  vtkDataArray* y = vtkFloatArray::New();
  vtkDataArray* z = vtkFloatArray::New();
  output->SetXCoordinates(x);
  output->SetYCoordinates(y);
  output->SetZCoordinates(z);
  x->Delete();
  y->Delete();
  z->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLRectilinearGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  // Allocate the coordinate arrays.
  vtkRectilinearGrid* output = this->GetOutput();  
  output->GetXCoordinates()->SetNumberOfTuples(this->PointDimensions[0]);
  output->GetYCoordinates()->SetNumberOfTuples(this->PointDimensions[1]);
  output->GetZCoordinates()->SetNumberOfTuples(this->PointDimensions[2]);
}

//----------------------------------------------------------------------------
int vtkXMLRectilinearGridReader::ReadPieceData()
{
  if(!this->Superclass::ReadPieceData()) { return 0; }
  int index=this->Piece;
  vtkXMLDataElement* xc = this->CoordinateElements[index]->GetNestedElement(0);
  vtkXMLDataElement* yc = this->CoordinateElements[index]->GetNestedElement(1);
  vtkXMLDataElement* zc = this->CoordinateElements[index]->GetNestedElement(2);
  int* pieceExtent = this->PieceExtents + index*6;
  vtkRectilinearGrid* output = this->GetOutput();
  return
    this->ReadSubCoordinates(pieceExtent, this->UpdateExtent,
                             this->SubExtent, xc,
                             output->GetXCoordinates()) &&
    this->ReadSubCoordinates(pieceExtent+2, this->UpdateExtent+2,
                             this->SubExtent+2, yc,
                             output->GetYCoordinates()) &&
    this->ReadSubCoordinates(pieceExtent+4, this->UpdateExtent+4,
                             this->SubExtent+4, zc,
                             output->GetZCoordinates());
}

//----------------------------------------------------------------------------
int vtkXMLRectilinearGridReader::ReadSubCoordinates(int* inBounds,
                                                    int* outBounds,
                                                    int* subBounds,
                                                    vtkXMLDataElement* da,
                                                    vtkDataArray* array)
{
  unsigned int components = array->GetNumberOfComponents();
  
  int destStartIndex = subBounds[0] - outBounds[0];
  int sourceStartIndex = subBounds[0] - inBounds[0];
  int length = subBounds[1] - subBounds[0] + 1;
  
  return this->ReadData(da, array->GetVoidPointer(destStartIndex*components),
                        array->GetDataType(), sourceStartIndex, length);
}
