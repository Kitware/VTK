/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelimitedTextReader.cxx

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

#include <vtkCommand.h>
#include <vtkDataSetAttributes.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStdString.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTable.h>
#include <vtkDelimitedTextReader.h>
#include <vtkUnicodeString.h>
#include <vtkUnicodeStringArray.h>
#include <vtkStringArray.h>
#include <vtkStringToNumeric.h>

#include <vtkstd/algorithm>
#include <vtkstd/iterator>
#include <vtkstd/stdexcept>
#include <vtksys/ios/sstream>
#include <vtkstd/set>
#include <vtkstd/vector>

#include <ctype.h>

#include <utf8.h>

/////////////////////////////////////////////////////////////////////////////////////////
// DelimitedTextIterator

/// Output iterator object that parses a stream of Unicode characters into records and
/// fields, inserting them into a vtkTable.

namespace {

class DelimitedTextIterator
{
public:
  typedef vtkstd::forward_iterator_tag iterator_category;
  typedef vtkUnicodeStringValueType value_type;
  typedef vtkstd::string::difference_type difference_type;
  typedef value_type* pointer;
  typedef value_type& reference;

  DelimitedTextIterator(
    const vtkIdType max_records,
    const vtkUnicodeString& record_delimiters,
    const vtkUnicodeString& field_delimiters,
    const vtkUnicodeString& string_delimiters,
    const vtkUnicodeString& whitespace,
    const vtkUnicodeString& escape,
    bool have_headers,
    bool unicode_array_output,
    bool merg_cons_delimiters,
    bool use_string_delimeter,
    vtkTable* const output_table
      ) :
    MaxRecords(max_records),
    MaxRecordIndex(have_headers ? max_records + 1 : max_records),
    RecordDelimiters(record_delimiters.begin(), record_delimiters.end()),
    FieldDelimiters(field_delimiters.begin(), field_delimiters.end()),
    StringDelimiters(string_delimiters.begin(), string_delimiters.end()),
    Whitespace(whitespace.begin(), whitespace.end()),
    EscapeDelimiter(escape.begin(), escape.end()),
    HaveHeaders(have_headers),
    UnicodeArrayOutput(unicode_array_output),
    WhiteSpaceOnlyString(true),
    OutputTable(output_table),
    CurrentRecordIndex(0),
    CurrentFieldIndex(0),
    RecordAdjacent(true),
    MergeConsDelims(merg_cons_delimiters),
    ProcessEscapeSequence(false),
    UseStringDelimiter(use_string_delimeter),
    WithinString(0)
  {
  }

  ~DelimitedTextIterator()
  {
    // Ensure that all table columns have the same length ...
    for(vtkIdType i = 0; i != this->OutputTable->GetNumberOfColumns(); ++i)
      {
      if(this->OutputTable->GetColumn(i)->GetNumberOfTuples() != this->OutputTable->GetColumn(0)->GetNumberOfTuples())
        this->OutputTable->GetColumn(i)->Resize(this->OutputTable->GetColumn(0)->GetNumberOfTuples());
      }
  }

  DelimitedTextIterator& operator++(int)
  {
    return *this;
  }

  DelimitedTextIterator& operator*()
  {
    return *this;
  }

  // Handle windows files that do not have a carriage return line feed on the last line of the file ...
  void ReachedEndOfInput()
  {
    if(this->CurrentField.empty())
      {
      return;
      }
    vtkUnicodeString::value_type value = this->CurrentField[this->CurrentField.character_count()-1];
    if(!this->RecordDelimiters.count(value)&&!this->Whitespace.count(value))
      {
      this->InsertField();
      }
  }

  DelimitedTextIterator& operator=(const DelimitedTextIterator& c)
  {
    if(this != &c)
      {
      this->CurrentField = c.CurrentField;
      this->CurrentFieldIndex = c.CurrentFieldIndex;
      this->CurrentRecordIndex = c.CurrentRecordIndex;
      this->OutputTable = c.OutputTable;
      }
    return *this;
  }

