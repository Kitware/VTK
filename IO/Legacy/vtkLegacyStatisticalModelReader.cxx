// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLegacyStatisticalModelReader.h"

#include "vtkBase64Utilities.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStatisticalModel.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTableReader.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLegacyStatisticalModelReader);

#ifdef read
#undef read
#endif

namespace
{

template <typename LineType>
bool ReadEncodedBlock(const char* blockName, std::string& decodedData,
  vtkLegacyStatisticalModelReader* self, LineType& line)
{
  if (!self->ReadString(line) || strncmp(self->LowerCase(line), blockName, strlen(blockName)) != 0)
  {
    vtkErrorWithObjectMacro(self, << "Cannot read " << blockName << " \"" << line << "\"!");
    return false;
  }

  vtkIdType encodedContentLength;
  vtkIdType decodedContentLength;
  if (!self->Read(&encodedContentLength) || !self->Read(&decodedContentLength))
  {
    vtkErrorWithObjectMacro(self, << "Cannot read content length: " << line);
    return false;
  }

  // Now read encoded data, beginning with a newline.
  if (!self->ReadLine(line))
  {
    vtkErrorWithObjectMacro(self, << "Cannot read end-of-line past content length: " << line);
    return false;
  }

  if (encodedContentLength > 0)
  {
    // Use 'char' instead of 'uint8_t' to avoid the following clang >= 19 error:
    // "implicit instantiation of undefined template 'std::char_traits<unsigned char>'"
    std::vector<char> raw;
    raw.resize(encodedContentLength);
    if (!self->GetIStream()->read(raw.data(), static_cast<std::streamsize>(encodedContentLength)))
    {
      vtkErrorWithObjectMacro(self, << "Cannot read encoded " << blockName << " content.");
      return false;
    }
    decodedData.resize(decodedContentLength, '\0');
    auto decodedLength = static_cast<vtkIdType>(vtkBase64Utilities::DecodeSafely(
      reinterpret_cast<const unsigned char*>(raw.data()), encodedContentLength,
      reinterpret_cast<unsigned char*>(decodedData.data()), decodedData.size()));
    decodedData.resize(decodedLength < decodedContentLength ? decodedLength : decodedContentLength);
  }
  return true;
}

} // anonymous namespace

vtkLegacyStatisticalModelReader::vtkLegacyStatisticalModelReader() = default;

vtkLegacyStatisticalModelReader::~vtkLegacyStatisticalModelReader() = default;

vtkStatisticalModel* vtkLegacyStatisticalModelReader::GetOutput()
{
  return this->GetOutput(0);
}

vtkStatisticalModel* vtkLegacyStatisticalModelReader::GetOutput(int idx)
{
  return vtkStatisticalModel::SafeDownCast(this->GetOutputDataObject(idx));
}

void vtkLegacyStatisticalModelReader::SetOutput(vtkStatisticalModel* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

int vtkLegacyStatisticalModelReader::ReadMeshSimple(
  const std::string& fname, vtkDataObject* doOutput)
{
  char line[256];
  vtkStatisticalModel* output = vtkStatisticalModel::SafeDownCast(doOutput);

  vtkDebugMacro(<< "Reading vtk statistical model...");

  if (!this->OpenVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read stuff specific to statistical model
  if (!this->ReadString(line) || strncmp(this->LowerCase(line), "dataset", 7) != 0)
  {
    vtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseVTKFile();
    return 1;
  }

  // Make sure we're reading right type of data
  if (!this->ReadString(line) || strncmp(this->LowerCase(line), "statistical_model", 17) != 0)
  {
    vtkErrorMacro(<< "Cannot read dataset type \"" << line << "\"!");
    this->CloseVTKFile();
    return 1;
  }

  int numberOfTableGroups;
  if (!this->Read(&numberOfTableGroups))
  {
    vtkErrorMacro(<< "Cannot read number of table groups.");
    this->CloseVTKFile();
    return 1;
  }

  // Read the algorithm parameters
  std::string decodedData;
  if (!ReadEncodedBlock("algorithm_parameters", decodedData, this, line))
  {
    vtkErrorMacro(<< "Cannot read algorithm parameters \"" << line << "\"!");
    this->CloseVTKFile();
    return 1;
  }
  output->SetAlgorithmParameters(decodedData.empty() ? nullptr : decodedData.c_str());

  // Read table groups
  vtkNew<vtkTableReader> tableReader;
  tableReader->ReadFromInputStringOn();
  for (int gg = 0; gg < numberOfTableGroups; ++gg)
  {
    int numberOfTables;
    if (!this->ReadString(line) || strncmp(this->LowerCase(line), "model_tables", 12) != 0 ||
      !this->ReadString(line))
    {
      vtkErrorMacro(<< "Cannot read model table group " << gg << ".");
      this->CloseVTKFile();
      return 1;
    }
    // Line should be a table type-name
    int tableType = vtkStatisticalModel::GetTableTypeValue(line);
    if (tableType < 0)
    {
      vtkErrorMacro(<< "Unrecognized table group " << gg << " type-name \"" << line << "\".");
      this->CloseVTKFile();
      return 1;
    }
    if (!this->Read(&numberOfTables))
    {
      vtkErrorMacro(<< "Cannot read model table group " << gg << " size.");
      this->CloseVTKFile();
      return 1;
    }
    output->SetNumberOfTables(tableType, numberOfTables);
    std::string tableName;
    for (int tt = 0; tt < numberOfTables; ++tt)
    {
      if (!ReadEncodedBlock("name", tableName, this, line) ||
        !ReadEncodedBlock("model_table", decodedData, this, line))
      {
        this->CloseVTKFile();
        return 1;
      }
      tableReader->SetInputString(decodedData.c_str());
      tableReader->Update();
      vtkNew<vtkTable> modelTable;
      modelTable->ShallowCopy(tableReader->GetOutputDataObject(0));
      output->SetTable(tableType, tt, modelTable, tableName);
    }
  }

  if (!this->ReadLine(line))
  {
    vtkErrorMacro(<< "Cannot read end-of-line past dataset length: " << line);
    this->CloseVTKFile();
    return 1;
  }

  this->CloseVTKFile();
  return 1;
}

int vtkLegacyStatisticalModelReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkStatisticalModel");
  return 1;
}

void vtkLegacyStatisticalModelReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
