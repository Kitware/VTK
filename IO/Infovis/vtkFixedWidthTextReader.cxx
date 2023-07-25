// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkFixedWidthTextReader.h"
#include "vtkCommand.h"
#include "vtkIOStream.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"
#include "vtksys/FStream.hxx"

#include <algorithm>
#include <fstream>
#include <vector>

#include <cctype>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFixedWidthTextReader);
vtkCxxSetObjectMacro(vtkFixedWidthTextReader, TableErrorObserver, vtkCommand);

// Function body at bottom of file
static int splitString(const std::string& input, unsigned int fieldWidth, bool stripWhitespace,
  std::vector<std::string>& results, bool includeEmpties = true);

// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
static int my_getline(std::istream& stream, std::string& output, char delim = '\n');

//------------------------------------------------------------------------------

vtkFixedWidthTextReader::vtkFixedWidthTextReader()
{
  this->FileName = nullptr;
  this->StripWhiteSpace = false;
  this->HaveHeaders = false;
  this->FieldWidth = 10;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->TableErrorObserver = nullptr;
}

//------------------------------------------------------------------------------

vtkFixedWidthTextReader::~vtkFixedWidthTextReader()
{
  this->SetFileName(nullptr);
  if (this->TableErrorObserver)
  {
    this->TableErrorObserver->Delete();
  }
}

//------------------------------------------------------------------------------

void vtkFixedWidthTextReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Field width: " << this->FieldWidth << endl;
  os << indent << "Strip leading/trailing whitespace: " << (this->StripWhiteSpace ? "Yes" : "No")
     << endl;
  os << indent << "HaveHeaders: " << (this->HaveHeaders ? "Yes" : "No") << endl;
}

//------------------------------------------------------------------------------

int vtkFixedWidthTextReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  int numLines = 0;

  // Check that the filename has been specified
  if (!this->FileName)
  {
    vtkErrorMacro("vtkFixedWidthTextReader: You must specify a filename!");
    return 2;
  }

  vtksys::ifstream infile(this->FileName, ios::in);
  if (!infile || infile.fail())
  {
    vtkErrorMacro(<< "vtkFixedWidthTextReader: Couldn't open file!");
    return 2;
  }

  // The first line of the file might contain the headers, so we want
  // to be a little bit careful about it.  If we don't have headers
  // we'll have to make something up.
  std::vector<std::string> headers;
  std::vector<std::string> firstLineFields;
  std::string firstLine;

  my_getline(infile, firstLine);

  //  vtkDebugMacro(<<"First line of file: " << firstLine);

  if (this->HaveHeaders)
  {
    splitString(firstLine, this->FieldWidth, this->StripWhiteSpace, headers);
  }
  else
  {
    splitString(firstLine, this->FieldWidth, this->StripWhiteSpace, firstLineFields);

    for (unsigned int i = 0; i < firstLineFields.size(); ++i)
    {
      char fieldName[64];
      snprintf(fieldName, sizeof(fieldName), "Field %u", i);
      headers.emplace_back(fieldName);
    }
  }

  vtkTable* table = vtkTable::GetData(outputVector);
  if (this->TableErrorObserver)
  {
    table->AddObserver(vtkCommand::ErrorEvent, this->TableErrorObserver);
  }

  // Now we can create the arrays that will hold the data for each
  // field.
  std::vector<std::string>::const_iterator fieldIter;
  for (fieldIter = headers.begin(); fieldIter != headers.end(); ++fieldIter)
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
    std::vector<std::string>::const_iterator I;
    for (I = firstLineFields.begin(); I != firstLineFields.end(); ++I)
    {
      dataArray->InsertNextValue(vtkVariant(*I));
    }

    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
  }

  // Read the file line-by-line and add it to the table.
  std::string nextLine;
  while (my_getline(infile, nextLine))
  {
    ++numLines;
    if (numLines > 0 && numLines % 100 == 0)
    {
      float numLinesRead = numLines;
      this->InvokeEvent(vtkCommand::ProgressEvent, &numLinesRead);
    }

    vtkDebugMacro(<< "Next line: " << nextLine);
    std::vector<std::string> dataVector;

    // Split string on the delimiters
    splitString(nextLine, this->FieldWidth, this->StripWhiteSpace, dataVector);

    vtkDebugMacro(<< "Split into " << dataVector.size() << " fields");
    // Add data to the output arrays

    // Convert from vector to variant array
    vtkVariantArray* dataArray = vtkVariantArray::New();
    std::vector<std::string>::const_iterator I;
    for (I = dataVector.begin(); I != dataVector.end(); ++I)
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

//------------------------------------------------------------------------------

static int splitString(const std::string& input, unsigned int fieldWidth, bool stripWhitespace,
  std::vector<std::string>& results, bool includeEmpties)
{
  if (input.empty())
  {
    return 0;
  }

  unsigned int thisField = 0;
  std::string thisFieldText;
  std::string parsedField;

  while (thisField * fieldWidth < input.size())
  {
    thisFieldText = input.substr(thisField * fieldWidth, fieldWidth);

    if (stripWhitespace)
    {
      unsigned int startIndex = 0, endIndex = static_cast<unsigned int>(thisFieldText.size()) - 1;
      while (startIndex < thisFieldText.size() &&
        isspace(static_cast<int>(thisFieldText.at(startIndex))))
      {
        ++startIndex;
      }
      while (endIndex > 0 && isspace(static_cast<int>(thisFieldText.at(endIndex))))
      {
        --endIndex;
      }

      if (startIndex <= endIndex)
      {
        parsedField = thisFieldText.substr(startIndex, (endIndex - startIndex) + 1);
      }
      else
      {
        parsedField = std::string();
      }
    }
    else
    {
      parsedField = thisFieldText;
    }
    ++thisField;
    if (!parsedField.empty() || includeEmpties)
    {
      results.push_back(parsedField);
    }
  }

  return static_cast<int>(results.size());
}

//------------------------------------------------------------------------------

static int my_getline(istream& in, std::string& out, char delimiter)
{
  out = std::string();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;

  while ((nextValue = in.get()) != EOF && numCharactersRead < out.max_size())
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
VTK_ABI_NAMESPACE_END
