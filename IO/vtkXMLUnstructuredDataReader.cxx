/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredDataReader.cxx
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
#include "vtkXMLUnstructuredDataReader.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkPoints.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkPointSet.h"

vtkCxxRevisionMacro(vtkXMLUnstructuredDataReader, "1.1");

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataReader::vtkXMLUnstructuredDataReader()
{  
  this->PointElements = 0;
  this->NumberOfPoints = 0;
  this->TotalNumberOfPoints = 0;
  this->TotalNumberOfCells = 0;
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataReader::~vtkXMLUnstructuredDataReader()
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPointSet* vtkXMLUnstructuredDataReader::GetOutputAsPointSet()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkPointSet*>(this->Outputs[0]);  
}

//----------------------------------------------------------------------------
vtkXMLDataElement*
vtkXMLUnstructuredDataReader
::FindDataArrayWithName(vtkXMLDataElement* eParent, const char* name)
{
  // Find a nested element that represents a data array with the given name.
  int i;
  for(i=0;i < eParent->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = eParent->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "DataArray") == 0)
      {
      const char* aName = eNested->GetAttribute("Name");
      if(aName && (strcmp(aName, name) == 0))
        {
        return eNested;
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataArray*
vtkXMLUnstructuredDataReader
::CreateDataArrayWithName(vtkXMLDataElement* eParent, const char* name)
{
  vtkXMLDataElement* eArray = this->FindDataArrayWithName(eParent, name);
  if(eArray)
    {
    return this->CreateDataArray(eArray);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupOutputTotals()
{
  int i;
  this->TotalNumberOfPoints = 0;
  for(i=this->StartPiece; i < this->EndPiece; ++i)
    {
    this->TotalNumberOfPoints += this->NumberOfPoints[i];
    }  
  this->StartPoint = 0;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupNextPiece()
{
  this->StartPoint += this->NumberOfPoints[this->Piece];
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupUpdateExtent(int piece,
                                                     int numberOfPieces,
                                                     int ghostLevel)
{
  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numberOfPieces;
  this->UpdateGhostLevel = ghostLevel;
  
  // If more pieces are requested than available, just return empty
  // pieces for the extra ones.
  if(this->UpdateNumberOfPieces > this->NumberOfPieces)
    {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
    }
  
  // Find the range of pieces to read.
  if(this->UpdatePiece < this->UpdateNumberOfPieces)
    {
    this->StartPiece = ((this->UpdatePiece*this->NumberOfPieces) /
                        this->UpdateNumberOfPieces);
    this->EndPiece = (((this->UpdatePiece+1)*this->NumberOfPieces) /
                      this->UpdateNumberOfPieces);
    }
  else
    {
    this->StartPiece = 0;
    this->EndPiece = 0;
    }
  
  // Find the total size of the output.
  this->SetupOutputTotals(); 
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::ReadXMLData()
{  
  // Get the update request.
  int piece;
  int numberOfPieces;
  int ghostLevel;
  this->GetOutputUpdateExtent(piece, numberOfPieces, ghostLevel);
  
  vtkDebugMacro("Updating piece " << piece << " of " << numberOfPieces
                << " with ghost level " << ghostLevel);
  
  // Setup the range of pieces that will be read.
  this->SetupUpdateExtent(piece, numberOfPieces, ghostLevel);
  
  // If there are no data to read, stop now.
  if(this->StartPiece == this->EndPiece)
    {
    return;
    }
  
  vtkDebugMacro("Reading piece range [" << this->StartPiece
                << ", " << this->EndPiece << ") from file.");  
  
  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();  
  
  // Read the data needed from each piece.
  int i;
  for(i=this->StartPiece; i < this->EndPiece; ++i)
    {
    this->Superclass::ReadPieceData(i);
    this->SetupNextPiece();
    }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->NumberOfPoints = new vtkIdType[numPieces];
  this->PointElements = new vtkXMLDataElement*[numPieces];
  int i;
  for(i=0;i < numPieces; ++i)
    {
    this->PointElements[i] = 0;
    this->NumberOfPoints[i] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::DestroyPieces()
{
  delete [] this->PointElements;
  delete [] this->NumberOfPoints;
  this->PointElements = 0;
  this->NumberOfPoints = 0;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataReader::GetNumberOfPoints()
{
  return this->TotalNumberOfPoints;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataReader::GetNumberOfCells()
{
  return this->TotalNumberOfCells;
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();
  
  vtkPointSet* output = this->GetOutputAsPointSet();
  
  // Set the maximum number of pieces that can be provided by this
  // reader.
  output->SetMaximumNumberOfPieces(this->NumberOfPieces);
  
  // Create the points array.
  vtkDataArray* array =
    this->CreateDataArray(this->PointElements[this->Piece]->GetNestedElement(0));
  vtkPoints* points = vtkPoints::New();
  points->SetData(array);
  output->SetPoints(points);
  points->Delete();
  array->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  // Allocate the points array.
  vtkPointSet* output = this->GetOutputAsPointSet();
  output->GetPoints()->GetData()->SetNumberOfTuples(this->GetNumberOfPoints());
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if(!this->Superclass::ReadPiece(ePiece)) { return 0; }
  
  if(!ePiece->GetScalarAttribute("NumberOfPoints",
                                 this->NumberOfPoints[this->Piece]))
    {
    vtkErrorMacro("Piece " << this->Piece
                  << " is missing its NumberOfPoints attribute.");
    this->NumberOfPoints[this->Piece] = 0;
    return 0;
    }
  
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
int vtkXMLUnstructuredDataReader::ReadPieceData()
{
  if(!this->Superclass::ReadPieceData()) { return 0; }
  
  vtkPointSet* output = this->GetOutputAsPointSet();
  
  // Read the points array.
  vtkXMLDataElement* ePoints = this->PointElements[this->Piece];
  if(!this->ReadArrayForPoints(ePoints->GetNestedElement(0),
                               output->GetPoints()->GetData()))
    {
    return 0;
    }  
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadCellArray(vtkIdType numberOfCells,
                                                vtkIdType totalNumberOfCells,
                                                vtkXMLDataElement* eCells,
                                                vtkCellArray* outCells)
{
  if(numberOfCells <= 0)
    {
    return 1;
    }
  else if(!eCells)
    {
    return 0;
    }
  
  // Read the cell offsets.
  vtkDataArray* c1 = this->CreateDataArrayWithName(eCells, "offsets");
  if(!c1)
    {
    vtkErrorMacro("Cannot read cell offsets from " << eCells->GetName()
                  << " in piece " << this->Piece);
    return 0;
    }
  c1->SetNumberOfTuples(numberOfCells);
  if(!this->ReadData(eCells->GetNestedElement(1), c1->GetVoidPointer(0),
                     c1->GetDataType(), 0, numberOfCells))
    {
    vtkErrorMacro("Cannot read cell offsets from " << eCells->GetName()
                  << " in piece " << this->Piece);
    return 0;
    }
  vtkIdTypeArray* cellOffsets = vtkIdTypeArray::SafeDownCast(c1);  
  
  // Read the cell points.
  vtkIdType cpLength = cellOffsets->GetValue(numberOfCells-1);
  vtkDataArray* c0 = this->CreateDataArrayWithName(eCells, "connectivity");
  if(!c0)
    {
    vtkErrorMacro("Cannot read cell connectivity from " << eCells->GetName()
                  << " in piece " << this->Piece);
    return 0;
    }
  c0->SetNumberOfTuples(cpLength);
  if(!this->ReadData(eCells->GetNestedElement(0), c0->GetVoidPointer(0),
                     c0->GetDataType(), 0, cpLength))
    {
    vtkErrorMacro("Cannot read cell connectivity from " << eCells->GetName()
                  << " in piece " << this->Piece);
    return 0;
    }
  vtkIdTypeArray* cellPoints = vtkIdTypeArray::SafeDownCast(c0);
  
  // Allocate memory in the output connectivity array.
  vtkIdType curSize = 0;
  if(outCells->GetData())
    {
    curSize = outCells->GetData()->GetNumberOfTuples();
    }
  vtkIdType newSize = curSize+numberOfCells+cellPoints->GetNumberOfTuples();
  vtkIdType* cptr = outCells->WritePointer(totalNumberOfCells, newSize);
  cptr += curSize;
  
  // Copy the connectivity data.
  vtkIdType i;
  vtkIdType previousOffset = 0;
  for(i=0; i < numberOfCells; ++i)
    {
    vtkIdType length = cellOffsets->GetValue(i)-previousOffset;
    *cptr++ = length;
    vtkIdType* sptr = cellPoints->GetPointer(previousOffset);
    // Copy the point indices, but increment them for the appended
    // version's index.
    vtkIdType j;
    for(j=0;j < length; ++j)
      {
      cptr[j] = sptr[j]+this->StartPoint;
      }
    cptr += length;
    previousOffset += length;
    }
  
  cellPoints->Delete();
  cellOffsets->Delete();
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadArrayForPoints(vtkXMLDataElement* da,
                                                     vtkDataArray* outArray)
{
  vtkIdType startPoint = this->StartPoint;
  vtkIdType numPoints = this->NumberOfPoints[this->Piece];  
  vtkIdType components = outArray->GetNumberOfComponents();
  return this->ReadData(da, outArray->GetVoidPointer(startPoint*components),
                        outArray->GetDataType(), 0, numPoints*components);
}
