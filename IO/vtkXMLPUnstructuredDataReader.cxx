/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredDataReader.cxx
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
#include "vtkXMLPUnstructuredDataReader.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLUnstructuredDataReader.h"
#include "vtkPointSet.h"
#include "vtkCellArray.h"

vtkCxxRevisionMacro(vtkXMLPUnstructuredDataReader, "1.1");

//----------------------------------------------------------------------------
vtkXMLPUnstructuredDataReader::vtkXMLPUnstructuredDataReader()
{
  this->TotalNumberOfPoints = 0;
  this->TotalNumberOfCells = 0;
}

//----------------------------------------------------------------------------
vtkXMLPUnstructuredDataReader::~vtkXMLPUnstructuredDataReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPointSet* vtkXMLPUnstructuredDataReader::GetOutputAsPointSet()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkPointSet*>(this->Outputs[0]);  
}

//----------------------------------------------------------------------------
vtkPointSet* vtkXMLPUnstructuredDataReader::GetPieceInputAsPointSet(int piece)
{
  vtkXMLDataReader* reader = this->PieceReaders[piece];
  if(!reader) { return 0; }
  if(reader->GetNumberOfOutputs() < 1) { return 0; }
  return static_cast<vtkPointSet*>(reader->GetOutputs()[0]);
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::SetupOutputTotals()
{
  int i;
  this->TotalNumberOfPoints = 0;
  for(i=this->StartPiece; i < this->EndPiece; ++i)
    {
    if(this->PieceReaders[i])
      {
      this->TotalNumberOfPoints += this->PieceReaders[i]->GetNumberOfPoints();
      }
    }  
  this->StartPoint = 0;
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::SetupNextPiece()
{
  if(this->PieceReaders[this->Piece])
    {
    this->StartPoint += this->PieceReaders[this->Piece]->GetNumberOfPoints();
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPUnstructuredDataReader::GetNumberOfPoints()
{
  return this->TotalNumberOfPoints;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPUnstructuredDataReader::GetNumberOfCells()
{
  return this->TotalNumberOfCells;
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();
  
  vtkPointSet* output = this->GetOutputAsPointSet();
  
  // Set the maximum number of pieces that can be provided by this
  // reader.
  output->SetMaximumNumberOfPieces(this->NumberOfPieces);
  
  // Create the points array.
  vtkPoints* points = vtkPoints::New();
  if(this->PPointsElement &&
     this->PPointsElement->GetNumberOfNestedElements())
    {
    vtkDataArray* array =
      this->CreateDataArray(this->PPointsElement->GetNestedElement(0));
    points->SetData(array);
    array->Delete();
    }
  output->SetPoints(points);
  points->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  // Allocate the points array.
  vtkPointSet* output = this->GetOutputAsPointSet();
  output->GetPoints()->GetData()->SetNumberOfTuples(this->GetNumberOfPoints());
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::SetupUpdateExtent(int piece,
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
  
  // Update the information of the pieces we need.
  int i;
  for(i=this->StartPiece; i < this->EndPiece; ++i)
    {
    if(this->CanReadPiece(i))
      {
      this->PieceReaders[i]->UpdateInformation();
      vtkXMLUnstructuredDataReader* pReader =
        static_cast<vtkXMLUnstructuredDataReader*>(this->PieceReaders[i]);
      pReader->SetupUpdateExtent(0, 1, this->UpdateGhostLevel);
      }
    }
  
  // Find the total size of the output.
  this->SetupOutputTotals(); 
}

//----------------------------------------------------------------------------
int
vtkXMLPUnstructuredDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }
  
  // Find the PPoints element.
  this->PPointsElement = 0;
  int i;
  int numNested = ePrimary->GetNumberOfNestedElements();
  for(i=0;i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "PPoints") == 0)
      {
      this->PPointsElement = eNested;
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::ReadXMLData()
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
int vtkXMLPUnstructuredDataReader::ReadPieceData()
{  
  // Use the internal reader to read the piece.
  vtkPointSet* input = this->GetPieceInputAsPointSet(this->Piece);
  input->SetUpdateExtent(0, 1, this->UpdateGhostLevel);
  input->Update();

  vtkPointSet* output = this->GetOutputAsPointSet();
  
  // Copy the points array.
  this->CopyArrayForPoints(input->GetPoints()->GetData(),
                           output->GetPoints()->GetData());
  
  // Let the superclass read the data it wants.
  return this->Superclass::ReadPieceData();
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::CopyArrayForPoints(vtkDataArray* inArray,
                                                       vtkDataArray* outArray)
{
  if(!this->PieceReaders[this->Piece])
    {
    return;
    }
  
  vtkIdType numPoints = this->PieceReaders[this->Piece]->GetNumberOfPoints();
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType tupleSize = inArray->GetDataTypeSize()*components;
  memcpy(outArray->GetVoidPointer(this->StartPoint*components),
         inArray->GetVoidPointer(0), numPoints*tupleSize);
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::CopyCellArray(vtkIdType totalNumberOfCells,
                                                  vtkCellArray* inCells,
                                                  vtkCellArray* outCells)
{
  // Allocate memory in the output connectivity array.
  vtkIdType curSize = 0;
  if(outCells->GetData())
    {
    curSize = outCells->GetData()->GetNumberOfTuples();
    }
  vtkIdTypeArray* inData = inCells->GetData();
  vtkIdType newSize = curSize+inData->GetNumberOfTuples();
  vtkIdType* in = inData->GetPointer(0);
  vtkIdType* end = inData->GetPointer(inData->GetNumberOfTuples());
  vtkIdType* out = outCells->WritePointer(totalNumberOfCells, newSize);
  out += curSize;
  
  // Copy the connectivity data.
  while(in < end)
    {
    vtkIdType length = *in++;
    *out++ = length;
    // Copy the point indices, but increment them for the appended
    // version's index.
    vtkIdType j;
    for(j=0;j < length; ++j)
      {
      out[j] = in[j]+this->StartPoint;
      }
    in += length;
    out += length;
    }
}
