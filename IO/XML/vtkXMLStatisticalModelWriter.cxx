// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBase64Utilities.h"
#include "vtkDataArray.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStatisticalModel.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkXMLTableWriter.h"
#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLStatisticalModelWriter.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLStatisticalModelWriter);

vtkXMLStatisticalModelWriter::vtkXMLStatisticalModelWriter()
{
  this->DataMode = vtkXMLWriter::Ascii;
  this->FieldDataOM->Allocate(0);
  // this->ModelTablesOM = new OffsetsManagerArray;
}

vtkXMLStatisticalModelWriter::~vtkXMLStatisticalModelWriter() = default;

int vtkXMLStatisticalModelWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStatisticalModel");
  return 1;
}

void vtkXMLStatisticalModelWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkStatisticalModel* vtkXMLStatisticalModelWriter::GetModelInput()
{
  return static_cast<vtkStatisticalModel*>(this->Superclass::GetInput());
}

const char* vtkXMLStatisticalModelWriter::GetDataSetName()
{
  return "StatisticalModel";
}

const char* vtkXMLStatisticalModelWriter::GetDefaultFileExtension()
{
  return "vtstat";
}

vtkTypeBool vtkXMLStatisticalModelWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Force the DataMode to be inline.
  int prevDataMode = this->DataMode;
  this->DataMode = vtkXMLWriter::Ascii;
  int result = 1;
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->SetErrorCode(vtkErrorCode::NoError);

    if (!this->Stream && !this->FileName && !this->WriteToOutputString)
    {
      this->SetErrorCode(vtkErrorCode::NoFileNameError);
      vtkErrorMacro("The FileName or Stream must be set first or "
                    "the output must be written to a string.");
      this->DataMode = prevDataMode;
      return 0;
    }

    float wholeProgressRange[2] = { 0, 1 };
    this->SetProgressRange(wholeProgressRange, 0, 1);
    this->UpdateProgress(0);

    if (this->CurrentTimeIndex == 0)
    {
      // We are just starting to write.  Do not call
      // UpdateProgressDiscrete because we want a 0 progress callback the
      // first time.
      this->UpdateProgress(0);

      // Initialize progress range to entire 0..1 range.
      this->SetProgressRange(wholeProgressRange, 0, 1);

      if (!this->OpenStream())
      {
        this->DataMode = prevDataMode;
        return 0;
      }
      // Force ASCII model data to exactly represent double-precision
      // floating-point values:
      this->Stream->precision(17);

      if (this->GetDataSetInput() != nullptr)
      {
        // use the current version for the file.
        this->UsePreviousVersion = false;
      }

      // Write the file.
      if (!this->StartFile())
      {
        this->DataMode = prevDataMode;
        return 0;
      }

      if (!this->WriteHeader())
      {
        this->DataMode = prevDataMode;
        return 0;
      }

      this->CurrentTimeIndex = 0;

      // Ignore DataMode. We will always write inline data.
      // Models are small and appended data is too complex.
    }

    // If the user asks to stop, do not try to write a piece.
    if (!(this->UserContinueExecuting == 0))
    {
      result = this->WriteAPiece();
    }

    {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      // We are done writing all the pieces, lets loop over time now:
      this->CurrentTimeIndex++;

      if (this->UserContinueExecuting != 1)
      {
        if (!this->WriteFooter())
        {
          this->DataMode = prevDataMode;
          return 0;
        }

        if (!this->EndFile())
        {
          this->DataMode = prevDataMode;
          return 0;
        }

        this->CloseStream();
        this->CurrentTimeIndex = 0; // Reset
      }
    }

    // We have finished writing (at least this piece)
    this->SetProgressPartial(1);
    this->DataMode = prevDataMode;
    return result;
  }
  result = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  this->DataMode = prevDataMode;
  return result;
}

void vtkXMLStatisticalModelWriter::SetInputUpdateExtent(int piece, int numPieces)
{
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
}

int vtkXMLStatisticalModelWriter::WriteHeader()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  ostream& os = *(this->Stream);

  if (!this->WritePrimaryElement(os, indent))
  {
    return 0;
  }

  this->WriteFieldData(indent.GetNextIndent());

  // Do not support appended data for now.
  return 1;
}

