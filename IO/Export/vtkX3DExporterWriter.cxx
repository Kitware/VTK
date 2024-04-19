// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkX3DExporterWriter.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkX3DExporterWriter::vtkX3DExporterWriter()
{
  this->WriteToOutputString = 0;
  this->OutputString = nullptr;
  this->OutputStringLength = 0;
}

//------------------------------------------------------------------------------
vtkX3DExporterWriter::~vtkX3DExporterWriter()
{
  delete[] this->OutputString;
  this->OutputString = nullptr;
}

//------------------------------------------------------------------------------
void vtkX3DExporterWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WriteToOutputString: " << (this->WriteToOutputString ? "On" : "Off")
     << std::endl;
  os << indent << "OutputStringLength: " << this->OutputStringLength << std::endl;
  if (this->OutputString)
  {
    os << indent << "OutputString: " << this->OutputString << std::endl;
  }
}

//------------------------------------------------------------------------------
char* vtkX3DExporterWriter::RegisterAndGetOutputString()
{
  char* tmp = this->OutputString;

  this->OutputString = nullptr;
  this->OutputStringLength = 0;

  return tmp;
}
VTK_ABI_NAMESPACE_END
