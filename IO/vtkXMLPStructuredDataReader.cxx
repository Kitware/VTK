/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredDataReader.cxx
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
#include "vtkXMLPStructuredDataReader.h"

#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkTableExtentTranslator.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLStructuredDataReader.h"

vtkCxxRevisionMacro(vtkXMLPStructuredDataReader, "1.4");

//----------------------------------------------------------------------------
vtkXMLPStructuredDataReader::vtkXMLPStructuredDataReader()
{
  this->ExtentTranslator = vtkTableExtentTranslator::New();
  this->PieceExtents = 0;
}

//----------------------------------------------------------------------------
vtkXMLPStructuredDataReader::~vtkXMLPStructuredDataReader()
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }
  this->ExtentTranslator->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkExtentTranslator* vtkXMLPStructuredDataReader::GetExtentTranslator()
{
  return this->ExtentTranslator;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPStructuredDataReader::GetNumberOfPoints()
{
  return (this->PointDimensions[0]*
          this->PointDimensions[1]*
          this->PointDimensions[2]);
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPStructuredDataReader::GetNumberOfCells()
{
  return (this->CellDimensions[0]*
          this->CellDimensions[1]*
          this->CellDimensions[2]);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataReader::ReadXMLData()
{
  // Get the requested Update Extent.
  this->GetOutputAsDataSet()->GetUpdateExtent(this->UpdateExtent);
  
  vtkDebugMacro("Updating extent "
                << this->UpdateExtent[0] << " " << this->UpdateExtent[1] << " "
                << this->UpdateExtent[2] << " " << this->UpdateExtent[3] << " "
                << this->UpdateExtent[4] << " " << this->UpdateExtent[5]
                << "\n");  
  
  // Prepare increments for the update extent.
  this->ComputeDimensions(this->UpdateExtent, this->PointDimensions, 1);
  this->ComputeIncrements(this->UpdateExtent, this->PointIncrements, 1);
  this->ComputeDimensions(this->UpdateExtent, this->CellDimensions, 0);
  this->ComputeIncrements(this->UpdateExtent, this->CellIncrements, 0);  
  
  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();
  
  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  
  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  float* fractions = new float[this->NumberOfPieces+1];
  int i;
  fractions[0] = 0;
  for(i=0;i < this->NumberOfPieces;++i)
    {
    int* pieceExtent = this->PieceExtents + i*6;
    int pieceDims[3] = {0,0,0};
    // Intersect the extents to get the part we need to read.
    if(this->IntersectExtents(pieceExtent, this->UpdateExtent,
                              this->SubExtent))
      {      
      this->ComputeDimensions(this->SubExtent, pieceDims, 1);
      fractions[i+1] = fractions[i] + pieceDims[0]*pieceDims[1]*pieceDims[2];
      }
    }
  if(fractions[this->NumberOfPieces] == 0)
    {
    fractions[this->NumberOfPieces] = 1;
    }
  for(i=1;i <= this->NumberOfPieces;++i)
    {
    fractions[i] = fractions[i] / fractions[this->NumberOfPieces];
    }
  
  // Read the data needed from each piece.
  for(i=0;i < this->NumberOfPieces;++i)
    {
    // Set the range of progress for this piece.
    this->SetProgressRange(progressRange, i, fractions);
    
    // Intersect the extents to get the part we need to read.
    int* pieceExtent = this->PieceExtents + i*6;
    if(this->IntersectExtents(pieceExtent, this->UpdateExtent,
                              this->SubExtent))
      {
      vtkDebugMacro("Reading extent "
                    << this->SubExtent[0] << " " << this->SubExtent[1] << " "
                    << this->SubExtent[2] << " " << this->SubExtent[3] << " "
                    << this->SubExtent[4] << " " << this->SubExtent[5]
                    << " from piece " << i);
      
      this->ComputeDimensions(this->SubExtent, this->SubPointDimensions, 1);
      this->ComputeDimensions(this->SubExtent, this->SubCellDimensions, 0);
      
      // Read the data from this piece.
      this->Superclass::ReadPieceData(i);
      }
    }
  
  delete [] fractions;
  
  // We filled the exact update extent in the output.
  this->SetOutputExtent(this->UpdateExtent);  
}

//----------------------------------------------------------------------------
int
vtkXMLPStructuredDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }
  vtkDataSet* output = this->GetOutputAsDataSet();
  
  // Read information about the structured data.
  int extent[6];
  if(ePrimary->GetVectorAttribute("WholeExtent", 6, extent) < 6)
    {
    vtkErrorMacro(<< this->GetDataSetName()
                  << " element has no WholeExtent.");
    return 0;
    }
  output->SetWholeExtent(extent);  
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataReader::SetupEmptyOutput()
{
  // Special extent to indicate no input.
  this->GetOutputAsDataSet()->SetUpdateExtent(1, 0, 1, 0, 1, 0);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->ExtentTranslator->SetNumberOfPieces(this->NumberOfPieces);
  this->ExtentTranslator->SetMaximumGhostLevel(this->GhostLevel);
  this->PieceExtents = new int[6*this->NumberOfPieces];
  int i;
  for(i=0;i < this->NumberOfPieces;++i)
    {
    int* extent = this->PieceExtents+i*6;
    extent[0] = 0; extent[1] = -1;
    extent[2] = 0; extent[3] = -1;
    extent[4] = 0; extent[5] = -1;
    }
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataReader::DestroyPieces()
{
  delete [] this->PieceExtents;
  this->PieceExtents = 0;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if(!this->Superclass::ReadPiece(ePiece)) { return 0; }
  int* pieceExtent = this->PieceExtents+this->Piece*6;
  if(ePiece->GetVectorAttribute("Extent", 6, pieceExtent) < 6)
    {
    vtkErrorMacro("Piece " << this->Piece << " has invalid Extent.");
    return 0;
    }
  this->ExtentTranslator->SetExtentForPiece(this->Piece, pieceExtent);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredDataReader::ReadPieceData()
{  
  // Use the internal reader to read the piece.
  vtkDataSet* input = this->GetPieceInputAsDataSet(this->Piece);
  input->SetUpdateExtent(this->SubExtent);
  input->Update();
  
  // Get the actual portion of the piece that was read.
  this->GetPieceInputExtent(this->Piece, this->SubPieceExtent);
  this->ComputeDimensions(this->SubPieceExtent,
                          this->SubPiecePointDimensions, 1);
  this->ComputeIncrements(this->SubPieceExtent,
                          this->SubPiecePointIncrements, 1);
  this->ComputeDimensions(this->SubPieceExtent,
                          this->SubPieceCellDimensions, 0);
  this->ComputeIncrements(this->SubPieceExtent,
                          this->SubPieceCellIncrements, 0);
  
  // Let the superclass read the data it wants.
  return this->Superclass::ReadPieceData();
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataReader::CopyArrayForPoints(vtkDataArray* inArray,
                                                     vtkDataArray* outArray)
{
  this->CopySubExtent(this->SubPieceExtent,
                      this->SubPiecePointDimensions,
                      this->SubPiecePointIncrements,
                      this->UpdateExtent, this->PointDimensions,
                      this->PointIncrements, this->SubExtent,
                      this->SubPointDimensions, inArray, outArray);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataReader::CopyArrayForCells(vtkDataArray* inArray,
                                                    vtkDataArray* outArray)
{
  this->CopySubExtent(this->SubPieceExtent,
                      this->SubPieceCellDimensions,
                      this->SubPieceCellIncrements,
                      this->UpdateExtent, this->CellDimensions,
                      this->CellIncrements, this->SubExtent,
                      this->SubCellDimensions, inArray, outArray);
}

//----------------------------------------------------------------------------
void
vtkXMLPStructuredDataReader
::CopySubExtent(int* inExtent, int* inDimensions, int* inIncrements,
                int* outExtent, int* outDimensions, int* outIncrements,
                int* subExtent, int* subDimensions,
                vtkDataArray* inArray, vtkDataArray* outArray)
{
  unsigned int components = inArray->GetNumberOfComponents();
  unsigned int tupleSize = inArray->GetDataTypeSize()*components;
  
  if((inDimensions[0] == outDimensions[0]) &&
     (inDimensions[1] == outDimensions[1]))
    {
    if(inDimensions[2] == outDimensions[2])
      {
      // Copy the whole volume at once.
      unsigned int volumeTuples = (inDimensions[0]*
                                   inDimensions[1]*
                                   inDimensions[2]);
      memcpy(outArray->GetVoidPointer(0), inArray->GetVoidPointer(0),
             volumeTuples*tupleSize);
      }
    else
      {
      // Copy an entire slice at a time.
      unsigned int sliceTuples = inDimensions[0]*inDimensions[1];
      int k;
      for(k=0;k < subDimensions[2];++k)
        {
        unsigned int sourceTuple = this->GetStartTuple(inExtent, inIncrements,
                                                       subExtent[0],
                                                       subExtent[2],
                                                       subExtent[4]+k);
        unsigned int destTuple = this->GetStartTuple(outExtent, outIncrements,
                                                     subExtent[0],
                                                     subExtent[2],
                                                     subExtent[4]+k);
        memcpy(outArray->GetVoidPointer(destTuple*components),
               inArray->GetVoidPointer(sourceTuple*components),
               sliceTuples*tupleSize);
        }
      }
    }
  else
    {
    // Copy a row at a time.
    unsigned int rowTuples = subDimensions[0];
    int j,k;
    for(k=0;k < subDimensions[2];++k)
      {
      for(j=0;j < subDimensions[1];++j)
        {
        unsigned int sourceTuple = this->GetStartTuple(inExtent, inIncrements,
                                                       subExtent[0],
                                                       subExtent[2]+j,
                                                       subExtent[4]+k);
        unsigned int destTuple = this->GetStartTuple(outExtent, outIncrements,
                                                     subExtent[0],
                                                     subExtent[2]+j,
                                                     subExtent[4]+k);
        memcpy(outArray->GetVoidPointer(destTuple*components),
               inArray->GetVoidPointer(sourceTuple*components),
               rowTuples*tupleSize);
        }
      }
    }
}
