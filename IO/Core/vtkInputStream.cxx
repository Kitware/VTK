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
int vtkInputStream::Seek(unsigned long offset)
{
  return (this->Stream->seekg(this->StreamStartPosition+offset)? 1:0);
}

//----------------------------------------------------------------------------
unsigned long vtkInputStream::Read(unsigned char* data, unsigned long length)
{
  return this->Read(reinterpret_cast<char*>(data), length);
}

//----------------------------------------------------------------------------
unsigned long vtkInputStream::Read(char* data, unsigned long length)
{
  this->Stream->read(data, length);
  return this->Stream->gcount();
}
