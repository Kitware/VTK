// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDirectory.h"
#include "vtkStringArray.h"

#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"

#include "vtksys/Directory.hxx"
#include "vtksys/Encoding.hxx"
#include "vtksys/SystemTools.hxx"

#include <cstdio>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDirectory);

//------------------------------------------------------------------------------
vtkDirectory::vtkDirectory()
  : Path(nullptr)
{
  this->Files = vtkStringArray::New();
}

//------------------------------------------------------------------------------
void vtkDirectory::CleanUpFilesAndPath()
{
  this->Files->Reset();
  delete[] this->Path;
  this->Path = nullptr;
}

//------------------------------------------------------------------------------
vtkDirectory::~vtkDirectory()
{
  this->CleanUpFilesAndPath();
  this->Files->Delete();
  this->Files = nullptr;
}

//------------------------------------------------------------------------------
void vtkDirectory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Files:  (" << this->Files << ")\n";
  if (!this->Path)
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

//------------------------------------------------------------------------------
int vtkDirectory::Open(const char* name)
{
  // clean up from any previous open
  this->CleanUpFilesAndPath();

  vtksys::Directory dir;

  if (name != nullptr && dir.Load(name).IsSuccess())
  {
    unsigned long numFiles = dir.GetNumberOfFiles();
    for (unsigned long i = 0; i < numFiles; i++)
    {
      this->Files->InsertNextValue(dir.GetFile(i));
    }

    this->Path = new char[strlen(name) + 1];
    strcpy(this->Path, name);
  }

  dir.Clear();

  return (this->Path != nullptr);
}

//------------------------------------------------------------------------------
const char* vtkDirectory::GetCurrentWorkingDirectory(char* buf, unsigned int len)
{
  std::string s = vtksys::SystemTools::GetCurrentWorkingDirectory();
  if (s.length() < len)
  {
    return strncpy(buf, s.c_str(), len);
  }
  return nullptr;
}

//------------------------------------------------------------------------------
int vtkDirectory::MakeDirectory(const char* dir)
{
  return vtksys::SystemTools::MakeDirectory(dir).IsSuccess();
}

//------------------------------------------------------------------------------
const char* vtkDirectory::GetFile(vtkIdType index)
{
  if (index >= this->Files->GetNumberOfValues() || index < 0)
  {
    vtkErrorMacro(<< "Bad index for GetFile on vtkDirectory\n");
    return nullptr;
  }

  return this->Files->GetValue(index).c_str();
}

//------------------------------------------------------------------------------
vtkIdType vtkDirectory::GetNumberOfFiles()
{
  return this->Files->GetNumberOfValues();
}

//------------------------------------------------------------------------------
int vtkDirectory::FileIsDirectory(const char* name)
{
  if (name == nullptr)
  {
    return 0;
  }

  int absolutePath = 0;
#if defined(_WIN32)
  if (name[0] == '/' || name[0] == '\\')
  {
    absolutePath = 1;
  }
  else
  {
    for (int i = 0; name[i] != '\0'; i++)
    {
      if (name[i] == ':')
      {
        absolutePath = 1;
        break;
      }
      else if (name[i] == '/' || name[i] == '\\')
      {
        break;
      }
    }
  }
#else
  if (name[0] == '/')
  {
    absolutePath = 1;
  }
#endif

  char* fullPath;

  int n = 0;
  if (!absolutePath && this->Path)
  {
    n = static_cast<int>(strlen(this->Path));
  }

  int m = static_cast<int>(strlen(name));

  fullPath = new char[n + m + 2];

  if (!absolutePath && this->Path)
  {
    strcpy(fullPath, this->Path);
#if defined(_WIN32)
    if (fullPath[n - 1] != '/' && fullPath[n - 1] != '\\')
    {
#if !defined(__CYGWIN__)
      fullPath[n++] = '\\';
#else
      fullPath[n++] = '/';
#endif
    }
#else
    if (fullPath[n - 1] != '/')
    {
      fullPath[n++] = '/';
    }
#endif
  }

  strcpy(&fullPath[n], name);

  int result = vtksys::SystemTools::FileIsDirectory(fullPath);

  delete[] fullPath;

  return result;
}

//------------------------------------------------------------------------------
int vtkDirectory::DeleteDirectory(const char* dir)
{
  return vtksys::SystemTools::RemoveADirectory(dir).IsSuccess();
}

//------------------------------------------------------------------------------
int vtkDirectory::Rename(const char* oldname, const char* newname)
{
#if defined(_WIN32)
  // we can't be sure what encoding "rename()" uses, so use "_wrename".
  std::wstring woldname = vtksys::Encoding::ToWide(oldname);
  std::wstring wnewname = vtksys::Encoding::ToWide(newname);
  return (0 == _wrename(woldname.c_str(), wnewname.c_str()));
#else
  return (0 == rename(oldname, newname));
#endif
}

VTK_ABI_NAMESPACE_END