int vtkXMLStatisticalModelWriter::WriteAPiece()
{
  vtkIndent indent = vtkIndent().GetNextIndent();

  int result = 1;

  if (this->DataMode == vtkXMLWriter::Appended)
  {
    this->WriteAppendedPieceData(/*CurrentPiece*/ 0);
  }
  else
  {
    result = this->WriteInlineModel(indent);
  }

  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    this->DeletePositionArrays();
    result = 0;
  }
  return result;
}

int vtkXMLStatisticalModelWriter::WriteFooter()
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

int vtkXMLStatisticalModelWriter::WriteInlineModel(vtkIndent indent)
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

void vtkXMLStatisticalModelWriter::WriteInlinePieceAttributes()
{
  vtkStatisticalModel* input = this->GetModelInput();
  this->WriteScalarAttribute(
    "NumberOfLearnedTables", input->GetNumberOfTables(vtkStatisticalModel::Learned));
  this->WriteScalarAttribute(
    "NumberOfDerivedTables", input->GetNumberOfTables(vtkStatisticalModel::Derived));
}

void vtkXMLStatisticalModelWriter::WriteInlinePiece(vtkIndent indent)
{
  vtkStatisticalModel* input = this->GetModelInput();

  // Split progress among row data arrays.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);

  // Set the range of progress for the row data arrays.
  this->SetProgressRange(progressRange, 0, 2);

  // Write the row data arrays.
  this->WriteModelDataInline(input, indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  // Set the range of progress for the row data arrays.
  this->SetProgressRange(progressRange, 1, 2);
}

void vtkXMLStatisticalModelWriter::WriteAppendedPieceData(int vtkNotUsed(index))
{
  // We do not support appended data for now.
  vtkErrorMacro("Appended data is not currently supported.");
}

void vtkXMLStatisticalModelWriter::WriteModelDataInline(vtkStatisticalModel* ds, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();
  os << indent << "<StatisticalModelData>\n";
  const char* params = ds->GetAlgorithmParameters();
  if (!params)
  {
    params = "";
  }
  os << i2 << "<AlgorithmParameters>" << params << "</AlgorithmParameters>\n";
  vtkNew<vtkXMLTableWriter> tableSerializer;
  tableSerializer->SetDataModeToAscii();
  tableSerializer->SetPrecision(17);
  tableSerializer->WriteToOutputStringOn();
  for (int ttype = vtkStatisticalModel::Learned; ttype <= vtkStatisticalModel::Derived; ++ttype)
  {
    int numTab = ds->GetNumberOfTables(ttype);
    if (numTab <= 0)
    {
      continue;
    }
    os << i2 << "<ModelTables Type=\"" << vtkStatisticalModel::GetTableTypeName(ttype)
       << "\" NumberOfTables=\"" << numTab << "\">\n";
    for (int ii = 0; ii < numTab; ++ii)
    {
      auto tab = ds->GetTable(ttype, ii);
      auto tabName = ds->GetTableName(ttype, ii);
      if (!tab)
      {
        continue;
      }
      tableSerializer->SetInputDataObject(tab);
      tableSerializer->Write();
      std::string tableData = tableSerializer->GetOutputString();
      std::string b64(tableData.size() + tableData.size() / 2, '\0');
      unsigned long encLen =
        vtkBase64Utilities::Encode(reinterpret_cast<const unsigned char*>(tableData.c_str()),
          static_cast<unsigned long>(tableData.size()),
          reinterpret_cast<unsigned char*>(b64.data()), 1);
      b64.resize(encLen);

      os << i3 << "<ModelTable Name=\"" << tabName << "\" Length=\"" << encLen << "\">"
         << b64.c_str() << "</ModelTable>\n";
    }
    os << i2 << "</ModelTables>\n";
  }
  os << indent << "</StatisticalModelData>\n";
}

void vtkXMLStatisticalModelWriter::AllocatePositionArrays()
{
  // The entire model is a single piece.
  // this->NumberOfColsPositions = new vtkTypeInt64[1];
  // this->NumberOfRowsPositions = new vtkTypeInt64[1];

  // this->ModelTablesOM->Allocate(1)
}

void vtkXMLStatisticalModelWriter::DeletePositionArrays()
{
  // delete[] this->NumberOfColsPositions;
  // delete[] this->NumberOfRowsPositions;
  // this->NumberOfColsPositions = nullptr;
  // this->NumberOfRowsPositions = nullptr;
}

VTK_ABI_NAMESPACE_END
