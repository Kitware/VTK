/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedWidthTextReader.cxx

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

#include "vtkFixedWidthTextReader.h"
#include "vtkCommand.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkPointData.h"
#include "vtkInformation.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkIOStream.h"

#include <algorithm>
#include <vector>
#include <fstream>

#include <cctype>

vtkStandardNewMacro(vtkFixedWidthTextReader);
vtkCxxSetObjectMacro(vtkFixedWidthTextReader,TableErrorObserver,vtkCommand);

// Function body at bottom of file
static int splitString(const vtkStdString& input,
                       unsigned int fieldWidth,
                       bool stripWhitespace,
                       std::vector<vtkStdString>& results,
                       bool includeEmpties=true);


// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
static int my_getline(std::istream& stream, vtkStdString &output, char delim='\n');

// ----------------------------------------------------------------------

vtkFixedWidthTextReader::vtkFixedWidthTextReader()
{
  this->FileName = NULL;
  this->StripWhiteSpace = false;
  this->HaveHeaders = false;
  this->FieldWidth = 10;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->TableErrorObserver = NULL;
}

// ----------------------------------------------------------------------

vtkFixedWidthTextReader::~vtkFixedWidthTextReader()
{
  this->SetFileName(0);
  if (this->TableErrorObserver)
  {
    this->TableErrorObserver->Delete();
  }
}

// ----------------------------------------------------------------------

void vtkFixedWidthTextReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Field width: " << this->FieldWidth << endl;
  os << indent << "Strip leading/trailing whitespace: "
     << (this->StripWhiteSpace ? "Yes" : "No") << endl;
  os << indent << "HaveHeaders: "
     << (this->HaveHeaders ? "Yes" : "No") << endl;
}

// ----------------------------------------------------------------------

int vtkFixedWidthTextReader::RequestData(
                                        vtkInformation*,
                                        vtkInformationVector**,
                                        vtkInformationVector* outputVector)
{
  int numLines = 0;

  // Check that the filename has been specified
  if (!this->FileName)
  {
    vtkErrorMacro("vtkFixedWidthTextReader: You must specify a filename!");
    return 2;
  }

  std::ifstream infile(this->FileName, ios::in);
  if (!infile || infile.fail())
  {
    vtkErrorMacro(<<"vtkFixedWidthTextReader: Couldn't open file!");
    return 2;
  }

  // The first line of the file might contain the headers, so we want
  // to be a little bit careful about it.  If we don't have headers
  // we'll have to make something up.
  std::vector<vtkStdString> headers;
  std::vector<vtkStdString> firstLineFields;
  vtkStdString firstLine;

  my_getline(infile, firstLine);

//  vtkDebugMacro(<<"First line of file: " << firstLine.c_str());

  if (this->HaveHeaders)
  {
    splitString(firstLine,
                this->FieldWidth,
                this->StripWhiteSpace,
                headers);
  }
  else
  {
    splitString(firstLine,
                this->FieldWidth,
                this->StripWhiteSpace,
                firstLineFields);

    for (unsigned int i = 0; i < firstLineFields.size(); ++i)
    {
      // I know it's not a great idea to use sprintf.  It's safe right
      // here because an unsigned int will never take up enough
      // characters to fill up this buffer.
      char fieldName[64];
      sprintf(fieldName, "Field %u", i);
      headers.push_back(fieldName);
    }
  }

  vtkTable *table = vtkTable::GetData(outputVector);
  if (this->TableErrorObserver)
  {
    table->AddObserver(vtkCommand::ErrorEvent, this->TableErrorObserver);
  }

  // Now we can create the arrays that will hold the data for each
  // field.
  std::vector<vtkStdString>::const_iterator fieldIter;
  for(fieldIter = headers.begin(); fieldIter != headers.end(); ++fieldIter)
  {
    vtkStringArray* array = vtkStringArray::New();
    array->SetName((*fieldIter).c_str());
    table->AddColumn(array);
    array->Delete();
  }

  // If the first line did not contain headers then we need to add it
  // to the table.
  if (!this->HaveHeaders)
  {
    vtkVariantArray* dataArray = vtkVariantArray::New();
    std::vector<vtkStdString>::const_iterator I;
    for(I = firstLineFields.begin(); I != firstLineFields.end(); ++I)
    {
      dataArray->InsertNextValue(vtkVariant(*I));
    }

    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
  }

  // Read the file line-by-line and add it to the table.
  vtkStdString nextLine;
  while (my_getline(infile, nextLine))
  {
    ++numLines;
    if (numLines > 0 && numLines % 100 == 0)
    {
      float numLinesRead = numLines;
      this->InvokeEvent(vtkCommand::ProgressEvent, &numLinesRead);
    }

    vtkDebugMacro(<<"Next line: " << nextLine.c_str());
    std::vector<vtkStdString> dataVector;

    // Split string on the delimiters
    splitString(nextLine,
                this->FieldWidth,
                this->StripWhiteSpace,
                dataVector);

    vtkDebugMacro(<<"Split into " << dataVector.size() << " fields");
    // Add data to the output arrays

    // Convert from vector to variant array
    vtkVariantArray* dataArray = vtkVariantArray::New();
    std::vector<vtkStdString>::const_iterator I;
    for(I = dataVector.begin(); I != dataVector.end(); ++I)
    {
      dataArray->InsertNextValue(vtkVariant(*I));
    }

    // Pad out any missing columns
    while (dataArray->GetNumberOfTuples() < table->GetNumberOfColumns())
    {
      dataArray->InsertNextValue(vtkVariant());
    }

    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
  }

  infile.close();

  return 1;
}

// ----------------------------------------------------------------------

static int
splitString(const vtkStdString& input,
            unsigned int fieldWidth,
            bool stripWhitespace,
            std::vector<vtkStdString>& results,
            bool includeEmpties)
{
  if (input.size() == 0)
  {
    return 0;
  }

  unsigned int thisField = 0;
  vtkStdString thisFieldText;
  vtkStdString parsedField;


  while (thisField * fieldWidth < input.size())
  {
    thisFieldText = input.substr(thisField*fieldWidth, fieldWidth);

    if (stripWhitespace)
    {
      unsigned int startIndex = 0, endIndex = static_cast<unsigned int>(thisFieldText.size()) - 1;
      while (startIndex < thisFieldText.size() &&
             isspace(static_cast<int>(thisFieldText.at(startIndex))))
      {
        ++startIndex;
      }
      while (endIndex > 0 &&
             isspace(static_cast<int>(thisFieldText.at(endIndex))))
      {
        -- endIndex;
      }

      if (startIndex <= endIndex)
      {
        parsedField =
          thisFieldText.substr(startIndex, (endIndex - startIndex) + 1);
      }
      else
      {
        parsedField = vtkStdString();
      }
    }
    else
    {
      parsedField = thisFieldText;
    }
    ++ thisField;
    if (parsedField.size() > 0 || includeEmpties)
    {
      results.push_back(parsedField);
    }
  }

  return static_cast<int>(results.size());
}

// ----------------------------------------------------------------------

static int
my_getline(istream& in, vtkStdString &out, char delimiter)
{
  out = vtkStdString();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;

  while ((nextValue = in.get()) != EOF &&
         numCharactersRead < out.max_size())
  {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast != delimiter && downcast != 0x0d)
    {
      out += downcast;
    }
    else
    {
      return numCharactersRead;
    }
  }

  return numCharactersRead;
}



