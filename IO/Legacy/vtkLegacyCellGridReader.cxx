// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLegacyCellGridReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellGrid.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLegacyCellGridReader);

#ifdef read
#undef read
#endif

//------------------------------------------------------------------------------
vtkLegacyCellGridReader::vtkLegacyCellGridReader() = default;

//------------------------------------------------------------------------------
vtkLegacyCellGridReader::~vtkLegacyCellGridReader() = default;

//------------------------------------------------------------------------------
vtkCellGrid* vtkLegacyCellGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkCellGrid* vtkLegacyCellGridReader::GetOutput(int idx)
{
  return vtkCellGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//------------------------------------------------------------------------------
void vtkLegacyCellGridReader::SetOutput(vtkCellGrid* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//------------------------------------------------------------------------------
int vtkLegacyCellGridReader::ReadMeshSimple(const std::string& fname, vtkDataObject* doOutput)
{
  char line[256];
  vtkCellGrid* output = vtkCellGrid::SafeDownCast(doOutput);

  vtkDebugMacro(<< "Reading vtk cell grid...");

  if (!this->OpenVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read cell grid specific stuff
  //
  if (!this->ReadString(line))
  {
    vtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseVTKFile();
    return 1;
  }

  if (!strncmp(this->LowerCase(line), "dataset", 7))
  {
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
    {
      vtkErrorMacro(<< "Data file ends prematurely!");
      this->CloseVTKFile();
      return 1;
    }

    if (strncmp(this->LowerCase(line), "cell_grid", 9) != 0)
    {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile();
      return 1;
    }

    vtkIdType contentLength;
    if (!this->Read(&contentLength))
    {
      vtkErrorMacro(<< "Cannot read dataset length: " << line);
      this->CloseVTKFile();
      return 1;
    }

    if (!this->ReadLine(line))
    {
      vtkErrorMacro(<< "Cannot read end-of-line past dataset length: " << line);
      this->CloseVTKFile();
      return 1;
    }

    std::string raw;
    raw.resize(contentLength);
    if (!this->GetIStream()->read(
          const_cast<char*>(raw.data()), static_cast<std::streamsize>(contentLength)))
    {
      vtkErrorMacro(<< "Cannot read encoded dataset.");
      this->CloseVTKFile();
      return 1;
    }

    nlohmann::json jdata;
    try
    {
      jdata = nlohmann::json::parse(raw);
    }
    catch (nlohmann::json::exception& e)
    {
      vtkErrorMacro(<< "JSON parse error \"" << e.what() << "\".");
      return 0;
    }

    if (!this->Subreader->FromJSON(jdata, output))
    {
      vtkErrorMacro(<< "Cannot parse encoded dataset.");
      this->CloseVTKFile();
      return 1;
    }
  }

  vtkDebugMacro(<< "Read " << output->GetNumberOfCells() << " cells.\n");
  this->CloseVTKFile();
  return 1;
}

//------------------------------------------------------------------------------
int vtkLegacyCellGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkCellGrid");
  return 1;
}

//------------------------------------------------------------------------------
void vtkLegacyCellGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
