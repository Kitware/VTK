// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkDelimitedTextReader.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataSetAttributes.h"
#include "vtkDelimitedTextCodecIteratorPrivate.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include "vtkTextCodec.h"
#include "vtkTextCodecFactory.h"
#include "vtksys/FStream.hxx"

#include <iostream>
#include <sstream>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkDelimitedTextReader);

//------------------------------------------------------------------------------
vtkDelimitedTextReader::vtkDelimitedTextReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->SetPedigreeIdArrayName("id");
  this->SetFieldDelimiterCharacters(",");
}

//------------------------------------------------------------------------------
vtkDelimitedTextReader::~vtkDelimitedTextReader()
{
  this->SetPedigreeIdArrayName(nullptr);
  this->SetUnicodeCharacterSet(nullptr);
  this->SetFileName(nullptr);
  this->SetInputString(nullptr);
  this->SetFieldDelimiterCharacters(nullptr);
}

//------------------------------------------------------------------------------
void vtkDelimitedTextReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "ReadFromInputString: " << (this->ReadFromInputString ? "On\n" : "Off\n");
  if (this->InputString)
  {
    os << indent << "Input String: " << this->InputString << "\n";
  }
  else
  {
    os << indent << "Input String: (None)\n";
  }
  os << indent << "UnicodeCharacterSet: "
     << (this->UnicodeCharacterSet ? this->UnicodeCharacterSet : "(none)") << endl;
  os << indent << "SkippedRecords: " << this->SkippedRecords << endl;
  os << indent << "MaxRecords: " << this->MaxRecords << endl;
  os << indent << "UnicodeRecordDelimiters: '" << this->UnicodeRecordDelimiters << "'" << endl;
  os << indent << "UnicodeFieldDelimiters: '" << this->UnicodeFieldDelimiters << "'" << endl;
  os << indent << "UnicodeStringDelimiters: '" << this->UnicodeStringDelimiters << "'" << endl;
  os << indent << "StringDelimiter: " << this->StringDelimiter << endl;
  os << indent << "ReplacementCharacter: " << this->ReplacementCharacter << endl;
  os << indent << "FieldDelimiterCharacters: "
     << (this->FieldDelimiterCharacters ? this->FieldDelimiterCharacters : "(none)") << endl;
  os << indent << "CommentCharacters: " << this->CommentCharacters << endl;
  os << indent << "HaveHeaders: " << (this->HaveHeaders ? "true" : "false") << endl;
  os << indent
     << "MergeConsecutiveDelimiters: " << (this->MergeConsecutiveDelimiters ? "true" : "false")
     << endl;
  os << indent << "UseStringDelimiter: " << (this->UseStringDelimiter ? "true" : "false") << endl;
  os << indent << "DetectNumericColumns: " << (this->DetectNumericColumns ? "true" : "false")
     << endl;
  os << indent << "ForceDouble: " << (this->ForceDouble ? "true" : "false") << endl;
  os << indent << "DefaultIntegerValue: " << this->DefaultIntegerValue << endl;
  os << indent << "DefaultDoubleValue: " << this->DefaultDoubleValue << endl;
  os << indent << "TrimWhitespacePriorToNumericConversion: "
     << (this->TrimWhitespacePriorToNumericConversion ? "true" : "false") << endl;
  os << indent << "GeneratePedigreeIds: " << this->GeneratePedigreeIds << endl;
  os << indent << "PedigreeIdArrayName: " << this->PedigreeIdArrayName << endl;
  os << indent << "OutputPedigreeIds: " << (this->OutputPedigreeIds ? "true" : "false") << endl;
  os << indent << "AddTabFieldDelimiter: " << (this->AddTabFieldDelimiter ? "true" : "false")
     << endl;
}

//------------------------------------------------------------------------------
void vtkDelimitedTextReader::SetInputString(const char* in)
{
  int len = 0;
  if (in != nullptr)
  {
    len = static_cast<int>(strlen(in));
  }
  this->SetInputString(in, len);
}

