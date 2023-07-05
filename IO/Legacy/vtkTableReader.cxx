// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTableReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTableReader);

#ifdef read
#undef read
#endif

//------------------------------------------------------------------------------
vtkTableReader::vtkTableReader() = default;

//------------------------------------------------------------------------------
vtkTableReader::~vtkTableReader() = default;

//------------------------------------------------------------------------------
vtkTable* vtkTableReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkTable* vtkTableReader::GetOutput(int idx)
{
  return vtkTable::SafeDownCast(this->GetOutputDataObject(idx));
}

//------------------------------------------------------------------------------
void vtkTableReader::SetOutput(vtkTable* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//------------------------------------------------------------------------------
int vtkTableReader::ReadMeshSimple(const std::string& fname, vtkDataObject* doOutput)
{
  vtkDebugMacro(<< "Reading vtk table...");

  if (!this->OpenVTKFile(fname.c_str()) || !this->ReadHeader())
  {
    return 1;
  }

  // Read table-specific stuff
  char line[256];
  if (!this->ReadString(line))
  {
    vtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseVTKFile();
    return 1;
  }

  if (strncmp(this->LowerCase(line), "dataset", 7) != 0)
  {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    this->CloseVTKFile();
    return 1;
  }

  if (!this->ReadString(line))
  {
    vtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseVTKFile();
    return 1;
  }

  if (strncmp(this->LowerCase(line), "table", 5) != 0)
  {
    vtkErrorMacro(<< "Cannot read dataset type: " << line);
    this->CloseVTKFile();
    return 1;
  }

  vtkTable* const output = vtkTable::SafeDownCast(doOutput);

  while (true)
  {
    if (!this->ReadString(line))
    {
      break;
    }

    if (!strncmp(this->LowerCase(line), "field", 5))
    {
      vtkFieldData* const field_data = this->ReadFieldData();
      output->SetFieldData(field_data);
      field_data->Delete();
      continue;
    }

    if (!strncmp(this->LowerCase(line), "row_data", 8))
    {
      vtkIdType row_count = 0;
      if (!this->Read(&row_count))
      {
        vtkErrorMacro(<< "Cannot read number of rows!");
        this->CloseVTKFile();
        return 1;
      }

      this->ReadRowData(output, row_count);
      continue;
    }

    vtkErrorMacro(<< "Unrecognized keyword: " << line);
  }

  vtkDebugMacro(<< "Read " << output->GetNumberOfRows() << " rows in "
                << output->GetNumberOfColumns() << " columns.\n");

  this->CloseVTKFile();

  return 1;
}

//------------------------------------------------------------------------------
int vtkTableReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
  return 1;
}

//------------------------------------------------------------------------------
void vtkTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
