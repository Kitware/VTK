// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkArrayDataReader.h"

#include "vtkArray.h"
#include "vtkArrayData.h"
#include "vtkArrayReader.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtksys/FStream.hxx"

#include <sstream>
#include <stdexcept>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkArrayDataReader);

vtkArrayDataReader::vtkArrayDataReader()
  : FileName(nullptr)
{
  this->SetNumberOfInputPorts(0);
  this->ReadFromInputString = false;
}

vtkArrayDataReader::~vtkArrayDataReader()
{
  this->SetFileName(nullptr);
}

void vtkArrayDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "InputString: " << this->InputString << endl;
  os << indent << "ReadFromInputString: " << (this->ReadFromInputString ? "on" : "off") << endl;
}

void vtkArrayDataReader::SetInputString(const vtkStdString& string)
{
  this->InputString = string;
  this->Modified();
}

vtkStdString vtkArrayDataReader::GetInputString()
{
  return this->InputString;
}

int vtkArrayDataReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  try
  {
    vtkArrayData* array_data = nullptr;
    if (this->ReadFromInputString)
    {
      array_data = vtkArrayDataReader::Read(this->InputString);
    }
    else
    {
      if (!this->FileName)
        throw std::runtime_error("FileName not set.");

      vtksys::ifstream file(this->FileName, std::ios::binary);

      array_data = vtkArrayDataReader::Read(file);
    }
    if (!array_data)
      throw std::runtime_error("Error reading vtkArrayData.");

    vtkArrayData* const output_array_data = vtkArrayData::GetData(outputVector);
    output_array_data->ShallowCopy(array_data);
    array_data->Delete();

    return 1;
  }
  catch (std::exception& e)
  {
    vtkErrorMacro(<< e.what());
  }

  return 0;
}

vtkArrayData* vtkArrayDataReader::Read(const vtkStdString& str)
{
  std::istringstream iss(str);
  return vtkArrayDataReader::Read(iss);
}

vtkArrayData* vtkArrayDataReader::Read(istream& stream)
{
  try
  {
    // Read enough of the file header to identify the type ...
    std::string header_string;
    std::getline(stream, header_string);
    std::istringstream header_buffer(header_string);

    std::string header_name;
    vtkIdType header_size;
    header_buffer >> header_name >> header_size;

    if (header_name != "vtkArrayData")
    {
      throw std::runtime_error("Not a vtkArrayData file");
    }
    if (header_size < 0)
    {
      throw std::runtime_error("Invalid number of arrays");
    }
    vtkArrayData* data = vtkArrayData::New();
    for (vtkIdType i = 0; i < header_size; ++i)
    {
      vtkArray* a = vtkArrayReader::Read(stream);
      data->AddArray(a);
      a->Delete();
    }
    return data;
  }
  catch (std::exception& e)
  {
    vtkGenericWarningMacro(<< e.what());
  }

  return nullptr;
}
VTK_ABI_NAMESPACE_END