  DelimitedTextIterator& operator=(const vtkUnicodeString::value_type value)
  {
    // If we've already read our maximum number of records, we're done ...
    if(this->MaxRecords && this->CurrentRecordIndex == this->MaxRecordIndex)
      return *this;

    // Strip adjacent record delimiters and whitespace...
    if(this->RecordAdjacent && (this->RecordDelimiters.count(value) || this->Whitespace.count(value)))
      {
      return *this;
      }
    else
      {
      this->RecordAdjacent = false;
      }

    // Look for record delimiters ...
    if(this->RecordDelimiters.count(value))
      {
      this->InsertField();
      this->CurrentRecordIndex += 1;
      this->CurrentFieldIndex = 0;
      this->CurrentField.clear();
      this->RecordAdjacent = true;
      this->WithinString = 0;
      this->WhiteSpaceOnlyString = true;
      return *this;
      }

    // Look for field delimiters unless we're in a string ...
    if(!this->WithinString && this->FieldDelimiters.count(value))
      {
       // Handle special case of merging consective delimiters ...
      if( !(this->CurrentField.empty() && this->MergeConsDelims) )
        {
        this->InsertField();
        this->CurrentFieldIndex += 1;
        this->CurrentField.clear();
        }
      return *this;
      }

    // Check for start of escape sequence ...
    if(!this->ProcessEscapeSequence && this->EscapeDelimiter.count(value))
      {
      this->ProcessEscapeSequence = true;
      return *this;
      }

    // Process escape sequence ...
    if(this->ProcessEscapeSequence)
      {
      vtkUnicodeString curr_char;
      curr_char += value;
      if(curr_char == vtkUnicodeString::from_utf8("0"))
        {
        this->CurrentField += vtkUnicodeString::from_utf8("\0");
        }
      else if(curr_char == vtkUnicodeString::from_utf8("a"))
        {
        this->CurrentField += vtkUnicodeString::from_utf8("\a");
        }
      else if(curr_char == vtkUnicodeString::from_utf8("b"))
        {
        this->CurrentField += vtkUnicodeString::from_utf8("\b");
        }
      else if(curr_char == vtkUnicodeString::from_utf8("t"))
        {
        this->CurrentField += vtkUnicodeString::from_utf8("\t");
        }
      else if(curr_char == vtkUnicodeString::from_utf8("n"))
        {
        this->CurrentField += vtkUnicodeString::from_utf8("\n");
        }
      else if(curr_char == vtkUnicodeString::from_utf8("v"))
        {
        this->CurrentField += vtkUnicodeString::from_utf8("\v");
        }
      else if(curr_char == vtkUnicodeString::from_utf8("f"))
        {
        this->CurrentField += vtkUnicodeString::from_utf8("\f");
        }
      else if(curr_char == vtkUnicodeString::from_utf8("r"))
        {
        this->CurrentField += vtkUnicodeString::from_utf8("\r");
        }
      else if(curr_char == vtkUnicodeString::from_utf8("\\"))
        {
        this->CurrentField += vtkUnicodeString::from_utf8("\\");
        }
      else
        {
        this->CurrentField += value;
        }
      this->ProcessEscapeSequence = false;
      return *this;
      }

    // Start a string ...
    if(!this->WithinString && this->StringDelimiters.count(value) && this->UseStringDelimiter)
      {
      this->WithinString = value;
      return *this;
      }
    
    // End a string ...
    if(this->WithinString && (this->WithinString == value) && this->UseStringDelimiter)
      {
      this->WithinString = 0;
      return *this;
      }

    if(!this->Whitespace.count(value))
     {
     this->WhiteSpaceOnlyString = false;
     }
    // Keep growing the current field ...
    this->CurrentField += value;
    return *this;
  }

private:
  void InsertField()
  {
    if(this->CurrentFieldIndex >= this->OutputTable->GetNumberOfColumns()&& 0 == this->CurrentRecordIndex)
      {
      vtkAbstractArray* array;
      if(this->UnicodeArrayOutput)
        {
        array = vtkUnicodeStringArray::New();
        }
      else
        {
        array = vtkStringArray::New();
        }

      if(this->HaveHeaders)
        {
        array->SetName(this->CurrentField.utf8_str());
        }
      else
        {
        vtkstd::stringstream buffer;
        buffer << "Field " << this->CurrentFieldIndex;
        array->SetName(buffer.str().c_str());
        if(this->UnicodeArrayOutput)
          {
          array->SetNumberOfTuples(this->CurrentRecordIndex + 1);
          vtkUnicodeStringArray::SafeDownCast(array)->SetValue(this->CurrentRecordIndex, this->CurrentField);
          }
        else
          {
          vtkstd::string s;
          this->CurrentField.utf8_str(s);
          vtkStringArray::SafeDownCast(array)->InsertValue(this->CurrentRecordIndex, s);
          }
        }
      this->OutputTable->AddColumn(array);
      array->Delete();
      }
    else if(this->CurrentFieldIndex < this->OutputTable->GetNumberOfColumns())
      {
      // Handle case where input file has header information ...
      vtkIdType rec_index;
      if(this->HaveHeaders)
        {
        rec_index = this->CurrentRecordIndex - 1;
        }
      else
        {
        rec_index = this->CurrentRecordIndex;
        }

      if(this->UnicodeArrayOutput)
        {
        vtkUnicodeStringArray* uarray = vtkUnicodeStringArray::SafeDownCast(this->OutputTable->GetColumn(this->CurrentFieldIndex));
        uarray->SetNumberOfTuples(rec_index + 1);
        uarray->SetValue(rec_index, this->CurrentField);
        }
      else
        {
        vtkStringArray* sarray = vtkStringArray::SafeDownCast(this->OutputTable->GetColumn(this->CurrentFieldIndex));
        vtkstd::string s;
        this->CurrentField.utf8_str(s);
        sarray->InsertValue(rec_index,s);
        }
      }
  }

