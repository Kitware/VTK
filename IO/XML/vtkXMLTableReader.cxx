/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLTableReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLTableReader.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLTableReader);

//----------------------------------------------------------------------------
vtkXMLTableReader::vtkXMLTableReader()
{
  this->NumberOfPieces = 0;
  this->RowDataElements = nullptr;
  this->RowElements = nullptr;
  this->NumberOfRows = nullptr;
  this->TotalNumberOfRows = 0;
}

//----------------------------------------------------------------------------
vtkXMLTableReader::~vtkXMLTableReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkTable* vtkXMLTableReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkTable* vtkXMLTableReader::GetOutput(int idx)
{
  return vtkTable::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLTableReader::GetDataSetName()
{
  return "Table";
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::GetOutputUpdateExtent(int& piece, int& numberOfPieces)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::SetupOutputTotals()
{
  this->TotalNumberOfRows = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    this->TotalNumberOfRows += this->NumberOfRows[i];
  }
  this->StartPoint = 0;
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::SetupNextPiece()
{
  this->StartPoint += this->NumberOfRows[this->Piece];
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::SetupUpdateExtent(int piece, int numberOfPieces)
{
  this->UpdatedPiece = piece;
  this->UpdateNumberOfPieces = numberOfPieces;

  // If more pieces are requested than available, just return empty
  // pieces for the extra ones.
  if (this->UpdateNumberOfPieces > this->NumberOfPieces)
  {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
  }

  // Find the range of pieces to read.
  if (this->UpdatedPiece < this->UpdateNumberOfPieces)
  {
    this->StartPiece = (this->UpdatedPiece * this->NumberOfPieces) / this->UpdateNumberOfPieces;
    this->EndPiece = ((this->UpdatedPiece + 1) * this->NumberOfPieces) / this->UpdateNumberOfPieces;
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
void vtkXMLTableReader::ReadXMLData()
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

  this->ReadFieldData();

  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);

  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  std::vector<float> fractions(this->EndPiece - this->StartPiece + 1);
  fractions[0] = 0;

  for (int currentIndex = this->StartPiece; currentIndex < this->EndPiece; ++currentIndex)
  {
    int index = currentIndex - this->StartPiece;
    fractions[index + 1] = fractions[index];
  }
  if (fractions[this->EndPiece - this->StartPiece] == 0)
  {
    fractions[this->EndPiece - this->StartPiece] = 1;
  }
  for (int currentIndex = this->StartPiece; currentIndex < this->EndPiece; ++currentIndex)
  {
    int index = currentIndex - this->StartPiece;
    fractions[index + 1] = fractions[index + 1] / fractions[this->EndPiece - this->StartPiece];
  }

  // Read the data needed from each piece.
  for (int currentIndex = this->StartPiece;
       (currentIndex < this->EndPiece && !this->AbortExecute && !this->DataError); ++currentIndex)
  {
    // Set the range of progress for this piece.
    this->SetProgressRange(progressRange, currentIndex - this->StartPiece, fractions.data());

    if (!this->ReadPieceData(currentIndex))
    {
      // An error occurred while reading the piece.
      this->DataError = 1;
    }
    this->SetupNextPiece();
  }
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::SetupPieces(int numPieces)
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  this->NumberOfPieces = numPieces;
  if (numPieces > 0)
  {
    this->RowDataElements = new vtkXMLDataElement*[numPieces];
  }
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    this->RowDataElements[i] = nullptr;
  }

  this->NumberOfRows = new vtkIdType[numPieces];
  this->RowElements = new vtkXMLDataElement*[numPieces];
  for (int i = 0; i < numPieces; ++i)
  {
    this->RowElements[i] = nullptr;
    this->NumberOfRows[i] = 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::DestroyPieces()
{
  delete[] this->RowElements;
  delete[] this->NumberOfRows;
  this->RowElements = nullptr;
  this->NumberOfRows = nullptr;

  delete[] this->RowDataElements;
  this->RowDataElements = nullptr;
  this->NumberOfPieces = 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLTableReader::GetNumberOfRows()
{
  return this->TotalNumberOfRows;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLTableReader::GetNumberOfPieces()
{
  return this->NumberOfPieces;
}

//----------------------------------------------------------------------------
int vtkXMLTableReader::ColumnIsEnabled(vtkXMLDataElement* eRowData)
{
  const char* name = eRowData->GetAttribute("Name");
  return (name && this->ColumnArraySelection->ArrayIsEnabled(name));
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::SetupOutputInformation(vtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  if (this->InformationError)
  {
    vtkErrorMacro("Should not still be processing output information if have set InformationError");
    return;
  }

  // Initialize DataArraySelections to enable all that are present
  this->SetDataArraySelections(this->RowDataElements[0], this->ColumnArraySelection);

  // Setup the Field Information for RowData.  We only need the
  // information from one piece because all pieces have the same set of arrays.
  vtkInformationVector* infoVector = nullptr;
  if (!this->SetFieldDataInfo(this->RowDataElements[0], vtkDataObject::FIELD_ASSOCIATION_ROWS,
        this->GetNumberOfRows(), infoVector))
  {
    return;
  }
  if (infoVector)
  {
    infoVector->Delete();
  }

  if (this->NumberOfPieces > 1)
  {
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }
}

//----------------------------------------------------------------------------
int vtkXMLTableReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Count the number of pieces in the file.
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numPieces = 0;

  for (int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Piece") == 0)
    {
      ++numPieces;
    }
  }

  // Now read each piece.  If no "Piece" elements were found, assume
  // the primary element itself is a single piece.
  if (numPieces)
  {
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
  }
  else
  {
    this->SetupPieces(1);
    if (!this->ReadPiece(ePrimary, 0))
    {
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
}

//----------------------------------------------------------------------------
void vtkXMLTableReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  vtkTable* output = vtkTable::SafeDownCast(this->GetCurrentOutput());
  vtkDataSetAttributes* rowData = output->GetRowData();

  // Get the size of the output arrays.
  vtkIdType rowTuples = this->GetNumberOfRows();

  // Allocate the arrays in the output.  We only need the information
  // from one piece because all pieces have the same set of arrays.
  vtkXMLDataElement* eRowData = this->RowDataElements[0];
  this->NumberOfColumns = 0;
  this->RowDataTimeStep.clear();
  this->RowDataOffset.clear();
  if (eRowData)
  {
    for (int i = 0; i < eRowData->GetNumberOfNestedElements(); i++)
    {
      vtkXMLDataElement* eNested = eRowData->GetNestedElement(i);
      const char* ename = eNested->GetAttribute("Name");
      if (this->ColumnIsEnabled(eNested) && !rowData->HasArray(ename))
      {
        this->NumberOfColumns++;
        this->RowDataTimeStep[ename] = -1;
        this->RowDataOffset[ename] = -1;
        vtkAbstractArray* array = this->CreateArray(eNested);
        if (array)
        {
          array->SetNumberOfTuples(rowTuples);
          // Manipulate directly RowData may have unexpected results
          // passing by AddColumn() instead of AddArray()
          output->AddColumn(array);
          array->Delete();
        }
        else
        {
          this->DataError = 1;
        }
      }
    }
  }

  // Setup attribute indices for the row data and cell data.
  this->ReadAttributeIndices(eRowData, rowData);
}

//----------------------------------------------------------------------------
int vtkXMLTableReader::ReadPiece(vtkXMLDataElement* ePiece, int piece)
{
  this->Piece = piece;
  return this->ReadPiece(ePiece);
}

//----------------------------------------------------------------------------
int vtkXMLTableReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  // Find the RowData in the piece.
  for (int i = 0; i < ePiece->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "RowData") == 0)
    {
      this->RowDataElements[this->Piece] = eNested;
    }
  }

  if (!this->RowDataElements[this->Piece])
  {
    return 0;
  }

  if (!ePiece->GetScalarAttribute("NumberOfRows", this->NumberOfRows[this->Piece]))
  {
    vtkErrorMacro("Piece " << this->Piece << " is missing its NumberOfRows attribute.");
    this->NumberOfRows[this->Piece] = 0;
    return 0;
  }

  // Find the Rows element in the piece.
  this->RowElements[this->Piece] = nullptr;
  for (int nestedIndex = 0; nestedIndex < ePiece->GetNumberOfNestedElements(); ++nestedIndex)
  {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(nestedIndex);
    if (strcmp(eNested->GetName(), "RowData") == 0)
    {
      this->RowElements[this->Piece] = eNested;
    }
  }

  if (!this->RowElements[this->Piece] && (this->NumberOfRows[this->Piece] > 0))
  {
    vtkErrorMacro("A piece has rows but is missing its RowData element.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLTableReader::ReadPieceData(int piece)
{
  this->Piece = piece;

  // Total amount of data in this piece comes from row data
  // arrays and the point specifications themselves.
  vtkIdType totalPieceSize = this->NumberOfRows[this->Piece];
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);

  // Set the range of progress.
  this->SetProgressRange(progressRange, 0, 2);

  // Let the superclass read its data.
  vtkTable* output = vtkTable::SafeDownCast(this->GetCurrentOutput());

  vtkXMLDataElement* eRowData = this->RowDataElements[this->Piece];

  // Split current progress range over number of arrays.  This assumes
  // that each array contributes approximately the same amount of data
  // within this piece.
  int currentArray = 0;
  int numArrays = this->NumberOfColumns;
  this->GetProgressRange(progressRange);

  // Read the data for this piece from each array.
  if (eRowData)
  {
    int currentArrayIndex = 0;
    for (int i = 0; (i < eRowData->GetNumberOfNestedElements() && !this->AbortExecute); ++i)
    {
      vtkXMLDataElement* eNested = eRowData->GetNestedElement(i);
      if (this->ColumnIsEnabled(eNested))
      {
        if (strcmp(eNested->GetName(), "DataArray") != 0 &&
          strcmp(eNested->GetName(), "Array") != 0)
        {
          vtkErrorMacro("Invalid Array.");
          this->DataError = 1;
          return 0;
        }

        if (this->RowDataNeedToReadTimeStep(eNested))
        {
          // Set the range of progress for this array.
          this->SetProgressRange(progressRange, currentArray++, numArrays);

          // Read the array.
          vtkAbstractArray* array = output->GetRowData()->GetAbstractArray(currentArrayIndex);
          vtkIdType components = array->GetNumberOfComponents();
          vtkIdType numberOfTuples = this->NumberOfRows[this->Piece];

          if (array && !this->ReadArrayValues(eNested, 0, array, 0, numberOfTuples * components))
          {
            if (!this->AbortExecute)
            {
              vtkErrorMacro("Cannot read row data array \""
                << array->GetName() << "\" from " << eRowData->GetName() << " in piece "
                << this->Piece << ".  The data array in the element may be too short.");
            }
            return 0;
          }
          currentArrayIndex++;
        }
      }
    }
  }

  return this->AbortExecute ? 0 : 1;
}

//----------------------------------------------------------------------------
int vtkXMLTableReader::RowDataNeedToReadTimeStep(vtkXMLDataElement* eNested)
{
  // First thing need to find the id of this dataarray from its name:
  const char* name = eNested->GetAttribute("Name");

  // Easy case no timestep:
  int numTimeSteps =
    eNested->GetVectorAttribute("TimeStep", this->NumberOfTimeSteps, this->TimeSteps);
  if (!(numTimeSteps <= this->NumberOfTimeSteps))
  {
    vtkErrorMacro("Invalid TimeStep specification");
    this->DataError = 1;
    return 0;
  }
  if (!numTimeSteps && !this->NumberOfTimeSteps)
  {
    assert(this->RowDataTimeStep.at(name) == -1); // No timestep in this file
    return 1;
  }
  // else TimeStep was specified but no TimeValues associated were found
  assert(this->NumberOfTimeSteps);

  // case numTimeSteps > 1
  int isCurrentTimeInArray =
    vtkXMLReader::IsTimeStepInArray(this->CurrentTimeStep, this->TimeSteps, numTimeSteps);
  if (numTimeSteps && !isCurrentTimeInArray)
  {
    return 0;
  }
  // we know that time steps are specified and that CurrentTimeStep is in the array
  // we need to figure out if we need to read the array or if it was forwarded
  // Need to check the current 'offset'
  vtkTypeInt64 offset;
  if (eNested->GetScalarAttribute("offset", offset))
  {
    if (this->RowDataOffset.at(name) != offset)
    {
      // save the pointsOffset
      assert(this->RowDataTimeStep.at(name) == -1); // cannot have mixture of binary and appended
      this->RowDataOffset.at(name) = offset;
      return 1;
    }
  }
  else
  {
    // No offset is specified this is a binary file
    // First thing to check if numTimeSteps == 0:
    if (!numTimeSteps && this->NumberOfTimeSteps && this->RowDataTimeStep.at(name) == -1)
    {
      // Update last PointsTimeStep read
      this->RowDataTimeStep.at(name) = this->CurrentTimeStep;
      return 1;
    }
    int isLastTimeInArray = vtkXMLReader::IsTimeStepInArray(
      this->RowDataTimeStep.at(name), this->TimeSteps, numTimeSteps);
    // If no time is specified or if time is specified and match then read
    if (isCurrentTimeInArray && !isLastTimeInArray)
    {
      // CurrentTimeStep is in TimeSteps but Last is not := need to read
      // Update last PointsTimeStep read
      this->RowDataTimeStep.at(name) = this->CurrentTimeStep;
      return 1;
    }
  }
  // all other cases we don't need to read:
  return 0;
}

//----------------------------------------------------------------------------
int vtkXMLTableReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
  return 1;
}
