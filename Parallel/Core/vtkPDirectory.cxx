/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDirectory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkPDirectory.h"

#include <vtkMultiProcessController.h>
#include "vtkObjectFactory.h"
#include "vtkPSystemTools.h"
#include "vtkStringArray.h"
#include <sys/stat.h>
#include <vtksys/Directory.hxx>
#include <string>

vtkStandardNewMacro(vtkPDirectory);

//----------------------------------------------------------------------------
vtkPDirectory::vtkPDirectory()
{
  this->Files = vtkStringArray::New();
}

//----------------------------------------------------------------------------
vtkPDirectory::~vtkPDirectory()
{
  this->Files->Delete();
  this->Files = 0;
}

//----------------------------------------------------------------------------
bool vtkPDirectory::Load(const std::string& name)
{
  this->Clear();

  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();

  long numFiles = 0;
  if(controller->GetLocalProcessId() == 0)
  {
    vtksys::Directory dir;
    if (dir.Load(name) == false)
    {
      numFiles = -1; // failure
      controller->Broadcast(&numFiles, 1, 0);
      return false;
    }

    for(unsigned long i=0;i<dir.GetNumberOfFiles();i++)
    {
      this->Files->InsertNextValue(dir.GetFile(i));
    }
    numFiles = static_cast<long>(dir.GetNumberOfFiles());
    controller->Broadcast(&numFiles, 1, 0);
    for(long i=0;i<numFiles;i++)
    {
      vtkPSystemTools::BroadcastString(this->Files->GetValue(i), 0);
    }
  }
  else
  {
    controller->Broadcast(&numFiles, 1, 0);
    if(numFiles == -1)
    {
      return false;
    }
    for(long i=0;i<numFiles;i++)
    {
      std::string str;
      vtkPSystemTools::BroadcastString(str, 0);
      this->Files->InsertNextValue(str);
    }
  }

  this->Path = name;
  return true;
}

//----------------------------------------------------------------------------
int vtkPDirectory::Open(const char* name)
{
  return static_cast<int>(this->Load(name));
}

//----------------------------------------------------------------------------
vtkIdType vtkPDirectory::GetNumberOfFiles() const
{
  return this->Files->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
const char* vtkPDirectory::GetFile(vtkIdType index) const
{
  if ( index >= this->Files->GetNumberOfTuples() )
  {
    return NULL;
  }
  return this->Files->GetValue(index).c_str();
}

//----------------------------------------------------------------------------
int vtkPDirectory::FileIsDirectory(const char *name)
{
  // The vtksys::SystemTools::FileIsDirectory()
  // does not equal the following code (it probably should),
  // and it will broke KWWidgets. Reverse back to 1.30
  // return vtksys::SystemTools::FileIsDirectory(name);

  if (name == 0)
  {
    return 0;
  }

  int result = 0;
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();

  if(controller->GetLocalProcessId() == 0)
  {
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

    char *fullPath;

    int n = 0;
    if (!absolutePath && !this->Path.empty())
    {
      n = static_cast<int>(this->Path.size());
    }

    int m = static_cast<int>(strlen(name));

    fullPath = new char[n+m+2];

    if (!absolutePath && !this->Path.empty())
    {
      strcpy(fullPath, this->Path.c_str());
#if defined(_WIN32)
      if (fullPath[n-1] != '/'
          && fullPath[n-1] != '\\')
      {
#if !defined(__CYGWIN__)
        fullPath[n++] = '\\';
#else
        fullPath[n++] = '/';
#endif
      }
#else
      if (fullPath[n-1] != '/')
      {
        fullPath[n++] = '/';
      }
#endif
    }

    strcpy(&fullPath[n], name);

    struct stat fs;
    if(stat(fullPath, &fs) == 0)
    {
#if defined(_WIN32)
      result = ((fs.st_mode & _S_IFDIR) != 0);
#else
      result = S_ISDIR(fs.st_mode);
#endif
    }

    delete [] fullPath;
  }

  controller->Broadcast(&result, 1, 0);

  return result;
}

//----------------------------------------------------------------------------
const char* vtkPDirectory::GetPath() const
{
  return this->Path.c_str();
}

//----------------------------------------------------------------------------
void vtkPDirectory::Clear()
{
  this->Path.clear();
  this->Files->Reset();
}

//----------------------------------------------------------------------------
void vtkPDirectory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Files:  (" << this->Files << ")\n";
  if(this->Path.empty())
  {
    os << indent << "Directory not open\n";
    return;
  }

  os << indent << "Directory for: " <<  this->Path << "\n";
  os << indent << "Contains the following files:\n";
  indent = indent.GetNextIndent();
  for(int i = 0; i < this->Files->GetNumberOfValues(); i++)
  {
    os << indent << this->Files->GetValue(i) << "\n";
  }
}
