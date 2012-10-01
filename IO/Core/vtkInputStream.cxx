/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInputStream.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInputStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkInputStream);

//----------------------------------------------------------------------------
vtkInputStream::vtkInputStream()
{
  this->Stream = 0;
}

//----------------------------------------------------------------------------
vtkInputStream::~vtkInputStream()
{
  this->SetStream(0);
}

//----------------------------------------------------------------------------
void vtkInputStream::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Stream: " << (this->Stream? "set" : "none") << "\n";
}

//----------------------------------------------------------------------------
void vtkInputStream::StartReading()
{
  if(!this->Stream)
    {
    vtkErrorMacro("StartReading() called with no Stream set.");
    }
  this->StreamStartPosition = this->Stream->tellg();
}

//----------------------------------------------------------------------------
void vtkInputStream::EndReading()
{
}

//----------------------------------------------------------------------------
int vtkInputStream::Seek(vtkTypeInt64 offset)
{
  std::streamoff off =
    static_cast<std::streamoff>(this->StreamStartPosition+offset);
  return (this->Stream->seekg(off, std::ios::beg)? 1:0);
}

//----------------------------------------------------------------------------
size_t vtkInputStream::Read(void* data, size_t length)
{
  return this->ReadStream(static_cast<char*>(data), length);
}

//----------------------------------------------------------------------------
size_t vtkInputStream::ReadStream(char* data, size_t length)
{
  this->Stream->read(data, length);
  return this->Stream->gcount();
}