  vtkIdType MaxRecords;
  vtkIdType MaxRecordIndex;
  vtkstd::set<vtkUnicodeString::value_type> RecordDelimiters;
  vtkstd::set<vtkUnicodeString::value_type> FieldDelimiters;
  vtkstd::set<vtkUnicodeString::value_type> StringDelimiters;
  vtkstd::set<vtkUnicodeString::value_type> Whitespace;
  vtkstd::set<vtkUnicodeString::value_type> EscapeDelimiter;
  bool HaveHeaders;
  bool UnicodeArrayOutput;
  bool WhiteSpaceOnlyString;
  vtkTable* OutputTable;
  vtkIdType CurrentRecordIndex;
  vtkIdType CurrentFieldIndex;
  vtkUnicodeString CurrentField;
  bool RecordAdjacent;
  bool MergeConsDelims;
  bool ProcessEscapeSequence;
  bool UseStringDelimiter;
  vtkUnicodeString::value_type WithinString;
};

/////////////////////////////////////////////////////////////////////////////////////////
// ascii_to_unicode

template<typename OctetIteratorT, typename OutputIteratorT>
void ascii_to_unicode(OctetIteratorT begin, OctetIteratorT end, OutputIteratorT output)
{
  while(begin != end)
    {
    const vtkTypeUInt32 code_point = *begin++;
    if(code_point > 0x7f)
      throw vtkstd::runtime_error("Detected a character that isn't valid US-ASCII.");

    *output++ = code_point;
    }

   output.ReachedEndOfInput();
}

/////////////////////////////////////////////////////////////////////////////////////////
// utf16_to_unicode

template<typename OctetIteratorT, typename OutputIteratorT>
void utf16_to_unicode(const bool big_endian, OctetIteratorT begin, OctetIteratorT end, OutputIteratorT output)
{
  while(begin != end)
    {
    const vtkTypeUInt8 first_byte = *begin++;
    if(begin == end)
      throw vtkstd::runtime_error("Premature end-of-sequence extracting UTF-16 code unit.");
    const vtkTypeUInt8 second_byte = *begin++;

    const vtkTypeUInt32 first_code_unit = big_endian ? first_byte << 8 | second_byte : second_byte << 8 | first_byte;

    if(first_code_unit >= 0xd800 && first_code_unit <= 0xdfff)
      {
      if(begin == end)
        throw vtkstd::runtime_error("Premature end-of-sequence extracting UTF-16 trail surrogate first byte.");
      const vtkTypeUInt8 third_byte = *begin++;
      if(begin == end)
        throw vtkstd::runtime_error("Premature end-of-sequence extracting UTF-16 trail surrogate second byte.");
      const vtkTypeUInt8 fourth_byte = *begin++;

      const vtkTypeUInt32 second_code_unit = big_endian ? third_byte << 8 | fourth_byte : fourth_byte << 8 | third_byte;
      if(second_code_unit >= 0xdc00 && second_code_unit <= 0xdfff)
        {
        *output++ = vtkTypeUInt32 (vtkTypeInt32 (first_code_unit << 10) + vtkTypeInt32 (second_code_unit) + (0x10000 - (0xd800 << 10) - 0xdc00));
        }
      else
        {
        throw vtkstd::runtime_error("Invalid UTF-16 trail surrogate.");
        }
      }
    else
      {
      *output++ = first_code_unit;
      }
    }
    output.ReachedEndOfInput();
}

} // End anonymous namespace

