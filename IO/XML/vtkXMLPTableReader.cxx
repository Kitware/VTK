/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPTableReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPTableReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTableReader.h"

#include <cassert>
#include <sstream>

vtkStandardNewMacro(vtkXMLPTableReader);

//----------------------------------------------------------------------------
vtkXMLPTableReader::vtkXMLPTableReader()
{
  this->PieceReaders = nullptr;

  this->TotalNumberOfRows = 0;

  this->ColumnSelection = vtkDataArraySelection::New();
  this->ColumnSelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
}

//----------------------------------------------------------------------------
vtkXMLPTableReader::~vtkXMLPTableReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }

  this->ColumnSelection->RemoveObserver(this->SelectionObserver);
  this->ColumnSelection->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  vtkInformation* localInfo = this->GetExecutive()->GetOutputInformation(port);

  if (localInfo->Has(CAN_HANDLE_PIECE_REQUEST()))
  {
    outInfo->CopyEntry(localInfo, CAN_HANDLE_PIECE_REQUEST());
  }
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Column Selection: " << this->ColumnSelection << "\n";
  os << indent << "Total Number Of Rows: " << this->TotalNumberOfRows << "\n";
}

//----------------------------------------------------------------------------
vtkTable* vtkXMLPTableReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkTable* vtkXMLPTableReader::GetOutput(int idx)
{
  return vtkTable::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLPTableReader::GetDataSetName()
{
  return "PTable";
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::GetOutputUpdateExtent(int& piece, int& numberOfPieces)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::SetupOutputTotals()
{
  this->TotalNumberOfRows = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->TotalNumberOfRows += this->PieceReaders[i]->GetNumberOfRows();
    }
  }
  this->StartRow = 0;
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  // Setup the output arrays.
  vtkTable* output = vtkTable::SafeDownCast(this->GetCurrentOutput());
  vtkDataSetAttributes* rowData = output->GetRowData();

  // Get the size of the output arrays.
  unsigned long rowTuples = this->GetNumberOfRows();

  // Allocate data in the arrays.
  if (this->PRowElement)
  {
    for (int i = 0; i < this->PRowElement->GetNumberOfNestedElements(); ++i)
    {
      vtkXMLDataElement* eNested = this->PRowElement->GetNestedElement(i);
      if (this->ColumnIsEnabled(eNested))
      {
        vtkAbstractArray* array = this->CreateArray(eNested);
        if (array)
        {
          array->SetNumberOfTuples(rowTuples);
          rowData->AddArray(array);
          array->Delete();
        }
        else
        {
          this->DataError = 1;
        }
      }
    }
  }

  // Setup attribute indices for the point data and cell data.
  this->ReadAttributeIndices(this->PRowElement, rowData);
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::ReadPieceData(int index)
{
  this->Piece = index;

  // We need data, make sure the piece can be read.
  if (!this->CanReadPiece(this->Piece))
  {
    vtkErrorMacro("File for piece " << this->Piece << " cannot be read.");
    return 0;
  }

  // Actually read the data.
  this->PieceReaders[this->Piece]->SetAbortExecute(0);

  return this->ReadPieceData();
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::CanReadPiece(int index)
{
  // If necessary, test whether the piece can be read.
  vtkXMLTableReader* reader = this->PieceReaders[index];
  if (reader && !this->CanReadPieceFlag[index])
  {
    if (reader->CanReadFile(reader->GetFileName()))
    {
      // We can read the piece.  Save result to avoid later repeat of
      // test.
      this->CanReadPieceFlag[index] = 1;
    }
    else
    {
      // We cannot read the piece.  Destroy the reader to avoid later
      // repeat of test.
      this->PieceReaders[index] = nullptr;
      reader->Delete();
    }
  }

  return (this->PieceReaders[index] ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::PieceProgressCallback()
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  float pieceProgress = this->PieceReaders[this->Piece]->GetProgress();
  float progress = this->ProgressRange[0] + pieceProgress * width;
  this->UpdateProgressDiscrete(progress);
  if (this->AbortExecute)
  {
    this->PieceReaders[this->Piece]->SetAbortExecute(1);
  }
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::SetupNextPiece()
{
  if (this->PieceReaders[this->Piece])
  {
    this->StartRow += this->PieceReaders[this->Piece]->GetNumberOfRows();
  }
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::ReadPieceData()
{
  // Use the internal reader to read the piece.
  this->PieceReaders[this->Piece]->UpdatePiece(0, 1, 0);

  vtkTable* input = this->GetPieceInputAsTable(this->Piece);

  if (!input)
  {
    vtkErrorMacro("No input piece found for the current piece index.");
    return 0;
  }

  vtkTable* output = vtkTable::SafeDownCast(this->GetCurrentOutput());

  // If there are some rows, but no PRows element, report the
  // error.
  if (!this->PRowElement && (this->GetNumberOfRows() > 0))
  {
    vtkErrorMacro("Could not find PRows element with 1 array.");
    return 0;
  }

  if (!input->GetRowData())
  {
    return 0;
  }

  // copy any row data
  if (input->GetRowData())
  {
    for (int i = 0; i < input->GetRowData()->GetNumberOfArrays(); i++)
    {
      if (this->ColumnSelection->ArrayIsEnabled(input->GetRowData()->GetArrayName(i)))
      {
        output->GetRowData()->AddArray(input->GetRowData()->GetArray(i));
      }
    }
  }

  // copy any field data
  if (input->GetFieldData())
  {
    for (int i = 0; i < input->GetFieldData()->GetNumberOfArrays(); i++)
    {
      output->GetFieldData()->AddArray(input->GetFieldData()->GetArray(i));
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkXMLTableReader* vtkXMLPTableReader::CreatePieceReader()
{
  return vtkXMLTableReader::New();
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
vtkTable* vtkXMLPTableReader::GetOutputAsTable()
{
  return vtkTable::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
vtkTable* vtkXMLPTableReader::GetPieceInputAsTable(int piece)
{
  vtkXMLTableReader* reader = this->PieceReaders[piece];
  if (!reader || reader->GetNumberOfOutputPorts() < 1)
  {
    return nullptr;
  }
  return static_cast<vtkTable*>(reader->GetExecutive()->GetOutputData(0));
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPTableReader::GetNumberOfRows()
{
  return this->TotalNumberOfRows;
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::SetupOutputInformation(vtkInformation* outInfo)
{
  if (this->InformationError)
  {
    vtkErrorMacro("Should not still be processing output information if have set InformationError");
    return;
  };

  // Initialize DataArraySelections to enable all that are present
  this->SetDataArraySelections(this->PRowElement, this->ColumnSelection);

  // Setup the Field Information for RowData.  We only need the
  // information from one piece because all pieces have the same set of arrays.
  vtkInformationVector* infoVector = nullptr;
  if (!this->SetFieldDataInfo(this->PRowElement, vtkDataObject::FIELD_ASSOCIATION_ROWS,
        this->GetNumberOfRows(), infoVector))
  {
    return;
  }
  if (infoVector)
  {
    infoVector->Delete();
  }

  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::ReadXMLData()
{
  // Get the update request.
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  vtkDebugMacro("Updating piece " << piece << " of " << numberOfPieces);

  // Setup the range of pieces that will be read.
  this->SetupUpdateExtent(piece, numberOfPieces);

  // If there are no data to read, stop now.
  if (this->StartPiece == this->EndPiece)
  {
    return;
  }

  vtkDebugMacro(
    "Reading piece range [" << this->StartPiece << ", " << this->EndPiece << ") from file.");

  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();

  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  std::vector<float> fractions(this->EndPiece - this->StartPiece + 1);
  fractions[0] = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    int index = i - this->StartPiece;
    fractions[index + 1] = (fractions[index] + this->GetNumberOfRowsInPiece(i));
  }
  if (fractions[this->EndPiece - this->StartPiece] == 0)
  {
    fractions[this->EndPiece - this->StartPiece] = 1;
  }
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    int index = i - this->StartPiece;
    fractions[index + 1] = fractions[index + 1] / fractions[this->EndPiece - this->StartPiece];
  }

  // Read the data needed from each piece.
  for (int i = this->StartPiece; (i < this->EndPiece && !this->AbortExecute && !this->DataError);
       ++i)
  {
    // Set the range of progress for this piece.
    this->SetProgressRange(progressRange, i - this->StartPiece, fractions.data());

    if (!this->ReadPieceData(i))
    {
      // An error occurred while reading the piece.
      this->DataError = 1;
    }
    this->SetupNextPiece();
  }
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{

  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Read information about the pieces.
  this->PRowElement = nullptr;
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numPieces = 0;
  for (int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Piece") == 0)
    {
      ++numPieces;
    }
    else if (strcmp(eNested->GetName(), "PRowData") == 0)
    {
      this->PRowElement = eNested;
    }
  }
  this->SetupPieces(numPieces);
  int piece = 0;
  for (int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Piece") == 0)
    {
      if (!this->ReadPiece(eNested, piece++))
      {
        return 0;
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::SetupUpdateExtent(int piece, int numberOfPieces)
{
  this->UpdatePieceId = piece;
  this->UpdateNumberOfPieces = numberOfPieces;

  // If more pieces are requested than available, just return empty
  // pieces for the extra ones
  if (this->UpdateNumberOfPieces > this->NumberOfPieces)
  {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
  }

  // Find the range of pieces to read.
  if (this->UpdatePieceId < this->UpdateNumberOfPieces)
  {
    this->StartPiece = (this->UpdatePieceId * this->NumberOfPieces) / this->UpdateNumberOfPieces;
    this->EndPiece =
      ((this->UpdatePieceId + 1) * this->NumberOfPieces) / this->UpdateNumberOfPieces;
  }
  else
  {
    this->StartPiece = 0;
    this->EndPiece = 0;
  }

  // Update the information of the pieces we need.
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    if (this->CanReadPiece(i))
    {
      this->PieceReaders[i]->UpdateInformation();
      vtkXMLTableReader* pReader = this->PieceReaders[i];
      pReader->SetupUpdateExtent(0, 1);
    }
  }

  // Find the total size of the output.
  this->SetupOutputTotals();
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPTableReader::GetNumberOfRowsInPiece(int piece)
{
  return this->PieceReaders[piece] ? this->PieceReaders[piece]->GetNumberOfRows() : 0;
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);

  this->PieceReaders = new vtkXMLTableReader*[this->NumberOfPieces];

  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    this->PieceReaders[i] = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::DestroyPieces()
{
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->PieceReaders[i]->RemoveObserver(this->PieceProgressObserver);
      this->PieceReaders[i]->Delete();
    }
  }

  delete[] this->PieceReaders;
  this->PieceReaders = nullptr;

  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  this->PieceElements[this->Piece] = ePiece;

  const char* fileName = ePiece->GetAttribute("Source");
  if (!fileName)
  {
    vtkErrorMacro("Piece " << this->Piece << " has no Source attribute.");
    return 0;
  }

  // The file name is relative to the summary file.  Convert it to
  // something we can use.
  char* pieceFileName = this->CreatePieceFileName(fileName);

  vtkXMLTableReader* reader = this->CreatePieceReader();
  this->PieceReaders[this->Piece] = reader;
  this->PieceReaders[this->Piece]->AddObserver(
    vtkCommand::ProgressEvent, this->PieceProgressObserver);
  reader->SetFileName(pieceFileName);

  delete[] pieceFileName;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::ColumnIsEnabled(vtkXMLDataElement* elementRowData)
{
  const char* name = elementRowData->GetAttribute("Name");
  return (name && this->ColumnSelection->ArrayIsEnabled(name));
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::GetNumberOfColumnArrays()
{
  return this->ColumnSelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkXMLPTableReader::GetColumnArrayName(int index)
{
  return this->ColumnSelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::GetColumnArrayStatus(const char* name)
{
  return this->ColumnSelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::SetColumnArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->ColumnSelection->EnableArray(name);
  }
  else
  {
    this->ColumnSelection->DisableArray(name);
  }
}
