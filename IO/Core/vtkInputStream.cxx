// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInputStream.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkInputStream);

//------------------------------------------------------------------------------
vtkInputStream::vtkInputStream()
{
  this->Stream = nullptr;
}

//------------------------------------------------------------------------------
vtkInputStream::~vtkInputStream()
{
  this->SetStream(nullptr);
}

//------------------------------------------------------------------------------
void vtkInputStream::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Stream: " << (this->Stream ? "set" : "none") << "\n";
}

//------------------------------------------------------------------------------
void vtkInputStream::StartReading()
{
  if (!this->Stream)
  {
    vtkErrorMacro("StartReading() called with no Stream set.");
    return;
  }
  this->StreamStartPosition = this->Stream->tellg();
}

//------------------------------------------------------------------------------
void vtkInputStream::EndReading() {}

//------------------------------------------------------------------------------
int vtkInputStream::Seek(vtkTypeInt64 offset)
{
  std::streamoff off = static_cast<std::streamoff>(this->StreamStartPosition + offset);
  return (this->Stream->seekg(off, std::ios::beg) ? 1 : 0);
}

//------------------------------------------------------------------------------
size_t vtkInputStream::Read(void* data, size_t length)
{
  return this->ReadStream(static_cast<char*>(data), length);
}

//------------------------------------------------------------------------------
size_t vtkInputStream::ReadStream(char* data, size_t length)
{
  this->Stream->read(data, length);
  return this->Stream->gcount();
}
VTK_ABI_NAMESPACE_END