/////////////////////////////////////////////////////////////////////////////////////////
// vtkDelimitedTextReader

vtkStandardNewMacro(vtkDelimitedTextReader);

vtkDelimitedTextReader::vtkDelimitedTextReader() :
  FileName(0),
  UnicodeCharacterSet(0),
  MaxRecords(0),
  UnicodeRecordDelimiters(vtkUnicodeString::from_utf8("\r\n")),
  UnicodeFieldDelimiters(vtkUnicodeString::from_utf8(",")),
  UnicodeStringDelimiters(vtkUnicodeString::from_utf8("\"")),
  UnicodeWhitespace(vtkUnicodeString::from_utf8(" \t\r\n\v\f")),
  UnicodeEscapeCharacter(vtkUnicodeString::from_utf8("\\")),
  HaveHeaders(false)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->MergeConsecutiveDelimiters = false;
  this->PedigreeIdArrayName = NULL;
  this->SetPedigreeIdArrayName("id");
  this->GeneratePedigreeIds = true;
  this->OutputPedigreeIds = false;
  this->UnicodeOutputArrays = false;
  this->FieldDelimiterCharacters = 0;
  this->SetFieldDelimiterCharacters(",");
  this->StringDelimiter='"';
  this->UseStringDelimiter = true;
  this->DetectNumericColumns = false;
}

vtkDelimitedTextReader::~vtkDelimitedTextReader()
{
  this->SetPedigreeIdArrayName(0);
  this->SetUnicodeCharacterSet(0);
  this->SetFileName(0);
  this->SetFieldDelimiterCharacters(0);
}

void vtkDelimitedTextReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " 
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "UnicodeCharacterSet: " 
     << (this->UnicodeCharacterSet ? this->UnicodeCharacterSet : "(none)") << endl;
  os << indent << "MaxRecords: " << this->MaxRecords
     << endl;
  os << indent << "UnicodeRecordDelimiters: '" << this->UnicodeRecordDelimiters.utf8_str()
     << "'" << endl;
  os << indent << "UnicodeFieldDelimiters: '" << this->UnicodeFieldDelimiters.utf8_str()
     << "'" << endl;
  os << indent << "UnicodeStringDelimiters: '" << this->UnicodeStringDelimiters.utf8_str()
     << "'" << endl;
  os << indent << "StringDelimiter: " 
     << this->StringDelimiter << endl;
  os << indent << "FieldDelimiterCharacters: "
     << (this->FieldDelimiterCharacters ? this->FieldDelimiterCharacters : "(none)") << endl;
  os << indent << "HaveHeaders: " 
     << (this->HaveHeaders ? "true" : "false") << endl;
  os << indent << "MergeConsecutiveDelimiters: " 
     << (this->MergeConsecutiveDelimiters ? "true" : "false") << endl;
  os << indent << "UseStringDelimiter: " 
     << (this->UseStringDelimiter ? "true" : "false") << endl;
  os << indent << "DetectNumericColumns: "
    << (this->DetectNumericColumns? "true" : "false") << endl;
  os << indent << "GeneratePedigreeIds: " 
    << this->GeneratePedigreeIds << endl;
  os << indent << "PedigreeIdArrayName: " 
    << this->PedigreeIdArrayName << endl;
  os << indent << "OutputPedigreeIds: "
    << (this->OutputPedigreeIds? "true" : "false") << endl;
}

void vtkDelimitedTextReader::SetUnicodeRecordDelimiters(const vtkUnicodeString& delimiters)
{
  this->UnicodeRecordDelimiters = delimiters;
  this->Modified();
}

