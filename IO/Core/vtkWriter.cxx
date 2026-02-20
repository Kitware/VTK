// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_6_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkWriter.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStringFormatter.h"

#include <sstream>

// Construct with no start and end write methods or arguments.
VTK_ABI_NAMESPACE_BEGIN
vtkWriter::vtkWriter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

vtkWriter::~vtkWriter() = default;

void vtkWriter::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

void vtkWriter::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

vtkDataObject* vtkWriter::GetInput()
{
  return this->GetInput(0);
}

vtkDataObject* vtkWriter::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(port, 0);
}

// Write data to output. Method executes subclasses WriteData() method, as
// well as StartMethod() and EndMethod() methods.
int vtkWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    vtkErrorMacro("No input provided!");
    return 0;
  }

  // always write even if the data hasn't changed
  this->Modified();
  bool ret = this->UpdateWholeExtent();
  ret &= this->GetErrorCode() == vtkErrorCode::NoError;
  return ret;
}

vtkTypeBool vtkWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkWriter::RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  this->SetErrorCode(vtkErrorCode::NoError);

  vtkDataObject* input = this->GetInput();

  // make sure input is available
  if (!input)
  {
    vtkErrorMacro(<< "No input!");
    return 0;
  }

  this->InvokeEvent(vtkCommand::StartEvent, nullptr);
  bool ret = this->WriteDataAndReturn();
  this->InvokeEvent(vtkCommand::EndEvent, nullptr);

  this->WriteTime.Modified();

  return ret ? 1 : 0;
}

void vtkWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkWriter::EncodeString(char* resname, const char* name)
{
  std::ostringstream str;
  vtkWriter::EncodeWriteString(&str, name);
  const auto string = str.str();
  std::copy_n(string.c_str(), string.size() + 1, resname);
}

void vtkWriter::EncodeString(char* resname, const char* name, bool doublePercent)
{
  std::ostringstream str;
  vtkWriter::EncodeWriteString(&str, name, doublePercent);
  const auto string = str.str();
  std::copy_n(string.c_str(), string.size() + 1, resname);
}

void vtkWriter::EncodeWriteString(ostream* out, const char* name)
{
  if (!name)
  {
    return;
  }
  int cc = 0;

  char buffer[10];

  while (name[cc])
  {
    // Encode spaces and %'s (and most non-printable ascii characters)
    // The reader does not support spaces in strings.
    if (name[cc] < 33 || name[cc] > 126 || name[cc] == '\"' || name[cc] == '%')
    {
      auto result =
        vtk::format_to_n(buffer, sizeof(buffer), "{:02X}", static_cast<unsigned char>(name[cc]));
      *result.out = '\0';
      *out << "%" << buffer;
    }
    else
    {
      *out << name[cc];
    }
    cc++;
  }
}

void vtkWriter::EncodeWriteString(ostream* out, const char* name, bool doublePercent)
{
  if (!name)
  {
    return;
  }
  int cc = 0;

  char buffer[10];

  while (name[cc])
  {
    // Encode spaces and %'s (and most non-printable ascii characters)
    // The reader does not support spaces in strings.
    if (name[cc] < 33 || name[cc] > 126 || name[cc] == '\"' || name[cc] == '%')
    {
      auto result =
        vtk::format_to_n(buffer, sizeof(buffer), "{:02X}", static_cast<unsigned char>(name[cc]));
      *result.out = '\0';
      if (doublePercent)
      {
        *out << "%%";
      }
      else
      {
        *out << "%";
      }
      *out << buffer;
    }
    else
    {
      *out << name[cc];
    }
    cc++;
  }
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_7_0 remove fully
void vtkWriter::WriteData()
{
  if (!this->WriteDataFlag)
  {
    this->WriteDataAndReturn();
  }
  else
  {
    this->WriteDataOverrideError = true;
  }
};

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_7_0 remove fully
bool vtkWriter::WriteDataAndReturn()
{
  if (!this->WriteDataFlag)
  {
    this->WriteDataFlag = true;
    this->WriteData();
  }
  else
  {
    this->WriteDataOverrideError = true;
  }

  if (this->WriteDataOverrideError)
  {
    // This is a runtime override warning in order to provide retro-compatibility with WriteData
    vtkErrorMacro(
      "This writer doesn't have a WriteDataAndReturn override implementation, but it should");
    return false;
  }
  return true;
};

VTK_ABI_NAMESPACE_END
