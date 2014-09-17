/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"


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
  return vtkPointSet::SafeDownCast( this->GetOutputDataObject(0) );
}

//----------------------------------------------------------------------------
vtkPointSet* vtkXMLPUnstructuredDataReader::GetPieceInputAsPointSet(int piece)
{
  vtkXMLDataReader* reader = this->PieceReaders[piece];
  if (!reader)
    {
    return 0;
    }
  if (reader->GetNumberOfOutputPorts() < 1)
    {
    return 0;
    }
  return static_cast<vtkPointSet*>(reader->GetExecutive()->GetOutputData(0));
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::SetupOutputTotals()
{
  this->TotalNumberOfPoints = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
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
  if (this->PieceReaders[this->Piece])
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
vtkIdType vtkXMLPUnstructuredDataReader::GetNumberOfPointsInPiece(int piece)
{
  if (this->PieceReaders[piece])
    {
    return this->PieceReaders[piece]->GetNumberOfPoints();
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPUnstructuredDataReader::GetNumberOfCellsInPiece(int piece)
{
  if (this->PieceReaders[piece])
    {
    return this->PieceReaders[piece]->GetNumberOfCells();
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void vtkXMLPUnstructuredDataReader::SetupOutputInformation(
  vtkInformation *outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::CopyOutputInformation(
  vtkInformation *outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  // Create the points array.
  vtkPoints* points = vtkPoints::New();
  if (this->PPointsElement)
    {
    vtkAbstractArray* aa = this->CreateArray(
      this->PPointsElement->GetNestedElement(0));
    vtkDataArray* a = vtkDataArray::SafeDownCast(aa);
    if (a)
      {
      a->SetNumberOfTuples(this->GetNumberOfPoints());
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
  vtkPointSet::SafeDownCast(this->GetCurrentOutput())->SetPoints(points);
  points->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::SetupUpdateExtent(
  int piece, int numberOfPieces, int ghostLevel)
{
  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numberOfPieces;
  this->UpdateGhostLevel = ghostLevel;

  // If more pieces are requested than available, just return empty
  // pieces for the extra ones.
  if (this->UpdateNumberOfPieces > this->NumberOfPieces)
    {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
    }

  // Find the range of pieces to read.
  if (this->UpdatePiece < this->UpdateNumberOfPieces)
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
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
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
int vtkXMLPUnstructuredDataReader::ReadPrimaryElement(vtkXMLDataElement* ePri)
{
  if(!this->Superclass::ReadPrimaryElement(ePri)) { return 0; }

  // Find the PPoints element.
  this->PPointsElement = 0;
  int numNested = ePri->GetNumberOfNestedElements();
  for (int i = 0;i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePri->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "PPoints") == 0) &&
      (eNested->GetNumberOfNestedElements() == 1))
      {
      this->PPointsElement = eNested;
      }
    }

  // If PPoints element was not found, we must assume there are 0
  // points.  If there are found to be points later, the error will be
  // reported by ReadPieceData.

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::ReadXMLData()
{
  // Get the update request.
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  int piece = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numberOfPieces = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int ghostLevel = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  vtkDebugMacro("Updating piece " << piece << " of " << numberOfPieces
                << " with ghost level " << ghostLevel);

  // Setup the range of pieces that will be read.
  this->SetupUpdateExtent(piece, numberOfPieces, ghostLevel);

  // If there are no data to read, stop now.
  if (this->StartPiece == this->EndPiece)
    {
    return;
    }

  vtkDebugMacro("Reading piece range [" << this->StartPiece
                << ", " << this->EndPiece << ") from file.");

  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();

  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  float* fractions = new float[this->EndPiece-this->StartPiece+1];
  fractions[0] = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
    {
    int index = i - this->StartPiece;
    fractions[index+1] = (fractions[index] +
                          this->GetNumberOfPointsInPiece(i) +
                          this->GetNumberOfCellsInPiece(i));
    }
  if (fractions[this->EndPiece-this->StartPiece] == 0)
    {
    fractions[this->EndPiece-this->StartPiece] = 1;
    }
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
    {
    int index = i-this->StartPiece;
    fractions[index+1] = fractions[index+1] /
      fractions[this->EndPiece-this->StartPiece];
    }

  // Read the data needed from each piece.
  for(int i = this->StartPiece;
    (i < this->EndPiece && !this->AbortExecute && !this->DataError); ++i)
    {
    // Set the range of progress for this piece.
    this->SetProgressRange(progressRange, i - this->StartPiece, fractions);

    if (!this->Superclass::ReadPieceData(i))
      {
      // An error occurred while reading the piece.
      this->DataError = 1;
      }
    this->SetupNextPiece();
    }

  delete [] fractions;
}

//----------------------------------------------------------------------------
int vtkXMLPUnstructuredDataReader::ReadPieceData()
{
  // Use the internal reader to read the piece.
  vtkStreamingDemandDrivenPipeline::SetUpdateExtent(
    this->PieceReaders[this->Piece]->GetOutputInformation(0),
    0, 1, this->UpdateGhostLevel);
  this->PieceReaders[this->Piece]->Update();

  vtkPointSet* input = this->GetPieceInputAsPointSet(this->Piece);
  vtkPointSet* output = vtkPointSet::SafeDownCast(this->GetCurrentOutput());

  // If there are some points, but no PPoints element, report the
  // error.
  if (!this->PPointsElement && (this->GetNumberOfPoints() > 0))
    {
    vtkErrorMacro("Could not find PPoints element with 1 array.");
    return 0;
    }

  if (!input->GetPoints())
    {
    return 0;
    }

  // Copy the points array.
  this->CopyArrayForPoints(
    input->GetPoints()->GetData(), output->GetPoints()->GetData());

  // Let the superclass read the data it wants.
  return this->Superclass::ReadPieceData();
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataReader::CopyArrayForPoints(
  vtkDataArray* inArray, vtkDataArray* outArray)
{
  if (!this->PieceReaders[this->Piece])
    {
    return;
    }
  if (!inArray || !outArray)
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
void vtkXMLPUnstructuredDataReader::CopyCellArray(
  vtkIdType totalNumberOfCells, vtkCellArray* inCells, vtkCellArray* outCells)
{
  // Allocate memory in the output connectivity array.
  vtkIdType curSize = 0;
  if (outCells->GetData())
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
  while (in < end)
    {
    vtkIdType length = *in++;
    *out++ = length;
    // Copy the point indices, but increment them for the appended
    // version's index.
    for (vtkIdType j = 0; j < length; ++j)
      {
      out[j] = in[j] + this->StartPoint;
      }
    in += length;
    out += length;
    }
}

//----------------------------------------------------------------------------
int vtkXMLPUnstructuredDataReader::RequestInformation(
   vtkInformation *request,
   vtkInformationVector **inputVector,
   vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}
