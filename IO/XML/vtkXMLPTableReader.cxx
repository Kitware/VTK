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
#include "vtkXMLDataElement.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkXMLTableReader.h"

#include <cassert>
#include <sstream>

vtkStandardNewMacro(vtkXMLPTableReader);

//----------------------------------------------------------------------------
vtkXMLPTableReader::vtkXMLPTableReader()
{
  this->NumberOfPieces = 0;

  this->PieceElements = nullptr;
  this->PieceReaders = nullptr;
  this->CanReadPieceFlag = nullptr;

  this->PathName = nullptr;

  this->TotalNumberOfRows = 0;

  // Setup a callback for the internal serial readers to report
  // progress.
  this->PieceProgressObserver = vtkCallbackCommand::New();
  this->PieceProgressObserver->SetCallback(&vtkXMLPTableReader::PieceProgressCallbackFunction);
  this->PieceProgressObserver->SetClientData(this);

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
  delete[] this->PathName;
  this->PieceProgressObserver->Delete();

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

  os << indent << "ColumnSelection: " << this->ColumnSelection << "\n";
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
  vtkDataArraySelection* pds = this->PieceReaders[this->Piece]->GetPointDataArraySelection();
  vtkDataArraySelection* cds = this->PieceReaders[this->Piece]->GetCellDataArraySelection();
  pds->CopySelections(this->PointDataArraySelection);
  cds->CopySelections(this->CellDataArraySelection);
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
char* vtkXMLPTableReader::CreatePieceFileName(const char* fileName)
{
  assert(fileName);

  std::ostringstream fn_with_warning_C4701;

  // only prepend the path if the given file name is not
  // absolute (i.e. doesn't start with '/')
  if (this->PathName && fileName[0] != '/')
  {
    fn_with_warning_C4701 << this->PathName;
  }
  fn_with_warning_C4701 << fileName;

  size_t len = fn_with_warning_C4701.str().length();
  char* buffer = new char[len + 1];
  strncpy(buffer, fn_with_warning_C4701.str().c_str(), len);
  buffer[len] = '\0';

  return buffer;
}
//----------------------------------------------------------------------------
void vtkXMLPTableReader::SplitFileName()
{
  if (!this->FileName)
  {
    vtkErrorMacro(<< "Need to specify a filename");
    return;
  }

  // Pull the PathName component out of the FileName.
  size_t length = strlen(this->FileName);
  char* fileName = new char[length + 1];
  strcpy(fileName, this->FileName);
  char* begin = fileName;
  char* end = fileName + length;
  char* s;

#if defined(_WIN32)
  // Convert to UNIX-style slashes.
  for (s = begin; s != end; ++s)
  {
    if (*s == '\\')
    {
      *s = '/';
    }
  }
#endif

  // Extract the path name up to the last '/'.
  delete[] this->PathName;
  this->PathName = nullptr;
  char* rbegin = end - 1;
  char* rend = begin - 1;
  for (s = rbegin; s != rend; --s)
  {
    if (*s == '/')
    {
      break;
    }
  }
  if (s >= begin)
  {
    length = (s - begin) + 1;
    this->PathName = new char[length + 1];
    strncpy(this->PathName, this->FileName, length);
    this->PathName[length] = '\0';
  }

  // Cleanup temporary name.
  delete[] fileName;
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::PieceProgressCallbackFunction(
  vtkObject*, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<vtkXMLPTableReader*>(clientdata)->PieceProgressCallback();
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
  vtkTable* output = vtkTable::SafeDownCast(this->GetCurrentOutput());

  // If there are some points, but no PPoints element, report the
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
    int i;
    for (i = 0; i < input->GetRowData()->GetNumberOfArrays(); i++)
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
    int i;
    for (i = 0; i < input->GetFieldData()->GetNumberOfArrays(); i++)
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
  }

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
  int i;
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numPieces = 0;
  for (i = 0; i < numNested; ++i)
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
  for (i = 0; i < numNested; ++i)
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

void vtkXMLPTableReader::SetupUpdateExtent(int piece, int numberOfPieces)
{
  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numberOfPieces;

  // If more pieces are requested than available, just return empty
  // pieces for the extra ones.s
  if (this->UpdateNumberOfPieces > this->NumberOfPieces)
  {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
  }

  // Find the range of pieces to read.
  if (this->UpdatePiece < this->UpdateNumberOfPieces)
  {
    this->StartPiece = ((this->UpdatePiece * this->NumberOfPieces) / this->UpdateNumberOfPieces);
    this->EndPiece =
      (((this->UpdatePiece + 1) * this->NumberOfPieces) / this->UpdateNumberOfPieces);
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
int vtkXMLPTableReader::ReadXMLInformation()
{
  // First setup the filename components.
  this->SplitFileName();

  // Now proceed with reading the information.
  return this->Superclass::ReadXMLInformation();
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::SetupPieces(int numPieces)
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  this->NumberOfPieces = numPieces;
  this->PieceElements = new vtkXMLDataElement*[this->NumberOfPieces];
  this->PieceReaders = new vtkXMLTableReader*[this->NumberOfPieces];
  this->CanReadPieceFlag = new int[this->NumberOfPieces];
  int i;
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    this->PieceElements[i] = nullptr;
    this->PieceReaders[i] = nullptr;
    this->CanReadPieceFlag[i] = 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPTableReader::DestroyPieces()
{
  int i;
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->PieceReaders[i]->RemoveObserver(this->PieceProgressObserver);
      this->PieceReaders[i]->Delete();
    }
  }
  delete[] this->PieceElements;
  delete[] this->CanReadPieceFlag;
  delete[] this->PieceReaders;
  this->PieceElements = nullptr;
  this->PieceReaders = nullptr;
  this->NumberOfPieces = 0;
}

//----------------------------------------------------------------------------
int vtkXMLPTableReader::ReadPiece(vtkXMLDataElement* ePiece, int index)
{
  this->Piece = index;
  return this->ReadPiece(ePiece);
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
