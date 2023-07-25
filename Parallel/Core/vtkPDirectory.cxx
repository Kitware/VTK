// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPDirectory.h"

#include "vtkDirectory.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPDirectory);

//------------------------------------------------------------------------------
vtkPDirectory::vtkPDirectory()
{
  this->Files = vtkStringArray::New();
}

//------------------------------------------------------------------------------
vtkPDirectory::~vtkPDirectory()
{
  this->Files->Delete();
  this->Files = nullptr;
}

//------------------------------------------------------------------------------
bool vtkPDirectory::Load(const std::string& name)
{
  this->Clear();
  this->Path = name;
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller == nullptr || controller->GetLocalProcessId() == 0)
  {
    vtkNew<vtkDirectory> directory;
    int status = directory->Open(name.c_str());
    this->Files->DeepCopy(directory->GetFiles());
    if (controller)
    {
      controller->Broadcast(&status, 1, 0);

      vtkMultiProcessStream stream;
      stream << this->Files->GetNumberOfValues();
      for (vtkIdType cc = 0, max = this->Files->GetNumberOfValues(); cc < max; ++cc)
      {
        stream << this->Files->GetValue(cc);
      }
      controller->Broadcast(stream, 0);
    }
    return status != 0;
  }
  else
  {
    int status;
    controller->Broadcast(&status, 1, 0);
    vtkMultiProcessStream stream;
    controller->Broadcast(stream, 0);
    vtkIdType count;
    stream >> count;
    this->Files->SetNumberOfValues(count);
    for (vtkIdType cc = 0; cc < count; ++cc)
    {
      std::string value;
      stream >> value;
      this->Files->SetValue(cc, value);
    }
    return status != 0;
  }
}

//------------------------------------------------------------------------------
int vtkPDirectory::Open(const char* dir)
{
  this->Clear();
  return (dir != nullptr && this->Load(dir)) ? 1 : 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkPDirectory::GetNumberOfFiles() const
{
  return this->Files->GetNumberOfTuples();
}

//------------------------------------------------------------------------------
const char* vtkPDirectory::GetFile(vtkIdType index) const
{
  if (index >= this->Files->GetNumberOfTuples())
  {
    return nullptr;
  }
  return this->Files->GetValue(index).c_str();
}

//------------------------------------------------------------------------------
int vtkPDirectory::FileIsDirectory(const char* name)
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller == nullptr || controller->GetLocalProcessId() == 0)
  {
    vtkNew<vtkDirectory> directory;
    directory->Open(this->Path.c_str());
    int result = directory->FileIsDirectory(name);
    if (controller)
    {
      controller->Broadcast(&result, 1, 0);
    }
    return result;
  }
  else
  {
    int result;
    controller->Broadcast(&result, 1, 0);
    return result;
  }
}

//------------------------------------------------------------------------------
const char* vtkPDirectory::GetPath() const
{
  return this->Path.c_str();
}

//------------------------------------------------------------------------------
void vtkPDirectory::Clear()
{
  this->Path.clear();
  this->Files->Reset();
}

//------------------------------------------------------------------------------
const char* vtkPDirectory::GetCurrentWorkingDirectory(char* buf, unsigned int len)
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller == nullptr || controller->GetLocalProcessId() == 0)
  {
    auto cwd = vtkDirectory::GetCurrentWorkingDirectory(buf, len);
    if (controller)
    {
      int error = (cwd == nullptr) ? 1 : 0;
      controller->Broadcast(&error, 1, 0);
      controller->Broadcast(buf, len, 0);
    }
    return cwd;
  }
  else
  {
    int error = 0;
    controller->Broadcast(&error, 1, 0);
    controller->Broadcast(buf, len, 0);
    return error ? nullptr : buf;
  }
}

//------------------------------------------------------------------------------
int vtkPDirectory::MakeDirectory(const char* dir)
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller == nullptr || controller->GetLocalProcessId() == 0)
  {
    int status = vtkDirectory::MakeDirectory(dir);
    if (controller)
    {
      controller->Broadcast(&status, 1, 0);
    }
    return status;
  }
  else
  {
    int status;
    controller->Broadcast(&status, 1, 0);
    return status;
  }
}

//------------------------------------------------------------------------------
int vtkPDirectory::DeleteDirectory(const char* dir)
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller == nullptr || controller->GetLocalProcessId() == 0)
  {
    int status = vtkDirectory::DeleteDirectory(dir);
    if (controller)
    {
      controller->Broadcast(&status, 1, 0);
    }
    return status;
  }
  else
  {
    int status;
    controller->Broadcast(&status, 1, 0);
    return status;
  }
}

//------------------------------------------------------------------------------
int vtkPDirectory::Rename(const char* oldname, const char* newname)
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller == nullptr || controller->GetLocalProcessId() == 0)
  {
    int status = vtkDirectory::Rename(oldname, newname);
    if (controller)
    {
      controller->Broadcast(&status, 1, 0);
    }
    return status;
  }
  else
  {
    int status;
    controller->Broadcast(&status, 1, 0);
    return status;
  }
}

//------------------------------------------------------------------------------
void vtkPDirectory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Files:  (" << this->Files << ")\n";
  if (this->Path.empty())
  {
    os << indent << "Directory not open\n";
    return;
  }

  os << indent << "Directory for: " << this->Path << "\n";
  os << indent << "Contains the following files:\n";
  indent = indent.GetNextIndent();
  for (int i = 0; i < this->Files->GetNumberOfValues(); i++)
  {
    os << indent << this->Files->GetValue(i) << "\n";
  }
}
VTK_ABI_NAMESPACE_END
