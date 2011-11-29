/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporterWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkX3DExporterWriter.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkX3DExporterWriter::vtkX3DExporterWriter()
{
  this->WriteToOutputString = 0;
  this->OutputString = NULL;
  this->OutputStringLength = 0;
}

//----------------------------------------------------------------------------
vtkX3DExporterWriter::~vtkX3DExporterWriter()
{
  if(this->OutputString)
    {
    delete[] this->OutputString;
    this->OutputString = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporterWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WriteToOutputString: "
     << (this->WriteToOutputString ? "On" : "Off") << std::endl;
  os << indent << "OutputStringLength: " << this->OutputStringLength << std::endl;
  if (this->OutputString)
    {
    os << indent << "OutputString: " << this->OutputString << std::endl;
    }
}

//----------------------------------------------------------------------------
char *vtkX3DExporterWriter::RegisterAndGetOutputString()
{
  char *tmp = this->OutputString;

  this->OutputString = NULL;
  this->OutputStringLength = 0;

  return tmp;
}