vtkUnicodeString vtkDelimitedTextReader::GetUnicodeRecordDelimiters()
{
  return this->UnicodeRecordDelimiters;
}

void vtkDelimitedTextReader::SetUTF8RecordDelimiters(const char* delimiters)
{
  this->UnicodeRecordDelimiters = vtkUnicodeString::from_utf8(delimiters);
  this->Modified();
}

const char* vtkDelimitedTextReader::GetUTF8RecordDelimiters()
{
  return this->UnicodeRecordDelimiters.utf8_str();
}

void vtkDelimitedTextReader::SetUnicodeFieldDelimiters(const vtkUnicodeString& delimiters)
{
  this->UnicodeFieldDelimiters = delimiters;
  this->Modified();
}

vtkUnicodeString vtkDelimitedTextReader::GetUnicodeFieldDelimiters()
{
  return this->UnicodeFieldDelimiters;
}

void vtkDelimitedTextReader::SetUTF8FieldDelimiters(const char* delimiters)
{
  this->UnicodeFieldDelimiters = vtkUnicodeString::from_utf8(delimiters);
  this->Modified();
}

const char* vtkDelimitedTextReader::GetUTF8FieldDelimiters()
{
  return this->UnicodeFieldDelimiters.utf8_str();
}

void vtkDelimitedTextReader::SetUnicodeStringDelimiters(const vtkUnicodeString& delimiters)
{
  this->UnicodeStringDelimiters = delimiters;
  this->Modified();
}

vtkUnicodeString vtkDelimitedTextReader::GetUnicodeStringDelimiters()
{
  return this->UnicodeStringDelimiters;
}

void vtkDelimitedTextReader::SetUTF8StringDelimiters(const char* delimiters)
{
  this->UnicodeStringDelimiters = vtkUnicodeString::from_utf8(delimiters);
  this->Modified();
}

const char* vtkDelimitedTextReader::GetUTF8StringDelimiters()
{
  return this->UnicodeStringDelimiters.utf8_str();
}

vtkStdString vtkDelimitedTextReader::GetLastError()
{
  return this->LastError;
}

