/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLTableWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataArray.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLTableWriter.h"

vtkStandardNewMacro(vtkXMLTableWriter);

//----------------------------------------------------------------------------
vtkXMLTableWriter::vtkXMLTableWriter()
{
  this->NumberOfPieces = 1;
  this->WritePiece = -1;
  this->CurrentPiece = 0;

  this->FieldDataOM->Allocate(0);
  this->RowsOM = new OffsetsManagerArray;
}

//----------------------------------------------------------------------------
vtkXMLTableWriter::~vtkXMLTableWriter()
{
  delete this->RowsOM;
}

//----------------------------------------------------------------------------
int vtkXMLTableWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
  os << indent << "WritePiece: " << this->WritePiece << "\n";
}

//----------------------------------------------------------------------------
vtkTable* vtkXMLTableWriter::GetInputAsTable()
{
  return static_cast<vtkTable*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* vtkXMLTableWriter::GetDataSetName()
{
  return "Table";
}

//----------------------------------------------------------------------------
const char* vtkXMLTableWriter::GetDefaultFileExtension()
{
  return "vtt";
}

//----------------------------------------------------------------------------
vtkTypeBool vtkXMLTableWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    if ((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      this->SetInputUpdateExtent(this->CurrentPiece, this->NumberOfPieces);
    }
    else
    {
      this->SetInputUpdateExtent(this->WritePiece, this->NumberOfPieces);
    }
    return 1;
  }

  // generate the data
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->SetErrorCode(vtkErrorCode::NoError);

    if (!this->Stream && !this->FileName && !this->WriteToOutputString)
    {
      this->SetErrorCode(vtkErrorCode::NoFileNameError);
      vtkErrorMacro("The FileName or Stream must be set first or "
                    "the output must be written to a string.");
      return 0;
    }

    int numPieces = this->NumberOfPieces;

    if (this->WritePiece >= 0)
    {
      this->CurrentPiece = this->WritePiece;
    }
    else
    {
      float wholeProgressRange[2] = { 0, 1 };
      this->SetProgressRange(wholeProgressRange, this->CurrentPiece, this->NumberOfPieces);
    }

    int result = 1;
    if ((this->CurrentPiece == 0 && this->CurrentTimeIndex == 0) || this->WritePiece >= 0)
    {
      // We are just starting to write.  Do not call
      // UpdateProgressDiscrete because we want a 0 progress callback the
      // first time.
      this->UpdateProgress(0);

      // Initialize progress range to entire 0..1 range.
      if (this->WritePiece >= 0)
      {
        float wholeProgressRange[2] = { 0, 1 };
        this->SetProgressRange(wholeProgressRange, 0, 1);
      }

      if (!this->OpenStream())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      if (this->GetInputAsDataSet() != nullptr)
      {
        // use the current version for the file.
        this->UsePreviousVersion = false;
      }

      // Write the file.
      if (!this->StartFile())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      if (!this->WriteHeader())
      {
        this->NumberOfPieces = numPieces;
        return 0;
      }

      this->CurrentTimeIndex = 0;

      if (this->DataMode == vtkXMLWriter::Appended && this->FieldDataOM->GetNumberOfElements())
      {
        vtkNew<vtkFieldData> fieldDataCopy;
        this->UpdateFieldData(fieldDataCopy);

        // Write the field data arrays.
        this->WriteFieldDataAppendedData(
          this->GetInput()->GetFieldData(), this->CurrentTimeIndex, this->FieldDataOM);
        if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }
      }
    }

    if (!(this->UserContinueExecuting == 0)) // if user ask to stop do not try to write a piece
    {
      result = this->WriteAPiece();
    }

    if ((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      // Tell the pipeline to start looping.
      if (this->CurrentPiece == 0)
      {
        request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      }
      this->CurrentPiece++;
    }

    if (this->CurrentPiece == this->NumberOfPieces || this->WritePiece >= 0)
    {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentPiece = 0;
      // We are done writing all the pieces, lets loop over time now:
      this->CurrentTimeIndex++;

      if (this->UserContinueExecuting != 1)
      {
        if (!this->WriteFooter())
        {
          this->NumberOfPieces = numPieces;
          return 0;
        }

        if (!this->EndFile())
        {
          this->NumberOfPieces = numPieces;
          return 0;
        }

        this->CloseStream();
        this->CurrentTimeIndex = 0; // Reset
      }
    }
    this->NumberOfPieces = numPieces;

    // We have finished writing (at least this piece)
    this->SetProgressPartial(1);
    return result;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::AllocatePositionArrays()
{
  this->NumberOfColsPositions = new vtkTypeInt64[this->NumberOfPieces];
  this->NumberOfRowsPositions = new vtkTypeInt64[this->NumberOfPieces];

  this->RowsOM->Allocate(this->NumberOfPieces);
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::DeletePositionArrays()
{
  delete[] this->NumberOfColsPositions;
  delete[] this->NumberOfRowsPositions;
  this->NumberOfColsPositions = nullptr;
  this->NumberOfRowsPositions = nullptr;
}

//----------------------------------------------------------------------------
int vtkXMLTableWriter::WriteHeader()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if (!this->WritePrimaryElement(os, indent))
  {
    return 0;
  }

  this->WriteFieldData(indent.GetNextIndent());

  if (this->DataMode == vtkXMLWriter::Appended)
  {
    vtkIndent nextIndent = indent.GetNextIndent();

    this->AllocatePositionArrays();

    if ((this->WritePiece < 0) || (this->WritePiece >= this->NumberOfPieces))
    {
      // Loop over each piece and write its structure.
      for (int currentPieceIndex = 0; currentPieceIndex < this->NumberOfPieces; ++currentPieceIndex)
      {
        // Open the piece's element.
        os << nextIndent << "<Piece";
        this->WriteAppendedPieceAttributes(currentPieceIndex);
        if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }
        os << ">\n";

        this->WriteAppendedPiece(currentPieceIndex, nextIndent.GetNextIndent());
        if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
        {
          this->DeletePositionArrays();
          return 0;
        }

        // Close the piece's element.
        os << nextIndent << "</Piece>\n";
      }
    }
    else
    {
      // Write just the requested piece.
      // Open the piece's element.
      os << nextIndent << "<Piece";
      this->WriteAppendedPieceAttributes(this->WritePiece);
      if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }
      os << ">\n";

      this->WriteAppendedPiece(this->WritePiece, nextIndent.GetNextIndent());
      if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
        this->DeletePositionArrays();
        return 0;
      }

      // Close the piece's element.
      os << nextIndent << "</Piece>\n";
    }

    // Close the primary element.
    os << indent << "</" << this->GetDataSetName() << ">\n";
    os.flush();
    if (os.fail())
    {
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      this->DeletePositionArrays();
      return 0;
    }

    this->StartAppendedData();
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      this->DeletePositionArrays();
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLTableWriter::WriteAPiece()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  int result = 1;

  if (this->DataMode == vtkXMLWriter::Appended)
  {
    this->WriteAppendedPieceData(this->CurrentPiece);
  }
  else
  {
    result = this->WriteInlineMode(indent);
  }

  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    this->DeletePositionArrays();
    result = 0;
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLTableWriter::WriteFooter()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if (this->DataMode == vtkXMLWriter::Appended)
  {
    this->DeletePositionArrays();
    this->EndAppendedData();
  }
  else
  {
    // Close the primary element.
    os << indent << "</" << this->GetDataSetName() << ">\n";
    os.flush();
    if (os.fail())
    {
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLTableWriter::WriteInlineMode(vtkIndent indent)
{
  ostream& os = *(this->Stream);
  vtkIndent nextIndent = indent.GetNextIndent();

  // Open the piece's element.
  os << nextIndent << "<Piece";
  this->WriteInlinePieceAttributes();
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }
  os << ">\n";

  this->WriteInlinePiece(nextIndent.GetNextIndent());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }

  // Close the piece's element.
  os << nextIndent << "</Piece>\n";

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::WriteInlinePieceAttributes()
{
  vtkTable* input = this->GetInputAsTable();
  this->WriteScalarAttribute("NumberOfCols", input->GetNumberOfColumns());
  this->WriteScalarAttribute("NumberOfRows", input->GetNumberOfRows());
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::WriteInlinePiece(vtkIndent indent)
{
  vtkTable* input = this->GetInputAsTable();

  // Split progress among row data arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);

  // Set the range of progress for the row data arrays.
  this->SetProgressRange(progressRange, 0, 2);

  // Write the row data arrays.
  this->WriteRowDataInline(input->GetRowData(), indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the row data arrays.
  this->SetProgressRange(progressRange, 1, 2);
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::WriteAppendedPieceAttributes(int index)
{
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->NumberOfColsPositions[index] = this->ReserveAttributeSpace("NumberOfCols");
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->NumberOfRowsPositions[index] = this->ReserveAttributeSpace("NumberOfRows");
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::WriteAppendedPiece(int index, vtkIndent indent)
{
  vtkTable* input = this->GetInputAsTable();

  this->WriteRowDataAppended(input->GetRowData(), indent, &this->RowsOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::WriteAppendedPieceData(int index)
{
  ostream& os = *(this->Stream);
  vtkTable* input = this->GetInputAsTable();

  std::streampos returnPosition = os.tellp();

  os.seekp(std::streampos(this->NumberOfRowsPositions[index]));
  this->WriteScalarAttribute("NumberOfRows", input->GetNumberOfRows());
  os.seekp(returnPosition);

  os.seekp(std::streampos(this->NumberOfColsPositions[index]));
  this->WriteScalarAttribute("NumberOfCols", input->GetNumberOfColumns());
  os.seekp(returnPosition);

  // Split progress among row arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);

  // Set the range of progress for the row data arrays.
  this->SetProgressRange(progressRange, 0, 2);

  // Write the row data arrays.
  this->WriteRowDataAppendedData(
    input->GetRowData(), this->CurrentTimeIndex, &this->RowsOM->GetPiece(index));
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the row data arrays.
  this->SetProgressRange(progressRange, 1, 2);
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::WriteRowDataAppended(
  vtkDataSetAttributes* ds, vtkIndent indent, OffsetsManagerGroup* dsManager)
{
  ostream& os = *(this->Stream);
  int numberOfArrays = ds->GetNumberOfArrays();
  char** names = this->CreateStringArray(numberOfArrays);

  os << indent << "<RowData";
  this->WriteAttributeIndices(ds, names);

  if (this->ErrorCode != vtkErrorCode::NoError)
  {
    this->DestroyStringArray(numberOfArrays, names);
    return;
  }

  os << ">\n";

  dsManager->Allocate(numberOfArrays);
  for (int i = 0; i < numberOfArrays; ++i)
  {
    dsManager->GetElement(i).Allocate(this->NumberOfTimeSteps);
    for (int t = 0; t < this->NumberOfTimeSteps; ++t)
    {
      this->WriteArrayAppended(
        ds->GetAbstractArray(i), indent.GetNextIndent(), dsManager->GetElement(i), names[i], 0, t);
      if (this->ErrorCode != vtkErrorCode::NoError)
      {
        this->DestroyStringArray(numberOfArrays, names);
        return;
      }
    }
  }

  os << indent << "</RowData>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
  }
  this->DestroyStringArray(numberOfArrays, names);
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::WriteRowDataAppendedData(
  vtkDataSetAttributes* ds, int timestep, OffsetsManagerGroup* dsManager)
{
  float progressRange[2] = { 0.f, 0.f };

  this->GetProgressRange(progressRange);
  int numberOfArrays = ds->GetNumberOfArrays();
  for (int i = 0; i < numberOfArrays; ++i)
  {
    this->SetProgressRange(progressRange, i, numberOfArrays);
    vtkMTimeType mtime = ds->GetMTime();
    // Only write ds if MTime has changed
    vtkMTimeType& dsMTime = dsManager->GetElement(i).GetLastMTime();
    vtkAbstractArray* currentAbstractArray = ds->GetAbstractArray(i);
    if (dsMTime != mtime)
    {
      dsMTime = mtime;
      this->WriteArrayAppendedData(currentAbstractArray,
        dsManager->GetElement(i).GetPosition(timestep),
        dsManager->GetElement(i).GetOffsetValue(timestep));
      if (this->ErrorCode != vtkErrorCode::NoError)
      {
        return;
      }
    }
    else
    {
      assert(timestep > 0);
      dsManager->GetElement(i).GetOffsetValue(timestep) =
        dsManager->GetElement(i).GetOffsetValue(timestep - 1);
      this->ForwardAppendedDataOffset(dsManager->GetElement(i).GetPosition(timestep),
        dsManager->GetElement(i).GetOffsetValue(timestep), "offset");
    }
    vtkDataArray* currentDataArray = vtkArrayDownCast<vtkDataArray>(currentAbstractArray);
    if (currentDataArray)
    {
      // ranges are only written in case of Data Arrays.
      double* range = currentDataArray->GetRange(-1);
      this->ForwardAppendedDataDouble(
        dsManager->GetElement(i).GetRangeMinPosition(timestep), range[0], "RangeMin");
      this->ForwardAppendedDataDouble(
        dsManager->GetElement(i).GetRangeMaxPosition(timestep), range[1], "RangeMax");
    }
  }
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::WriteRowDataInline(vtkDataSetAttributes* ds, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  int numberOfArrays = ds->GetNumberOfArrays();
  char** names = this->CreateStringArray(numberOfArrays);

  os << indent << "<RowData";
  this->WriteAttributeIndices(ds, names);

  if (this->ErrorCode != vtkErrorCode::NoError)
  {
    this->DestroyStringArray(numberOfArrays, names);
    return;
  }

  os << ">\n";

  float progressRange[2] = { 0.f, 1.f };
  this->GetProgressRange(progressRange);
  for (int i = 0; i < numberOfArrays; ++i)
  {
    this->SetProgressRange(progressRange, i, numberOfArrays);
    vtkAbstractArray* currentAbstractArray = ds->GetAbstractArray(i);
    this->WriteArrayInline(currentAbstractArray, indent.GetNextIndent(), names[i]);
    if (this->ErrorCode != vtkErrorCode::NoError)
    {
      this->DestroyStringArray(numberOfArrays, names);
      return;
    }
  }

  os << indent << "</RowData>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
    this->DestroyStringArray(numberOfArrays, names);
    return;
  }

  this->DestroyStringArray(numberOfArrays, names);
}

//----------------------------------------------------------------------------
void vtkXMLTableWriter::SetInputUpdateExtent(int piece, int numPieces)
{
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
}
