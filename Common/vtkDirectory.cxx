/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDirectory.h"
#include "vtkStringArray.h"

#include "vtkDebugLeaks.h"

#include <sys/stat.h>

vtkCxxRevisionMacro(vtkDirectory, "1.28");

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkDirectory);

//----------------------------------------------------------------------------
vtkDirectory* vtkDirectory::New()
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkDirectory");
#endif    
  return new vtkDirectory;
}

vtkDirectory::vtkDirectory() 
  : Path(0)
{
  this->Files = vtkStringArray::New();
}


void vtkDirectory::CleanUpFilesAndPath()
{
  this->Files->Reset();
  delete [] this->Path;
  this->Path = 0;
}

vtkDirectory::~vtkDirectory() 
{
  this->CleanUpFilesAndPath();
  this->Files->Delete();
  this->Files = 0;
}



void vtkDirectory::PrintSelf(ostream& os, vtkIndent indent)
{ 
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Files:  (" << this->Files << ")\n";
  if(!this->Path)
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

// First microsoft and borland compilers

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW32__)
#include "vtkWindows.h"
#include <io.h>
#include <ctype.h>
#include <direct.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int vtkDirectory::Open(const char* name)
{
  // clean up from any previous open
  this->CleanUpFilesAndPath();

  char* buf=0;
  int n = static_cast<int>(strlen(name));
  if (name[n - 1] == '/') 
    {
    buf = new char[n + 1 + 1];
    sprintf(buf, "%s*", name);
    } 
  else
    {
    buf = new char[n + 2 + 1];
    sprintf(buf, "%s/*", name);
    }
  struct _finddata_t data;      // data of current file
  
  // First count the number of files in the directory
#if _MSC_VER < 1300
  long srchHandle;
#else
  intptr_t srchHandle;
#endif

  srchHandle = _findfirst(buf, &data);

  if (srchHandle == -1)
    {
    _findclose(srchHandle);
    delete[] buf;
    return 0;
    }
  
  delete [] buf;
  
  // Loop through names
  do 
    {
    this->Files->InsertNextValue(data.name);
    }
  while (_findnext(srchHandle, &data) != -1);

  this->Path = strcpy(new char[strlen(name)+1], name);

  return _findclose(srchHandle) != -1;
}

const char* vtkDirectory::GetCurrentWorkingDirectory(char* buf, 
                                                     unsigned int len)
{
  return _getcwd(buf, len);
}

#else

// Now the POSIX style directory access

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

int vtkDirectory::Open(const char* name)
{
  // clean up from any previous open
  this->CleanUpFilesAndPath();

  DIR* dir = opendir(name);

  if (!dir) 
    {
    return 0;
    }

  dirent* d =0;
  
  for (d = readdir(dir); d; d = readdir(dir))
    {
    this->Files->InsertNextValue(d->d_name);
    }
  this->Path = strcpy(new char[strlen(name)+1], name);

  closedir(dir);

  return 1;
}

const char* vtkDirectory::GetCurrentWorkingDirectory(char* buf, 
                                                     unsigned int len)
{
  return getcwd(buf, len);
}

#endif

//----------------------------------------------------------------------------
int vtkDirectory::MakeDirectory(const char* dir)
{
#if defined(_WIN32) && (defined(_MSC_VER) || defined(__BORLANDC__) \
                        || defined(__MINGW32__))
  return _mkdir(dir) == 0;
#else 
  return mkdir(dir, 00777) == 0;
#endif
}


const char* vtkDirectory::GetFile(int index)
{
  if(index >= this->Files->GetNumberOfValues() || index < 0)
    {
    vtkErrorMacro( << "Bad index for GetFile on vtkDirectory\n");
    return 0;
    }
  
  return this->Files->GetValue(index).c_str();
}


int vtkDirectory::GetNumberOfFiles()
{
  return this->Files->GetNumberOfValues();
}

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
# ifdef VTK_WORKAROUND_WINDOWS_MANGLE
#  undef CreateDirectory
int vtkDirectory::CreateDirectoryA(const char* dir)
{
  VTK_LEGACY_REPLACED_BODY(vtkDirectory::CreateDirectory, "VTK 5.0",
                           vtkDirectory::MakeDirectory);
  return vtkDirectory::MakeDirectory(dir);
}
int vtkDirectory::CreateDirectoryW(const char* dir)
{
  VTK_LEGACY_REPLACED_BODY(vtkDirectory::CreateDirectory, "VTK 5.0",
                           vtkDirectory::MakeDirectory);
  return vtkDirectory::MakeDirectory(dir);
}
# endif
int vtkDirectory::CreateDirectory(const char* dir)
{
  VTK_LEGACY_REPLACED_BODY(vtkDirectory::CreateDirectory, "VTK 5.0",
                           vtkDirectory::MakeDirectory);
  return vtkDirectory::MakeDirectory(dir);
}
#endif
