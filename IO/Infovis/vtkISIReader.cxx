/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkISIReader.cxx

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

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkISIReader.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include <map>
#include <string>

vtkStandardNewMacro(vtkISIReader);

// Not all platforms support std::getline(istream&, std::string) so
// we have to provide our own
static istream& my_getline(istream& input, std::string& output, char delimiter = '\n');

// ----------------------------------------------------------------------

vtkISIReader::vtkISIReader() :
  FileName(0),
  Delimiter(0),
  MaxRecords(0)
{
  this->SetDelimiter(";");

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkISIReader::~vtkISIReader()
{
  this->SetDelimiter(0);
  this->SetFileName(0);
}

// ----------------------------------------------------------------------

void vtkISIReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Delimiter: "
     << (this->Delimiter ? this->Delimiter : "(none)") << endl;
  os << indent << "MaxRecords: " << this->MaxRecords
     << endl;
}

// ----------------------------------------------------------------------

int vtkISIReader::RequestData(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  // Check that the filename has been specified
  if (!this->FileName)
    {
    vtkErrorMacro("vtkISIReader: You must specify a filename!");
    return 0;
    }

  // Open the file
  ifstream file(this->FileName, ios::in | ios::binary);
  if(!file)
    {
    vtkErrorMacro(<< "vtkISIReader could not open file " << this->FileName);
    return 0;
    }

  // Get the total size of the file ...
  file.seekg(0, ios::end);
  const int total_bytes = file.tellg();

  // Go back to the top of the file
  file.seekg(0, ios::beg);

  // Store the text data into a vtkTable
  vtkTable* table = vtkTable::GetData(outputVector);

  // Keep a mapping of column-name to column-index for quick lookups ...
  std::map<std::string, vtkIdType> columns;

  // Get header information from the first two lines of the file ...
  std::string line_buffer;
  my_getline(file, line_buffer);
  if(line_buffer != "FN ISI Export Format")
    {
    vtkErrorMacro(<< "File " << this->FileName << " is not an ISI file");
    return 0;
    }

  my_getline(file, line_buffer);
  if(line_buffer != "VR 1.0")
    {
    vtkErrorMacro(<< "File " << this->FileName << " is not an ISI version 1.0 file");
    return 0;
    }

  const std::string delimiter(this->Delimiter ? this->Delimiter : "");
  int record_count = 0;

  // For each record in the file ...
  for(my_getline(file, line_buffer); file; my_getline(file, line_buffer))
    {
    // Stop if we exceed the maximum number of records ...
    if(this->MaxRecords && record_count >= this->MaxRecords)
      break;

    double progress = total_bytes
      ? static_cast<double>(file.tellg()) / static_cast<double>(total_bytes)
      : 0.5;

    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

    // Add a new row to the table for the record ...
    table->InsertNextBlankRow();

    // For each field in the record ...
    for(; file; )
      {
      const std::string tag_type = line_buffer.size() >= 2 ? line_buffer.substr(0, 2) : std::string();
      if(tag_type == "ER")
        break;
      if(tag_type == "EF")
        break;

      std::string tag_value = line_buffer.size() > 3 ? line_buffer.substr(3) : std::string();

      // For each line in the field ...
      for(my_getline(file, line_buffer); file; my_getline(file, line_buffer))
        {
        const std::string next_tag_type = line_buffer.size() >= 2 ? line_buffer.substr(0, 2) : std::string();
        if(next_tag_type != "  ")
          break;

        const std::string next_tag_value = line_buffer.size() > 3 ? line_buffer.substr(3) : std::string();

        tag_value += delimiter + next_tag_value;
        }

      // If necessary, add a new column to the table to store this value ...
      if(!columns.count(tag_type))
        {
        vtkStringArray* const new_column = vtkStringArray::New();
        new_column->SetName(tag_type.c_str());
        new_column->SetNumberOfTuples(record_count + 1);
        columns[tag_type] = table->GetNumberOfColumns();
        table->AddColumn(new_column);
        new_column->Delete();
        }

      // Set the table value ...
      table->SetValue(record_count, columns[tag_type], tag_value.c_str());
      }

    // Keep track of the current record count ...
    ++record_count;
    }

  return 1;
}

// ----------------------------------------------------------------------

static istream& my_getline(istream& input, std::string& output, char delimiter)
{
  output = "";

  unsigned int numCharactersRead = 0;
  int nextValue = 0;

  while ((nextValue = input.get()) != EOF &&
         numCharactersRead < output.max_size())
    {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast == delimiter || (delimiter == '\n' && downcast == '\r'))
      {
      if (delimiter == '\n' && downcast == '\r' && input.peek() == '\n')
        {
        input.get();
        }
      return input;
      }
    else
      {
      output += downcast;
      }
    }

  return input;
}
