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

#include "vtkArrayIteratorIncludes.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"


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

  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = -1;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = -1;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = -1;
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataReader::~vtkXMLStructuredDataReader()
{
  if (this->NumberOfPieces)
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
  // Get the whole extent attribute.
  int extent[6];
  if (ePrimary->GetVectorAttribute("WholeExtent", 6, extent) == 6)
  {
    memcpy(this->WholeExtent, extent, 6*sizeof(int));

    // Set the output's whole extent.
    vtkInformation* outInfo = this->GetCurrentOutputInformation();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        extent, 6);

    // Check each axis to see if it has cells.
    for (int a = 0; a < 3; ++a)
    {
      this->AxesEmpty[a] = (extent[2*a+1] > extent[2*a]) ? 0 : 1;
    }
  }
  else
  {
    vtkErrorMacro(<< this->GetDataSetName() << " element has no WholeExtent.");
    return 0;
  }

  return this->Superclass::ReadPrimaryElement(ePrimary);
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataReader::SetupOutputInformation(
  vtkInformation *outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);
}

//----------------------------------------------------------------------------
void
vtkXMLStructuredDataReader::CopyOutputInformation(vtkInformation* outInfo,
                                                  int port)
{
  // Let the superclass copy information first.
  this->Superclass::CopyOutputInformation(outInfo, port);

  // All structured data has a whole extent.
  vtkInformation *localInfo =
    this->GetExecutive()->GetOutputInformation(port);
  if (localInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    outInfo->CopyEntry(
      localInfo, vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  }
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
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

  for (int i = 0; i < numPieces; ++i)
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
  return (static_cast<vtkIdType>(this->PointDimensions[0]) *
          static_cast<vtkIdType>(this->PointDimensions[1]) *
          static_cast<vtkIdType>(this->PointDimensions[2]));
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLStructuredDataReader::GetNumberOfCells()
{
  return (static_cast<vtkIdType>(this->CellDimensions[0]) *
          static_cast<vtkIdType>(this->CellDimensions[1]) *
          static_cast<vtkIdType>(this->CellDimensions[2]));
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if (!this->Superclass::ReadPiece(ePiece))
  {
    return 0;
  }
  int* pieceExtent = this->PieceExtents + this->Piece * 6;

  // Read the extent of the piece.
  if (strcmp(ePiece->GetName(), "Piece") == 0)
  {
    if (!ePiece->GetAttribute("Extent"))
    {
      vtkErrorMacro("Piece has no extent.");
    }
    if (ePiece->GetVectorAttribute("Extent", 6, pieceExtent) < 6)
    {
      vtkErrorMacro("Extent attribute is not 6 integers.");
      return 0;
    }
  }
  else if (ePiece->GetVectorAttribute("WholeExtent", 6, pieceExtent) < 6)
  {
    vtkErrorMacro("WholeExtent attribute is not 6 integers.");
    return 0;
  }

  // Compute the dimensions and increments for this piece's extent.
  int* piecePointDimensions =
    this->PiecePointDimensions + this->Piece * 3;
  vtkIdType* piecePointIncrements =
    this->PiecePointIncrements + this->Piece * 3;
  int* pieceCellDimensions =
    this->PieceCellDimensions + this->Piece * 3;
  vtkIdType* pieceCellIncrements =
    this->PieceCellIncrements + this->Piece * 3;

  this->ComputePointDimensions(pieceExtent, piecePointDimensions);
  this->ComputePointIncrements(pieceExtent, piecePointIncrements);
  this->ComputeCellDimensions(pieceExtent, pieceCellDimensions);
  this->ComputeCellIncrements(pieceExtent, pieceCellIncrements);

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLStructuredDataReader::ReadXMLData()
{
  // Get the requested Update Extent.
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      this->UpdateExtent);

  // For debugging
  /*
  int numPieces = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numGhosts = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (piece == 0)
    {
    cout << "Piece:" << piece << " " << numPieces << " " << numGhosts << endl;
    cout << "Extent: "
         << this->UpdateExtent[0] << " "
         << this->UpdateExtent[1] << " "
         << this->UpdateExtent[2] << " "
         << this->UpdateExtent[3] << " "
         << this->UpdateExtent[4] << " "
         << this->UpdateExtent[5] << endl;
    }
  */

  vtkDebugMacro("Updating extent "
    << this->UpdateExtent[0] << " " << this->UpdateExtent[1] << " "
    << this->UpdateExtent[2] << " " << this->UpdateExtent[3] << " "
    << this->UpdateExtent[4] << " " << this->UpdateExtent[5] << "\n");

  // Prepare increments for the update extent.
  this->ComputePointDimensions(this->UpdateExtent, this->PointDimensions);
  this->ComputePointIncrements(this->UpdateExtent, this->PointIncrements);
  this->ComputeCellDimensions(this->UpdateExtent, this->CellDimensions);
  this->ComputeCellIncrements(this->UpdateExtent, this->CellIncrements);

  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();

  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  float* fractions = new float[this->NumberOfPieces+1];
  fractions[0] = 0;
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    int* pieceExtent = this->PieceExtents + i*6;
    int pieceDims[3] = {0,0,0};
    // Intersect the extents to get the part we need to read.
    if (this->IntersectExtents(
      pieceExtent, this->UpdateExtent, this->SubExtent))
    {
      this->ComputePointDimensions(this->SubExtent, pieceDims);
      fractions[i+1] =
        fractions[i] + pieceDims[0] * pieceDims[1] * pieceDims[2];
    }
    else
    {
      fractions[i+1] = 0;
    }
  }
  if (fractions[this->NumberOfPieces] == 0)
  {
    fractions[this->NumberOfPieces] = 1;
  }
  for (int i = 1;i <= this->NumberOfPieces; ++i)
  {
    fractions[i] = fractions[i] / fractions[this->NumberOfPieces];
  }

  // Read the data needed from each piece.
  for (int i = 0;
    (i < this->NumberOfPieces && !this->AbortExecute && !this->DataError); ++i)
  {
    // Set the range of progress for this piece.
    this->SetProgressRange(progressRange, i, fractions);

    // Intersect the extents to get the part we need to read.
    int* pieceExtent = this->PieceExtents + i * 6;
    if (this->IntersectExtents(pieceExtent, this->UpdateExtent,
                              this->SubExtent))
    {
      vtkDebugMacro("Reading extent "
        << this->SubExtent[0] << " " << this->SubExtent[1] << " "
        << this->SubExtent[2] << " " << this->SubExtent[3] << " "
        << this->SubExtent[4] << " " << this->SubExtent[5]
        << " from piece " << i);

      this->ComputePointDimensions(this->SubExtent, this->SubPointDimensions);
      this->ComputeCellDimensions(this->SubExtent, this->SubCellDimensions);

      // Read the data from this piece.
      if (!this->ReadPieceData(i))
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
                                                   vtkAbstractArray* outArray)
{
  int* pieceExtent = this->PieceExtents + this->Piece*6;
  int* piecePointDimensions = this->PiecePointDimensions + this->Piece*3;
  vtkIdType* piecePointIncrements = this->PiecePointIncrements + this->Piece*3;
  if (!this->ReadSubExtent(pieceExtent, piecePointDimensions,
                           piecePointIncrements, this->UpdateExtent,
                           this->PointDimensions, this->PointIncrements,
                           this->SubExtent, this->SubPointDimensions,
                           da, outArray, POINT_DATA))
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
                                                  vtkAbstractArray* outArray)
{
  int* pieceExtent = this->PieceExtents + this->Piece*6;
  int* pieceCellDimensions = this->PieceCellDimensions + this->Piece*3;
  vtkIdType* pieceCellIncrements = this->PieceCellIncrements + this->Piece*3;
  if (!this->ReadSubExtent(pieceExtent, pieceCellDimensions,
                           pieceCellIncrements, this->UpdateExtent,
                           this->CellDimensions, this->CellIncrements,
                           this->SubExtent, this->SubCellDimensions,
                           da, outArray, CELL_DATA))
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
template <class iterT>
void vtkXMLStructuredDataReaderSubExtentCopyValues(
  iterT* destIter, vtkIdType destIndex,
  iterT* srcIter, vtkIdType srcIndex,
  vtkIdType numValues)
{
  // for all contiguous fixed-size arrays ie. vtkDataArray.
  memcpy(destIter->GetArray()->GetVoidPointer(destIndex),
    srcIter->GetArray()->GetVoidPointer(srcIndex),
    numValues);
}
//----------------------------------------------------------------------------
template<>
void vtkXMLStructuredDataReaderSubExtentCopyValues(
  vtkArrayIteratorTemplate<vtkStdString>* destIter, vtkIdType destIndex,
  vtkArrayIteratorTemplate<vtkStdString>* srcIter, vtkIdType srcIndex,
  vtkIdType numValues)
{
  vtkIdType maxIndex = destIndex + numValues;
  for (vtkIdType cc = destIndex; cc < maxIndex; ++cc)
  {
    destIter->GetValue(cc) =
      srcIter->GetValue(srcIndex++);
  }
}

//----------------------------------------------------------------------------
int vtkXMLStructuredDataReader::ReadSubExtent(
  int* inExtent, int* inDimensions, vtkIdType* inIncrements,
  int* outExtent, int* outDimensions, vtkIdType* outIncrements,
  int* subExtent, int* subDimensions, vtkXMLDataElement* da,
  vtkAbstractArray* array, FieldType fieldType)
{
  int components = array->GetNumberOfComponents();

  if ((inDimensions[0] == outDimensions[0]) &&
     (subDimensions[0] == outDimensions[0]) &&
     (inDimensions[1] == outDimensions[1]) &&
     (subDimensions[1] == outDimensions[1])
    )
  {
    if ((inDimensions[2] == outDimensions[2]) &&
        (subDimensions[2] == outDimensions[2])
      )
    {
      // Read the whole volume at once.  This fills the array's entire
      // progress range.
      vtkIdType volumeTuples =
        (static_cast<vtkIdType>(inDimensions[0])*
         static_cast<vtkIdType>(inDimensions[1])*
         static_cast<vtkIdType>(inDimensions[2]));

      vtkIdType sourceTuple =
        this->GetStartTuple(inExtent, inIncrements,
                            subExtent[0], subExtent[2], subExtent[4]);
      vtkIdType destTuple =
        this->GetStartTuple(outExtent, outIncrements,
                            subExtent[0], subExtent[2], subExtent[4]);

      if (!this->ReadArrayValues(
            da, destTuple*components, array,
            sourceTuple*components, volumeTuples*components, fieldType))
      {
        return 0;
      }
    }
    else
    {
      // Read an entire slice at a time.  Split progress range by
      // slice.
      float progressRange[2] = { 0.f, 0.f };
      this->GetProgressRange(progressRange);
      vtkIdType sliceTuples =
        static_cast<vtkIdType>(inDimensions[0])*
        static_cast<vtkIdType>(inDimensions[1]);

      for (int k = 0; k < subDimensions[2] && !this->AbortExecute; ++k)
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
        if (!this->ReadArrayValues(
              da, destTuple*components, array,
              sourceTuple*components, sliceTuples*components, fieldType))
        {
          return 0;
        }
      }
    }
  }
  else
  {
    if (!this->WholeSlices)
    {
      // Read a row at a time.  Split progress range by row.
      float progressRange[2] = { 0.f, 0.f };
      this->GetProgressRange(progressRange);
      vtkIdType rowTuples = subDimensions[0];
      for (int k = 0; k < subDimensions[2] && !this->AbortExecute; ++k)
      {
        for (int j = 0;j < subDimensions[1] && !this->AbortExecute; ++j)
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
          if (!this->ReadArrayValues(
                da, destTuple*components,
                array, sourceTuple*components,
                rowTuples*components, fieldType))
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
      float progressRange[2] = { 0.f, 0.f };
      this->GetProgressRange(progressRange);
      vtkIdType rowTuples = subDimensions[0];
      vtkIdType partialSliceTuples =
        static_cast<vtkIdType>(inDimensions[0]) *
        static_cast<vtkIdType>(subDimensions[1]);
      int tupleSize = components*array->GetDataTypeSize();
      vtkAbstractArray* temp = array->NewInstance();
      temp->SetNumberOfComponents(array->GetNumberOfComponents());
      temp->SetNumberOfTuples(partialSliceTuples);
      vtkArrayIterator* srcIter = temp->NewIterator();
      vtkArrayIterator* destIter = array->NewIterator();

      for (int k = 0; k < subDimensions[2] && !this->AbortExecute; ++k)
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
        if (!this->ReadArrayValues(
              da, 0, temp, inTuple*components,
              partialSliceTuples*components, fieldType))
        {
          temp->Delete();
          return 0;
        }
        // since arrays have changed, reinit the iters.
        destIter->Initialize(array);
        srcIter->Initialize(temp);
        // Copy the portion of the slice we need.
        for (int j = 0;j < subDimensions[1]; ++j)
        {
          vtkIdType sourceTuple =
            this->GetStartTuple(memExtent, inIncrements,
              subExtent[0], subExtent[2]+j, subExtent[4]+k);
          vtkIdType destTuple =
            this->GetStartTuple(outExtent, outIncrements,
              subExtent[0], subExtent[2]+j, subExtent[4]+k);

          switch (array->GetDataType())
          {
            vtkArrayIteratorTemplateMacro(
              vtkXMLStructuredDataReaderSubExtentCopyValues(
                static_cast<VTK_TT*>(destIter),
                destTuple*components,
                static_cast<VTK_TT*>(srcIter),
                sourceTuple*components, tupleSize*rowTuples));
          default:
            vtkErrorMacro("Array not supported : " << array->GetDataTypeAsString());
            break;
          }
        }
      }
      srcIter->Delete();
      destIter->Delete();
      temp->Delete();
    }
  }
  return 1;
}