//------------------------------------------------------------------------------
void vtkDelimitedTextReader::SetInputString(const char* in, int len)
{
  if (this->InputString && in && strncmp(in, this->InputString, len) == 0)
  {
    return;
  }

  delete[] this->InputString;

  if (in && len > 0)
  {
    // Add a nullptr terminator so that GetInputString
    // callers (from wrapped languages) get a valid
    // C string in *ALL* cases...
    //
    this->InputString = new char[len + 1];
    memcpy(this->InputString, in, len);
    this->InputString[len] = 0;
    this->InputStringLength = len;
  }
  else
  {
    this->InputString = nullptr;
    this->InputStringLength = 0;
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDelimitedTextReader::SetUTF8RecordDelimiters(const char* delimiters)
{
  this->UnicodeRecordDelimiters = delimiters;
  this->Modified();
}

//------------------------------------------------------------------------------
const char* vtkDelimitedTextReader::GetUTF8RecordDelimiters()
{
  return this->UnicodeRecordDelimiters.c_str();
}

//------------------------------------------------------------------------------
void vtkDelimitedTextReader::SetUTF8FieldDelimiters(const char* delimiters)
{
  this->UnicodeFieldDelimiters = delimiters;
  this->Modified();
}

//------------------------------------------------------------------------------
const char* vtkDelimitedTextReader::GetUTF8FieldDelimiters()
{
  return this->UnicodeFieldDelimiters.c_str();
}

//------------------------------------------------------------------------------
void vtkDelimitedTextReader::SetUTF8StringDelimiters(const char* delimiters)
{
  this->UnicodeStringDelimiters = delimiters;
  this->Modified();
}

//------------------------------------------------------------------------------
const char* vtkDelimitedTextReader::GetUTF8StringDelimiters()
{
  return this->UnicodeStringDelimiters.c_str();
}

//------------------------------------------------------------------------------
vtkStdString vtkDelimitedTextReader::GetLastError()
{
  return this->LastError;
}

//------------------------------------------------------------------------------
int vtkDelimitedTextReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  this->Preview.clear();
  if (this->PreviewNumberOfLines == 0)
  {
    return 1;
  }

  std::unique_ptr<std::istream> input_stream(this->OpenStream());
  if (!input_stream)
  {
    vtkWarningMacro("Unable to open file, RequestInformation aborted.");
    return 1;
  }

  std::string line;
  for (int indx = 0; indx < this->PreviewNumberOfLines; indx++)
  {
    if (!vtksys::SystemTools::GetLineFromStream(*input_stream, line))
    {
      break;
    }
    this->Preview += line + "\r\n";
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkDelimitedTextReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkTable* const output_table = vtkTable::GetData(outputVector);

  // This reader always retrieves a single piece. It ignore request on
  // additional ones.
  vtkInformation* const outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) &&
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 1;
  }

  return this->ReadData(output_table);
}

//------------------------------------------------------------------------------
std::unique_ptr<std::istream> vtkDelimitedTextReader::OpenStream()
{
  if (!this->ReadFromInputString)
  {
    if (!this->FileName)
    {
      vtkErrorMacro("No Filename provided, aborting.");
      return nullptr;
    }
    std::unique_ptr<std::istream> file_stream{ new vtksys::ifstream(this->FileName, ios::binary) };

    if (!file_stream->good())
    {
      vtkErrorMacro("Unable to open input file " + std::string(this->FileName));
      return nullptr;
    }

    return file_stream;
  }
  else
  {
    if (!this->InputString)
    {
      vtkErrorMacro("Empty input string, aborting.");
      return nullptr;
    }
    return std::unique_ptr<std::istream>{ new std::istringstream(this->InputString) };
  }
}

//------------------------------------------------------------------------------
vtkTextCodec* vtkDelimitedTextReader::CreateTextCodec(std::istream* input_stream)
{
  if (this->UnicodeCharacterSet)
  {
    return vtkTextCodecFactory::CodecForName(this->UnicodeCharacterSet);
  }
  else
  {
    return vtkTextCodecFactory::CodecToHandle(*input_stream);
  }
}

