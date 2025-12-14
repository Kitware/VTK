// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLStatisticalModelReader.h"

#include "vtkBase64Utilities.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStatisticalModel.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringScanner.h"
#include "vtkTable.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTableReader.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLStatisticalModelReader);

vtkXMLStatisticalModelReader::vtkXMLStatisticalModelReader()
{
  this->ParamElement = nullptr;
}

vtkXMLStatisticalModelReader::~vtkXMLStatisticalModelReader() = default;

void vtkXMLStatisticalModelReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkStatisticalModel* vtkXMLStatisticalModelReader::GetOutput()
{
  return this->GetOutput(0);
}

vtkStatisticalModel* vtkXMLStatisticalModelReader::GetOutput(int idx)
{
  return vtkStatisticalModel::SafeDownCast(this->GetOutputDataObject(idx));
}

const char* vtkXMLStatisticalModelReader::GetDataSetName()
{
  return "StatisticalModel";
}

void vtkXMLStatisticalModelReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

void vtkXMLStatisticalModelReader::ReadXMLData()
{
  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();
  this->ReadFieldData();

  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);

  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  std::vector<float> fractions(2);
  fractions[0] = 0;
  fractions[1] = 1;

  // Set the range of progress for this piece.
  this->SetProgressRange(progressRange, 0, fractions.data());

  if (!this->ReadPieceData(/*currentIndex*/ 0))
  {
    // An error occurred while reading the piece.
    this->DataError = 1;
  }
}

void vtkXMLStatisticalModelReader::SetupOutputInformation(vtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  if (this->InformationError)
  {
    vtkErrorMacro("Should not still be processing output information if have set InformationError");
    return;
  }

  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 0);
}

int vtkXMLStatisticalModelReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Count the number of pieces in the file.
  int numNested = ePrimary->GetNumberOfNestedElements();
  if (numNested != 1)
  {
    vtkErrorMacro("Statistical models must have a single piece for now.");
    return 0;
  }

  if (!this->ReadPiece(ePrimary->GetNestedElement(0)))
  {
    return 0;
  }
  return 1;
}

void vtkXMLStatisticalModelReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
}

int vtkXMLStatisticalModelReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if (ePiece->GetNumberOfNestedElements() != 1)
  {
    vtkErrorMacro("Piece must contain a single StatisticalModelData element for now.");
    return 0;
  }
  auto* modelDataElem = ePiece->GetNestedElement(0);
  this->ParamElement = nullptr;
  this->TableGroupElements.clear();
  // Find the children we accept in the "piece".
  for (int i = 0; i < modelDataElem->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* eNested = modelDataElem->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "AlgorithmParameters") == 0)
    {
      if (this->ParamElement)
      {
        vtkErrorMacro("More than one \"AlgorithmParameters\" element in the model.");
      }
      else
      {
        this->ParamElement = eNested;
      }
    }
    else if (strcmp(eNested->GetName(), "ModelTables") == 0)
    {
      this->TableGroupElements.push_back(eNested);
    }
  }

  if (!this->ParamElement)
  {
    vtkErrorMacro("Model is missing \"AlgorithmParameters\" element.");
    return 0;
  }
  // NB: We do not require this->TableGroupElements to have any entries,
  // but we could check that each one has a unique "Type" attribute.

  // TODO: We could check more here (i.e., existence of <Table> entries in <Tables> elements).
  return 1;
}

int vtkXMLStatisticalModelReader::ReadPieceData(int vtkNotUsed(piece))
{
  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);

  // Set the range of progress.
  this->SetProgressRange(progressRange, 0, 2);

  // Let the superclass read its data.
  vtkStatisticalModel* output = vtkStatisticalModel::SafeDownCast(this->GetCurrentOutput());

  auto* pdata = this->ParamElement->GetCharacterData();
  output->SetAlgorithmParameters(strlen(pdata) > 0 ? pdata : nullptr);
  vtkNew<vtkXMLTableReader> tableReader;
  tableReader->ReadFromInputStringOn();
  for (const auto& tableGroupElement : this->TableGroupElements)
  {
    int nn = tableGroupElement->GetNumberOfNestedElements();
    const char* tableTypeChar = tableGroupElement->GetAttribute("Type");
    if (!tableTypeChar)
    {
      vtkErrorMacro("No table type for \"ModelTables\" element. Skipping.");
      continue;
    }
    int tableType = vtkStatisticalModel::GetTableTypeValue(tableTypeChar);
    if (tableType < 0)
    {
      vtkErrorMacro(
        "Invalid table type \"" << tableTypeChar << "\" for \"ModelTables\" element. Skipping.");
      continue;
    }
    const char* numModelTablesStr = tableGroupElement->GetAttribute("NumberOfTables");
    if (!numModelTablesStr)
    {
      vtkErrorMacro("The \"NumberOfTables\" attribute of the \"ModelTables\" element is missing.");
      return 0;
    }
    int numModelTables = vtk::scan_int<int>(numModelTablesStr)->value();
    output->SetNumberOfTables(tableType, numModelTables);
    int modelTableIdx = 0;
    for (int tt = 0; tt < nn; ++tt)
    {
      if (tableGroupElement->GetNestedElement(tt)->GetName() != std::string("ModelTable"))
      {
        // Skip non-ModelTable entries (such as comments).
        continue;
      }
      std::string tableName;
      const char* tableNameChar = tableGroupElement->GetNestedElement(tt)->GetAttribute("Name");
      if (!tableNameChar || !tableNameChar[0])
      {
        vtkErrorMacro("Missing \"Name\" attribute for \"Table\" element.");
      }
      else
      {
        tableName = tableNameChar;
      }
      char* buffer = tableGroupElement->GetNestedElement(tt)->GetCharacterData();
      std::string decoded(strlen(buffer), '\0');
      std::size_t decodeLen =
        vtkBase64Utilities::DecodeSafely(reinterpret_cast<const unsigned char*>(buffer),
          strlen(buffer), reinterpret_cast<unsigned char*>(decoded.data()), strlen(buffer));
      decoded.resize(decodeLen);
      tableReader->SetInputString(decoded.c_str());
      tableReader->Update();
      if (modelTableIdx >= numModelTables)
      {
        vtkErrorMacro("Too many ModelTable elements. Skipping.");
        continue;
      }
      // Copy the table since the reader re-purposes its output for the next
      // table in the model:
      vtkNew<vtkTable> modelTable;
      modelTable->ShallowCopy(tableReader->GetOutput());
      // Add the table to the model:
      output->SetTable(tableType, modelTableIdx++, modelTable, tableName);
    }
  }

  return 1;
}

int vtkXMLStatisticalModelReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkStatisticalModel");
  return 1;
}
VTK_ABI_NAMESPACE_END
