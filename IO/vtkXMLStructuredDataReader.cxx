/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLStructuredDataReader.h"

#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"

vtkCxxRevisionMacro(vtkXMLStructuredDataReader, "1.18");

//----------------------------------------------------------------------------
vtkXMLStructuredDataReader::vtkXMLStructuredDataReader()
{
  this->PieceExtents = 0;
  this->PiecePointDimensions = 0;
  this->PiecePointIncrements = 0;
  this->PieceCellDimensions = 0;
  this->PieceCellIncrements = 0;
  this->WholeSlices = 1;

  // Initialize these in case someone calls GetNumberOfPoints or
  // GetNumberOfCells before UpdateInformation is called.
  this->PointDimensions[0] = 0;
  this->PointDimensions[1] = 0;
  this->PointDimensions[2] = 0;
  this->CellDimensions[0] = 0;
  this->CellDimensions[1] = 0;
  this->CellDimensions[2] = 0;
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataReader::~vtkXMLStructuredDataReader()
{
  if(this->NumberOfPieces)
    {
    this->DestroyPieces();
    }
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WholeSlices: " << this->WholeSlices << "\n";
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  // Set the output's whole extent.
  int extent[6];
  if(ePrimary->GetVectorAttribute("WholeExtent", 6, extent) == 6)
    {
    this->GetOutputAsDataSet(0)->SetWholeExtent(extent);
    }
  else
    {
    vtkErrorMacro(<< this->GetDataSetName() << " element has no WholeExtent.");
    return 0;
    }
  
  return this->Superclass::ReadPrimaryElement(ePrimary);  
}

//----------------------------------------------------------------------------
void
vtkXMLStructuredDataReader::CopyOutputInformation(vtkInformation* outInfo,
                                                  int port)
{
  // Let the superclass copy information first.
  this->Superclass::CopyOutputInformation(outInfo, port);

  // All structured data has a whole extent.
  vtkInformation* localInfo = this->GetExecutive()->GetOutputInformation(port);
  if(localInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
    outInfo->CopyEntry(localInfo,
                       vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    }
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataReader::SetupEmptyOutput()
{
  // Special extent to indicate no input.
  this->GetOutputAsDataSet(0)->SetWholeExtent(0, -1, 0, -1, 0, -1);
  this->GetOutputAsDataSet(0)->SetUpdateExtent(0, -1, 0, -1, 0, -1);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->PieceExtents = new int[numPieces*6];
  this->PiecePointDimensions = new int[numPieces*3];
  this->PiecePointIncrements = new vtkIdType[numPieces*3];
  this->PieceCellDimensions = new int[numPieces*3];
  this->PieceCellIncrements = new vtkIdType[numPieces*3];
  int i;
  for(i=0; i < numPieces; ++i)
    {
    int* extent = this->PieceExtents + i*6;
    extent[0]=0; extent[1]=-1;
    extent[2]=0; extent[3]=-1;
    extent[4]=0; extent[5]=-1;
    }
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataReader::DestroyPieces()
{
  delete [] this->PieceExtents;
  delete [] this->PiecePointDimensions;
  delete [] this->PiecePointIncrements;
  delete [] this->PieceCellDimensions;
  delete [] this->PieceCellIncrements;
  this->PieceExtents = 0;
  this->PiecePointDimensions = 0;
  this->PiecePointIncrements = 0;
  this->PieceCellDimensions = 0;
  this->PieceCellIncrements = 0;
  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLStructuredDataReader::GetNumberOfPoints()
{
  return (this->PointDimensions[0]*
          this->PointDimensions[1]*
          this->PointDimensions[2]);
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLStructuredDataReader::GetNumberOfCells()
{
  return (this->CellDimensions[0]*
          this->CellDimensions[1]*
          this->CellDimensions[2]);
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if(!this->Superclass::ReadPiece(ePiece))
    {
    return 0;
    }
  int* pieceExtent = this->PieceExtents + this->Piece*6;
  
  // Read the extent of the piece.
  if(strcmp(ePiece->GetName(), "Piece") == 0)
    {
    if(!ePiece->GetAttribute("Extent"))
      {
      vtkErrorMacro("Piece has no extent.");
      }
    if(ePiece->GetVectorAttribute("Extent", 6, pieceExtent) < 6)
      {
      vtkErrorMacro("Extent attribute is not 6 integers.");
      return 0;
      }
    }
  else if(ePiece->GetVectorAttribute("WholeExtent", 6, pieceExtent) < 6)
    {
    vtkErrorMacro("WholeExtent attribute is not 6 integers.");
    return 0;
    }
  
  // Compute the dimensions and increments for this piece's extent.
  int* piecePointDimensions = this->PiecePointDimensions + this->Piece*3;
  vtkIdType* piecePointIncrements = this->PiecePointIncrements + this->Piece*3;
  int* pieceCellDimensions = this->PieceCellDimensions + this->Piece*3;
  vtkIdType* pieceCellIncrements = this->PieceCellIncrements + this->Piece*3;  
  this->ComputeDimensions(pieceExtent, piecePointDimensions, 1);
  this->ComputeIncrements(pieceExtent, piecePointIncrements, 1); 
  this->ComputeDimensions(pieceExtent, pieceCellDimensions, 0);
  this->ComputeIncrements(pieceExtent, pieceCellIncrements, 0);
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataReader::ReadXMLData()
{
  // Get the requested Update Extent.
  this->GetOutputAsDataSet(0)->GetUpdateExtent(this->UpdateExtent);
  
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
    else
      {
      fractions[i+1] = 0;
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
  for(i=0;(i < this->NumberOfPieces && !this->AbortExecute &&
           !this->DataError);++i)
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
      if(!this->ReadPieceData(i))
        {
        // An error occurred while reading the piece.
        this->DataError = 1;
        }
      }
    }
  
  delete [] fractions;
  
  // We filled the exact update extent in the output.
  this->SetOutputExtent(this->UpdateExtent);  
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataReader::ReadArrayForPoints(vtkXMLDataElement* da,
                                                   vtkDataArray* outArray)
{
  int* pieceExtent = this->PieceExtents + this->Piece*6;
  int* piecePointDimensions = this->PiecePointDimensions + this->Piece*3;
  vtkIdType* piecePointIncrements = this->PiecePointIncrements + this->Piece*3;
  if(!this->ReadSubExtent(pieceExtent, piecePointDimensions,
                          piecePointIncrements, this->UpdateExtent,
                          this->PointDimensions, this->PointIncrements,
                          this->SubExtent, this->SubPointDimensions,
                          da, outArray))
    {
    vtkErrorMacro("Error reading extent "
                  << this->SubExtent[0] << " " << this->SubExtent[1] << " "
                  << this->SubExtent[2] << " " << this->SubExtent[3] << " "
                  << this->SubExtent[4] << " " << this->SubExtent[5]
                  << " from piece " << this->Piece);
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataReader::ReadArrayForCells(vtkXMLDataElement* da,
                                                  vtkDataArray* outArray)
{
  int* pieceExtent = this->PieceExtents + this->Piece*6;
  int* pieceCellDimensions = this->PieceCellDimensions + this->Piece*3;
  vtkIdType* pieceCellIncrements = this->PieceCellIncrements + this->Piece*3;
  if(!this->ReadSubExtent(pieceExtent, pieceCellDimensions,
                          pieceCellIncrements, this->UpdateExtent,
                          this->CellDimensions, this->CellIncrements,
                          this->SubExtent, this->SubCellDimensions,
                          da, outArray))
    {
    vtkErrorMacro("Error reading extent "
                  << this->SubExtent[0] << " " << this->SubExtent[1] << " "
                  << this->SubExtent[2] << " " << this->SubExtent[3] << " "
                  << this->SubExtent[4] << " " << this->SubExtent[5]
                  << " from piece " << this->Piece);
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int
vtkXMLStructuredDataReader
::ReadSubExtent(int* inExtent, int* inDimensions, vtkIdType* inIncrements,
                int* outExtent, int* outDimensions, vtkIdType* outIncrements,
                int* subExtent, int* subDimensions, vtkXMLDataElement* da,
                vtkDataArray* array)
{
  int components = array->GetNumberOfComponents();
  
  if((inDimensions[0] == outDimensions[0]) &&
     (inDimensions[1] == outDimensions[1]))
    {
    if(inDimensions[2] == outDimensions[2])
      {
      // Read the whole volume at once.  This fills the array's entire
      // progress range.
      vtkIdType volumeTuples =
        (inDimensions[0]*inDimensions[1]*inDimensions[2]);
      if(!this->ReadData(da, array->GetVoidPointer(0), array->GetDataType(),
                         0, volumeTuples*components))
        {
        return 0;
        }
      }
    else
      {
      // Read an entire slice at a time.  Split progress range by
      // slice.
      float progressRange[2] = {0,0};
      this->GetProgressRange(progressRange);
      vtkIdType sliceTuples = inDimensions[0]*inDimensions[1];
      int k;
      for(k=0;k < subDimensions[2] && !this->AbortExecute;++k)
        {
        // Calculate the starting tuples for source and destination.
        vtkIdType sourceTuple =
          this->GetStartTuple(inExtent, inIncrements,
                              subExtent[0], subExtent[2], subExtent[4]+k);
        vtkIdType destTuple =
          this->GetStartTuple(outExtent, outIncrements,
                              subExtent[0], subExtent[2], subExtent[4]+k);
        
        // Set the range of progress for this slice.
        this->SetProgressRange(progressRange, k, subDimensions[2]);
        
        // Read the slice.
        if(!this->ReadData(da, array->GetVoidPointer(destTuple*components),
                           array->GetDataType(), sourceTuple*components,
                           sliceTuples*components))
          {
          return 0;
          }
        }
      }
    }
  else
    {
    if(!this->WholeSlices)
      {
      // Read a row at a time.  Split progress range by row.
      float progressRange[2] = {0,0};
      this->GetProgressRange(progressRange);
      vtkIdType rowTuples = subDimensions[0];
      int j,k;
      for(k=0;k < subDimensions[2] && !this->AbortExecute;++k)
        {
        for(j=0;j < subDimensions[1] && !this->AbortExecute;++j)
          {          
          // Calculate the starting tuples for source and destination.
          vtkIdType sourceTuple =
            this->GetStartTuple(inExtent, inIncrements,
                                subExtent[0], subExtent[2]+j, subExtent[4]+k);
          vtkIdType destTuple =
            this->GetStartTuple(outExtent, outIncrements,
                                subExtent[0], subExtent[2]+j, subExtent[4]+k);
          
          // Set the range of progress for this row.
          this->SetProgressRange(progressRange, subDimensions[1]*k+j,
                                 subDimensions[2]*subDimensions[1]);
          
          // Read the row.
          if(!this->ReadData(da, array->GetVoidPointer(destTuple*components),
                             array->GetDataType(), sourceTuple*components,
                             rowTuples*components))
            {
            return 0;
            }
          }
        }
      }
    else
      {
      // Read in each slice and copy the needed rows from it.  Split
      // progress range by slice.
      float progressRange[2] = {0,0};
      this->GetProgressRange(progressRange);
      vtkIdType rowTuples = subDimensions[0];
      vtkIdType partialSliceTuples = inDimensions[0]*subDimensions[1];
      int tupleSize = components*array->GetDataTypeSize();
      vtkDataArray* temp = array->NewInstance();
      temp->SetNumberOfComponents(array->GetNumberOfComponents());
      temp->SetNumberOfTuples(partialSliceTuples);
      int k;
      for(k=0;k < subDimensions[2] && !this->AbortExecute;++k)
        {
        // Calculate the starting tuple from the input.
        vtkIdType inTuple =
          this->GetStartTuple(inExtent, inIncrements,
                              inExtent[0], subExtent[2], subExtent[4]+k);
        int memExtent[6];
        memExtent[0] = inExtent[0];
        memExtent[1] = inExtent[1];
        memExtent[2] = subExtent[2];
        memExtent[3] = subExtent[3];
        memExtent[4] = subExtent[4]+k;
        memExtent[5] = subExtent[4]+k;
        
        // Set the range of progress for this slice.
        this->SetProgressRange(progressRange, k, subDimensions[2]);
        
        // Read the slice.
        if(!this->ReadData(da, temp->GetVoidPointer(0), temp->GetDataType(),
                           inTuple*components,
                           partialSliceTuples*components))
          {
          temp->Delete();
          return 0;
          }
        
        // Copy the portion of the slice we need.
        int j;
        for(j=0;j < subDimensions[1];++j)
          {
          vtkIdType sourceTuple =
            this->GetStartTuple(memExtent, inIncrements,
                                subExtent[0], subExtent[2]+j, subExtent[4]+k);
          vtkIdType destTuple =
            this->GetStartTuple(outExtent, outIncrements,
                                subExtent[0], subExtent[2]+j, subExtent[4]+k);
          memcpy(array->GetVoidPointer(destTuple*components),
                 temp->GetVoidPointer(sourceTuple*components),
                 tupleSize*rowTuples);
          }
        }
      temp->Delete();
      }
    }
  return 1;
}
