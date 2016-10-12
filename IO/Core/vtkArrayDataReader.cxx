/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkArrayDataReader.h"

#include "vtkArrayReader.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <sstream>
#include <stdexcept>

vtkStandardNewMacro(vtkArrayDataReader);

vtkArrayDataReader::vtkArrayDataReader() :
  FileName(0)
{
  this->SetNumberOfInputPorts(0);
  this->ReadFromInputString = false;
}

vtkArrayDataReader::~vtkArrayDataReader()
{
  this->SetFileName(0);
}

void vtkArrayDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "InputString: " << this->InputString << endl;
  os << indent << "ReadFromInputString: "
     << (this->ReadFromInputString ? "on" : "off") << endl;
}

void vtkArrayDataReader::SetInputString(const vtkStdString& string)
{
  this->InputString = string;
  this->Modified();
}

vtkStdString vtkArrayDataReader::GetInputString()
{
  return this->InputString;
}

int vtkArrayDataReader::RequestData(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  try
  {
    vtkArrayData* array_data = NULL;
    if(this->ReadFromInputString)
    {
      array_data = this->Read(this->InputString);
    }
    else
    {
      if(!this->FileName)
        throw std::runtime_error("FileName not set.");

      ifstream file(this->FileName, std::ios::binary);

      array_data = this->Read(file);
    }
    if(!array_data)
      throw std::runtime_error("Error reading vtkArrayData.");

    vtkArrayData* const output_array_data = vtkArrayData::GetData(outputVector);
    output_array_data->ShallowCopy(array_data);
    array_data->Delete();

    return 1;
  }
  catch(std::exception& e)
  {
    vtkErrorMacro(<< e.what());
  }

  return 0;
}

vtkArrayData* vtkArrayDataReader::Read(vtkStdString str)
{
  std::istringstream iss(str);
  return vtkArrayDataReader::Read(iss);
}

vtkArrayData* vtkArrayDataReader::Read(istream& stream)
{
  try
  {
    // Read enough of the file header to identify the type ...
    std::string header_string;
    std::getline(stream, header_string);
    std::istringstream header_buffer(header_string);

    std::string header_name;
    vtkIdType header_size;
    header_buffer >> header_name >> header_size;

    if(header_name != "vtkArrayData")
    {
      throw std::runtime_error("Not a vtkArrayData file");
    }
    if(header_size < 0)
    {
      throw std::runtime_error("Invalid number of arrays");
    }
    vtkArrayData* data = vtkArrayData::New();
    for (vtkIdType i = 0; i < header_size; ++i)
    {
      vtkArray* a = vtkArrayReader::Read(stream);
      data->AddArray(a);
      a->Delete();
    }
    return data;
  }
  catch(std::exception& e)
  {
    vtkGenericWarningMacro(<< e.what());
  }

  return 0;
}