int vtkDelimitedTextReader::RequestData(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  vtkTable* const output_table = vtkTable::GetData(outputVector);

  this->LastError = "";

  try
    {
    // We only retrieve one piece ...
    vtkInformation* const outInfo = outputVector->GetInformationObject(0);
    if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) &&
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
      {
      return 1;
      }

    // If the filename hasn't been specified, we're done ...
    if(!this->FileName)
      {
      return 1;
      }

    vtkStdString character_set;

    if(this->UnicodeCharacterSet)
      {
      this->UnicodeOutputArrays = true;
      character_set = this->UnicodeCharacterSet;
      }
    else
      {
      char tstring[2];
      tstring[1] = '\0';
      tstring[0] = this->StringDelimiter;
      this->SetUnicodeFieldDelimiters(vtkUnicodeString::from_utf8(this->FieldDelimiterCharacters));
      this->SetUnicodeStringDelimiters(vtkUnicodeString::from_utf8(tstring));
      this->UnicodeOutputArrays = false;
      }

    if (!this->PedigreeIdArrayName)
      throw vtkstd::runtime_error("You must specify a pedigree id array name");

    // Get the total size of the input file in bytes ... 
    ifstream file_stream(this->FileName, ios::binary);
    if(!file_stream.good())
      throw vtkstd::runtime_error("Unable to open input file " + std::string(this->FileName));
    
    file_stream.seekg(0, ios::end);
    const vtkIdType total_bytes = file_stream.tellg();
    file_stream.seekg(0, ios::beg);

    // Read the file into a buffer ...
    vtkstd::vector<unsigned char> content(total_bytes);
    file_stream.read(reinterpret_cast<char*>(&content[0]), total_bytes);

    DelimitedTextIterator iterator(
      this->MaxRecords,
      this->UnicodeRecordDelimiters,
      this->UnicodeFieldDelimiters,
      this->UnicodeStringDelimiters,
      this->UnicodeWhitespace,
      this->UnicodeEscapeCharacter,
      this->HaveHeaders,
      this->UnicodeOutputArrays,
      this->MergeConsecutiveDelimiters,
      this->UseStringDelimiter,
      output_table);

    if("US-ASCII" == character_set || character_set.empty())
      {
      ascii_to_unicode(content.begin(), content.end(), iterator);
      }
    else if("UTF-8" == character_set)
      {
      if(!vtk_utf8::is_valid(content.begin(), content.end()))
        throw vtkstd::runtime_error("Detected a byte sequence that isn't valid UTF-8.");

      iterator = vtk_utf8::utf8to32(content.begin(),  content.end(), iterator);
      iterator.ReachedEndOfInput();
      }
    else if("UTF-16" == character_set)
      {
      if(content.size() > 1 && static_cast<unsigned char>(content[0]) == 0xfe && static_cast<unsigned char>(content[1]) == 0xff)
        {
        utf16_to_unicode(true, content.begin() + 2, content.end(), iterator);
        }
      else if(content.size() > 1 && static_cast<unsigned char>(content[0]) == 0xff && static_cast<unsigned char>(content[1]) == 0xfe)
        {
        utf16_to_unicode(false, content.begin() + 2, content.end(), iterator);
        }
      else
        {
        throw vtkstd::runtime_error("Cannot detect UTF-16 endianness.  Try 'UTF-16BE' or 'UTF-16LE' instead.");
        }
      }
    else if("UTF-16BE" == character_set)
      {
      if(content.size() > 1 && static_cast<unsigned char>(content[0]) == 0xfe && static_cast<unsigned char>(content[1]) == 0xff)
        {
        throw vtkstd::runtime_error("UTF-16BE file should not contain BOM.  Try 'UTF-16' instead.");
        }
      else if(content.size() > 1 && static_cast<unsigned char>(content[0]) == 0xff && static_cast<unsigned char>(content[1]) == 0xfe)
        {
        throw vtkstd::runtime_error("UTF-16BE file should not contain BOM.  Try 'UTF-16' instead.");
        }
      else
        {
        utf16_to_unicode(true, content.begin(), content.end(), iterator);
        }
      }
    else if("UTF-16LE" == character_set)
      {
      if(content.size() > 1 && static_cast<unsigned char>(content[0]) == 0xfe && static_cast<unsigned char>(content[1]) == 0xff)
        {
        throw vtkstd::runtime_error("UTF-16LE file should not contain BOM.  Try 'UTF-16' instead.");
        }
      else if(content.size() > 1 && static_cast<unsigned char>(content[0]) == 0xff && static_cast<unsigned char>(content[1]) == 0xfe)
        {
        throw vtkstd::runtime_error("UTF-16LE file should not contain BOM.  Try 'UTF-16' instead.");
        }
      else
        {
        utf16_to_unicode(false, content.begin(), content.end(), iterator);
        }
      }
    else
      {
      throw vtkstd::runtime_error("Unknown Unicode character set: " + vtkStdString(this->UnicodeCharacterSet));
      }

    if(this->OutputPedigreeIds)
      {
      if (this->GeneratePedigreeIds)
        {
        vtkSmartPointer<vtkIdTypeArray> pedigreeIds =
          vtkSmartPointer<vtkIdTypeArray>::New();
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
        vtkAbstractArray* arr =
          output_table->GetColumnByName(this->PedigreeIdArrayName);
        if (arr)
          {
          output_table->GetRowData()->SetPedigreeIds(arr);
          }
        else
          {
          throw vtkstd::runtime_error("Could not find pedigree id array: " + vtkStdString(this->PedigreeIdArrayName));
          }
      }
    }

    if (this->DetectNumericColumns && !this->UnicodeOutputArrays)
      {
      vtkStringToNumeric* convertor = vtkStringToNumeric::New();
      vtkTable* clone = output_table->NewInstance();
      clone->ShallowCopy(output_table);
      convertor->SetInput(clone);
      convertor->Update();
      clone->Delete();
      output_table->ShallowCopy(convertor->GetOutputDataObject(0));
      convertor->Delete();
      }

    } 
  catch(vtkstd::exception& e)
    {
    vtkErrorMacro(<< "caught exception: " << e.what() << endl);
    this->LastError = e.what();
    output_table->Initialize();
    }
  catch(...)
    {
    vtkErrorMacro(<< "caught unknown exception." << endl);
    this->LastError = "Unknown exception.";
    output_table->Initialize();
    }

  return 1;
}

