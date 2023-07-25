// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOutputStream.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOutputStream);

//------------------------------------------------------------------------------
vtkOutputStream::vtkOutputStream()
{
  this->Stream = nullptr;
}

//------------------------------------------------------------------------------
vtkOutputStream::~vtkOutputStream()
{
  this->SetStream(nullptr);
}

//------------------------------------------------------------------------------
void vtkOutputStream::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Stream: " << (this->Stream ? "set" : "none") << "\n";
}

//------------------------------------------------------------------------------
int vtkOutputStream::StartWriting()
{
  if (!this->Stream)
  {
    vtkErrorMacro("StartWriting() called with no Stream set.");
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkOutputStream::EndWriting()
{
  return 1;
}

//------------------------------------------------------------------------------
int vtkOutputStream::Write(void const* data, size_t length)
{
  return this->WriteStream(static_cast<const char*>(data), length);
}

//------------------------------------------------------------------------------
int vtkOutputStream::WriteStream(const char* data, size_t length)
{
  return (this->Stream->write(data, length) ? 1 : 0);
}
VTK_ABI_NAMESPACE_END