//------------------------------------------------------------------------------
void vtkDelimitedTextReader::ReadBOM(std::istream* stream)
{
  namespace vtkfs = vtksys::FStream;
  vtkfs::BOM fBOM = vtkfs::ReadBOM(*stream);

  if (!this->UnicodeCharacterSet)
  {
    switch (fBOM)
    {
      case vtkfs::BOM_UTF8:
        this->UnicodeCharacterSet = new char[6];
        strcpy(this->UnicodeCharacterSet, "UTF-8");
        break;
      case vtkfs::BOM_UTF16BE:
        this->UnicodeCharacterSet = new char[9];
        strcpy(this->UnicodeCharacterSet, "UTF-16BE");
        break;
      case vtkfs::BOM_UTF16LE:
        this->UnicodeCharacterSet = new char[9];
        strcpy(this->UnicodeCharacterSet, "UTF-16LE");
        break;
      default:
        break;
    }
  }
}

//------------------------------------------------------------------------------
int vtkDelimitedTextReader::ReadData(vtkTable* output_table)
{
  this->LastError = "";

  if (!this->PedigreeIdArrayName)
  {
    vtkErrorMacro("You must specify a pedigree id array name");
    return 1;
  }

  if (!this->ReadFromInputString && !this->FileName)
  {
    vtkWarningMacro("Cannot read from file without a file name set. Nothing read.");
    return 1;
  }

  std::unique_ptr<std::istream> input_stream = this->OpenStream();
  if (!input_stream)
  {
    vtkWarningMacro("Unable to open file, ReadData aborted.");
    return 1;
  }

  this->ReadBOM(input_stream.get());

  auto transCodec = vtkSmartPointer<vtkTextCodec>::Take(this->CreateTextCodec(input_stream.get()));

  char tstring[2];
  tstring[1] = '\0';
  tstring[0] = this->StringDelimiter;
  // don't use Set* methods since they change the MTime in
  // RequestData() !!!!!
  std::string fieldDelimiterCharacters = this->FieldDelimiterCharacters;
  if (this->AddTabFieldDelimiter)
  {
    fieldDelimiterCharacters.push_back('\t');
  }
  this->UnicodeFieldDelimiters = fieldDelimiterCharacters;
  this->UnicodeStringDelimiters = tstring;

  if (nullptr == transCodec)
  {
    // should this use the locale instead??
    return 1;
  }

  try
  {
    vtkDelimitedTextCodecIteratorPrivate iterator(this->SkippedRecords, this->MaxRecords,
      this->UnicodeRecordDelimiters, this->UnicodeFieldDelimiters, this->UnicodeStringDelimiters,
      this->UnicodeWhitespace, this->CommentCharacters, this->UnicodeEscapeCharacter,
      this->HaveHeaders, this->MergeConsecutiveDelimiters, this->UseStringDelimiter,
      this->DetectNumericColumns, this->ForceDouble, this->DefaultIntegerValue,
      this->DefaultDoubleValue, output_table);

    transCodec->ToUnicode(*input_stream, iterator);
    iterator.ReachedEndOfInput();

    if (this->OutputPedigreeIds)
    {
      if (this->GeneratePedigreeIds)
      {
        vtkSmartPointer<vtkIdTypeArray> pedigreeIds = vtkSmartPointer<vtkIdTypeArray>::New();
        vtkIdType numRows = output_table->GetNumberOfRows();
        pedigreeIds->SetNumberOfTuples(numRows);
        pedigreeIds->SetName(this->PedigreeIdArrayName);
        for (vtkIdType i = 0; i < numRows; ++i)
        {
          pedigreeIds->InsertValue(i, i);
        }
        output_table->GetRowData()->SetPedigreeIds(pedigreeIds);
      }
      else
      {
        vtkAbstractArray* arr = output_table->GetColumnByName(this->PedigreeIdArrayName);
        if (arr)
        {
          output_table->GetRowData()->SetPedigreeIds(arr);
        }
        else
        {
          throw std::runtime_error(
            "Could not find pedigree id array: " + std::string(this->PedigreeIdArrayName));
        }
      }
    }
  }
  catch (std::exception& e)
  {
    vtkErrorMacro(<< "caught exception: " << e.what() << endl);
    this->LastError = e.what();
    output_table->Initialize();
  }
  catch (...)
  {
    vtkErrorMacro(<< "caught unknown exception." << endl);
    this->LastError = "Unknown exception.";
    output_table->Initialize();
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
