/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRISReader.cxx

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
#include "vtkRISReader.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include <vtkstd/map>
#include <vtkstd/string>

vtkStandardNewMacro(vtkRISReader);

// Not all platforms support vtkstd::getline(istream&, vtkstd::string) so
// we have to provide our own
static istream& my_getline(istream& input, vtkstd::string& output, char delimiter = '\n');

// ----------------------------------------------------------------------

vtkRISReader::vtkRISReader() :
  FileName(0),
  Delimiter(0),
  MaxRecords(0)
{
  this->SetDelimiter(";");

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkRISReader::~vtkRISReader()
{
  this->SetDelimiter(0);
  this->SetFileName(0);
}

// ----------------------------------------------------------------------

void vtkRISReader::PrintSelf(ostream& os, vtkIndent indent)
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

int vtkRISReader::RequestData(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  // Check that the filename has been specified
  if (!this->FileName)
    {
    vtkErrorMacro("vtkRISReader: You must specify a filename!");
    return 0;
    }

  // Open the file
  ifstream file(this->FileName, ios::in | ios::binary);
  if(!file)
    {
    vtkErrorMacro(<< "vtkRISReader could not open file " << this->FileName);
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
  vtkstd::map<vtkstd::string, vtkIdType> columns;

  vtkstd::string line_buffer;
  
  const vtkstd::string delimiter(this->Delimiter ? this->Delimiter : "");
  int record_count = 0;

  // For each record in the file ...
  for(my_getline(file, line_buffer); file; my_getline(file, line_buffer))
    {
    // Skip empty lines ...
    if(line_buffer.empty())
      continue;
    
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
      const vtkstd::string tag_type = line_buffer.size() >= 6 && line_buffer[2] == ' ' && line_buffer[3] == ' ' && line_buffer[4] == '-' && line_buffer[5] == ' '
        ? line_buffer.substr(0, 2)
        : vtkstd::string();
        
      if(tag_type == "ER")
        break;

      vtkstd::string tag_value = line_buffer.size() > 6 ? line_buffer.substr(6) : vtkstd::string();

      // For each line in the field ...
      for(my_getline(file, line_buffer); file; my_getline(file, line_buffer))
        {
        const vtkstd::string next_tag_type = line_buffer.size() >= 6 && line_buffer[2] == ' ' && line_buffer[3] == ' ' && line_buffer[4] == '-' && line_buffer[5] == ' '
          ? line_buffer.substr(0, 2)
          : vtkstd::string();

        if(next_tag_type == tag_type)
          {
          const vtkstd::string next_tag_value = line_buffer.size() > 6 ? line_buffer.substr(6) : vtkstd::string();
          tag_value += delimiter + next_tag_value;
          }
        else if(next_tag_type.empty())
          {
          tag_value += line_buffer;
          }
        else
          {
          break;
          }
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
/*  
  // Loop through every line in the file ...
  vtkstd::string tag;
  vtkstd::string tag_type;
  vtkStdString tag_value;
  bool explicit_tag;
  for(my_getline(file, line_buffer); file; my_getline(file, line_buffer))
    {
    if(this->MaxRecords && record_count >= this->MaxRecords)
      break;
    
    // Skip empty lines ...
    if(line_buffer.empty())
      continue;
    
    double progress = total_bytes
      ? static_cast<double>(file.tellg()) / static_cast<double>(total_bytes)
      : 0.5;
      
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
    
    // Try to extract the tag, tag type, and tag value ...
    if(line_buffer.size() >= 6 && line_buffer[2] == ' ' && line_buffer[3] == ' ' && line_buffer[4] == '-' && line_buffer[5] == ' ')
      {
      explicit_tag = true;
      tag_type = line_buffer.substr(0, 2);
      tag_value = line_buffer.substr(6);
      }
    else
      {
      explicit_tag = false;
      tag_value = line_buffer;
      }

    // If this is the end of a record ...
    if(tag_type == "ER")
      {
      ++record_count;
      continue;
      }

    // If necessary, add a new row to the table to store this value ...
    while(table->GetNumberOfRows() < record_count + 1)
      table->InsertNextBlankRow();
    
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
    vtkStdString old_value = table->GetValue(record_count, columns[tag_type]).ToString();
    if(old_value.empty())
      {
      table->SetValue(record_count, columns[tag_type], tag_value);
      }
    else
      {
      if(explicit_tag)
        {
        table->SetValue(record_count, columns[tag_type], (old_value + delimiter + tag_value).c_str());
        }
      else
        {
        table->SetValue(record_count, columns[tag_type], (old_value + " " + tag_value).c_str());
        }
      }
    }
*/

  return 1;
}

// ----------------------------------------------------------------------
static istream& my_getline(istream& input, vtkstd::string& output, char delimiter)
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
