// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkDelimitedTextReader.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkValueFromString.h"

#include "vtkTextCodec.h"
#include "vtkTextCodecFactory.h"
#include "vtksys/FStream.hxx"

#include <vtk_utf8.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

#include <cctype>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
////////////////////////////////////////////////////////////////////////////////
// DelimitedTextIterator

/// Output iterator object that parses a stream of Unicode characters into records and
/// fields, inserting them into a vtkTable.
class DelimitedTextIterator : public vtkTextCodec::OutputIterator
{
public:
  typedef std::forward_iterator_tag iterator_category;
  typedef vtkTypeUInt32 value_type;
  typedef std::string::difference_type difference_type;
  typedef value_type* pointer;
  typedef value_type& reference;

  DelimitedTextIterator(const vtkIdType max_records, const std::string& record_delimiters,
    const std::string& field_delimiters, const std::string& string_delimiters,
    const std::string& whitespace, const std::string& escape, bool have_headers,
    bool merg_cons_delimiters, bool use_string_delimiter, bool detect_numeric_columns,
    bool force_double, int default_int, double default_double, vtkTable* const output_table)
    : MaxRecords(max_records)
    , MaxRecordIndex(have_headers ? max_records + 1 : max_records)
    , RecordDelimiters(record_delimiters.begin(), record_delimiters.end())
    , FieldDelimiters(field_delimiters.begin(), field_delimiters.end())
    , StringDelimiters(string_delimiters.begin(), string_delimiters.end())
    , Whitespace(whitespace.begin(), whitespace.end())
    , EscapeDelimiter(escape.begin(), escape.end())
    , HaveHeaders(have_headers)
    , WhiteSpaceOnlyString(true)
    , OutputTable(output_table)
    , CurrentRecordIndex(0)
    , CurrentFieldIndex(0)
    , RecordAdjacent(true)
    , MergeConsDelims(merg_cons_delimiters)
    , ProcessEscapeSequence(false)
    , UseStringDelimiter(use_string_delimiter)
    , DetectNumericColumns(detect_numeric_columns)
    , ForceDouble(force_double)
    , DefaultIntegerValue(default_int)
    , DefaultDoubleValue(default_double)
    , WithinString(0)
  {
  }

  ~DelimitedTextIterator() override
  {
    // Ensure that all table columns have the same length ...
    for (vtkIdType i = 0; i != this->OutputTable->GetNumberOfColumns(); ++i)
    {
      if (this->OutputTable->GetColumn(i)->GetNumberOfTuples() !=
        this->OutputTable->GetColumn(0)->GetNumberOfTuples())
      {
        this->OutputTable->GetColumn(i)->Resize(
          this->OutputTable->GetColumn(0)->GetNumberOfTuples());
      }
    }
  }

  // Handle windows files that do not have a carriage return line feed on the last line of the file
  // ...
  void ReachedEndOfInput()
  {
    if (!this->CurrentField.empty())
    {
      std::string::iterator start = this->CurrentField.begin();
      std::string::iterator end = this->CurrentField.end();
      std::string::size_type size = utf8::distance(start, end);

      std::string::iterator iterator = start;
      utf8::advance(iterator, size - 1, end);
      vtkTypeUInt32 value = utf8::next(iterator, end);
      // if the last character is not a CR/LF add a column
      if (!this->RecordDelimiters.count(value) && !this->Whitespace.count(value))
      {
        this->InsertField();
      }
    }
  }

  DelimitedTextIterator& operator=(const vtkTypeUInt32& value) override
  {
    // If we've already read our maximum number of records, we're done ...
    if (this->MaxRecords && this->CurrentRecordIndex == this->MaxRecordIndex)
    {
      return *this;
    }

    // Strip adjacent record delimiters and whitespace...
    if (this->RecordAdjacent &&
      (this->RecordDelimiters.count(value) || this->Whitespace.count(value)))
    {
      return *this;
    }
    else
    {
      this->RecordAdjacent = false;
    }

    // Look for record delimiters ...
    if (this->RecordDelimiters.count(value))
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
    if (!this->WithinString && this->FieldDelimiters.count(value))
    {
      // Handle special case of merging consecutive delimiters ...
      if (!(this->CurrentField.empty() && this->MergeConsDelims))
      {
        this->InsertField();
        this->CurrentFieldIndex += 1;
        this->CurrentField.clear();
      }
      return *this;
    }

    // Check for start of escape sequence ...
    if (!this->ProcessEscapeSequence && this->EscapeDelimiter.count(value))
    {
      this->ProcessEscapeSequence = true;
      return *this;
    }

    // Process escape sequence ...
    if (this->ProcessEscapeSequence)
    {
      std::string curr_char;
      utf8::append(value, std::back_inserter(curr_char));
      if (curr_char == "a")
      {
        this->CurrentField += "\a";
      }
      else if (curr_char == "b")
      {
        this->CurrentField += "\b";
      }
      else if (curr_char == "t")
      {
        this->CurrentField += "\t";
      }
      else if (curr_char == "n")
      {
        this->CurrentField += "\n";
      }
      else if (curr_char == "v")
      {
        this->CurrentField += "\v";
      }
      else if (curr_char == "f")
      {
        this->CurrentField += "\f";
      }
      else if (curr_char == "r")
      {
        this->CurrentField += "\r";
      }
      else if (curr_char == "\\")
      {
        this->CurrentField += "\\";
      }
      else if (!(curr_char == "0"))
      {
        this->CurrentField += curr_char;
      }
      this->ProcessEscapeSequence = false;
      return *this;
    }

    // Start a string ...
    if (!this->WithinString && this->StringDelimiters.count(value) && this->UseStringDelimiter)
    {
      this->WithinString = value;
      this->CurrentField.clear();
      return *this;
    }

    // End a string ...
    if (this->WithinString && (this->WithinString == value) && this->UseStringDelimiter)
    {
      this->WithinString = 0;
      return *this;
    }

    if (!this->Whitespace.count(value))
    {
      this->WhiteSpaceOnlyString = false;
    }
    // Keep growing the current field ...
    utf8::append(value, std::back_inserter(this->CurrentField));
    return *this;
  }

private:
  template <typename T>
  static vtkSmartPointer<vtkStringArray> ToStringArray(T* array)
  {
    auto output = vtkSmartPointer<vtkStringArray>::New();
    output->SetNumberOfTuples(array->GetNumberOfTuples());
    output->SetName(array->GetName());

    for (vtkIdType i = 0; i < array->GetNumberOfTuples(); ++i)
    {
      output->SetValue(i, std::to_string(array->GetValue(i)));
    }

    return output;
  }

  static vtkSmartPointer<vtkDoubleArray> ToDoubleArray(vtkIntArray* array)
  {
    auto output = vtkSmartPointer<vtkDoubleArray>::New();
    output->SetNumberOfTuples(array->GetNumberOfTuples());
    output->SetName(array->GetName());

    for (vtkIdType i = 0; i < array->GetNumberOfTuples(); ++i)
    {
      output->SetValue(i, static_cast<double>(array->GetValue(i)));
    }

    return output;
  }

  /**
   * Append value in an Int array.
   * Convert array if needed.
   * Return the new array if constructed, or nullptr.
   */
  vtkSmartPointer<vtkAbstractArray> Append(
    vtkIntArray* array, vtkIdType index, const std::string& str)
  {
    const auto pred = [](char c) { return std::isspace(static_cast<unsigned char>(c)); };

    const auto begin = str.data();
    const auto end = str.data() + str.size();

    const auto trimBegin = std::find_if_not(begin, end, pred);
    if (trimBegin == end)
    {
      array->InsertValue(index, this->DefaultIntegerValue);
      return nullptr;
    }

    // Try convert to double to check if it is a valid numeric entry.
    // If not, convert to vtkStringArray.
    double valAsDouble = 0.0;
    const size_t consumed = vtkValueFromString(trimBegin, end, valAsDouble);
    if (consumed == 0 || std::find_if_not(trimBegin + consumed, end, pred) != end)
    {
      auto output = this->ToStringArray(array);
      output->InsertValue(index, str.c_str());
      return output;
    }

    // Now try using integer.
    // If not, convert to vtkDoubleArray.
    int valAsInt = 0;
    const size_t consumed2 = vtkValueFromString(trimBegin, end, valAsInt);
    if (consumed2 == 0 || consumed2 != consumed)
    {
      auto output = this->ToDoubleArray(array);
      output->InsertValue(index, valAsDouble);
      return output;
    }

    array->InsertValue(index, valAsInt);
    return nullptr;
  }

  /**
   * Append value in a Double array.
   * Convert array if needed.
   * Return the new array if constructed, or nullptr.
   */
  vtkSmartPointer<vtkAbstractArray> Append(
    vtkDoubleArray* array, vtkIdType index, const std::string& str)
  {
    const auto pred = [](char c) { return std::isspace(static_cast<unsigned char>(c)); };

    const auto begin = str.data();
    const auto end = str.data() + str.size();

    // empty data, insert default value.
    const auto trimBegin = std::find_if_not(begin, end, pred);
    if (trimBegin == end)
    {
      array->InsertValue(index, this->DefaultDoubleValue);
      return nullptr;
    }

    // it is a double or a string that started with an integer
    double valAsDouble = 0.0;
    const size_t consumed = vtkValueFromString(trimBegin, end, valAsDouble);
    if (consumed == 0 || std::find_if_not(trimBegin + consumed, end, pred) != end)
    {
      auto output = this->ToStringArray(array);
      output->InsertValue(index, str.c_str());
      return output;
    }

    array->InsertValue(index, valAsDouble);
    return nullptr;
  }

  /**
   * Append value in a string array.
   * Always return nullptr as it never convert the array to a new type.
   */
  vtkSmartPointer<vtkAbstractArray> Append(
    vtkStringArray* array, vtkIdType index, const std::string& str)
  {
    array->InsertValue(index, str.c_str());
    return nullptr;
  }

  /**
   * Append value to array.
   * Forwards to the dedicated method specialized for each array type.
   * Convert the array as needed to another type. If converted, return the new array.
   * By default, return nullptr (array was not converted).
   */
  vtkSmartPointer<vtkAbstractArray> Append(
    vtkAbstractArray* array, vtkIdType index, const std::string& str)
  {
    auto iarr = vtkIntArray::SafeDownCast(array);
    if (iarr)
    {
      return this->Append(iarr, index, str);
    }

    auto darr = vtkDoubleArray::SafeDownCast(array);
    if (darr)
    {
      return this->Append(darr, index, str);
    }

    return this->Append(vtkStringArray::SafeDownCast(array), index, str);
  }

  /**
   * Create new array with the good basic type.
   * Use header to set array name or construct a default name.
   */
  void CreateColumn()
  {
    vtkSmartPointer<vtkAbstractArray> array;
    if (this->DetectNumericColumns)
    {
      if (this->ForceDouble)
      {
        array = vtkSmartPointer<vtkDoubleArray>::New();
      }
      else
      {
        array = vtkSmartPointer<vtkIntArray>::New();
      }
    }
    else
    {
      array = vtkSmartPointer<vtkStringArray>::New();
    }

    // Set array name
    if (this->HaveHeaders)
    {
      array->SetName(this->CurrentField.c_str());
    }
    else
    {
      std::stringstream buffer;
      buffer << "Field " << this->CurrentFieldIndex;
      array->SetName(buffer.str().c_str());
    }

    array->SetNumberOfTuples(this->OutputTable->GetNumberOfRows());
    this->OutputTable->AddColumn(array);
  }

  // Insert value in a column.
  // Create the column as needed.
  // Convert the column type if needed.
  void InsertField()
  {
    // Add column only when parsing first line
    if (this->CurrentFieldIndex >= this->OutputTable->GetNumberOfColumns() &&
      this->CurrentRecordIndex == 0)
    {
      this->CreateColumn();
    }

    if (this->CurrentFieldIndex < this->OutputTable->GetNumberOfColumns())
    {
      vtkIdType rec_index = this->CurrentRecordIndex;
      if (this->HaveHeaders)
      {
        rec_index--;
      }

      if (rec_index >= 0)
      {
        auto array = this->OutputTable->GetColumn(this->CurrentFieldIndex);
        auto newArray = this->Append(array, rec_index, this->CurrentField);
        if (newArray) // Array has been converted to another type
        {
          this->OutputTable->SetNumberOfRows(newArray->GetNumberOfTuples());
          this->OutputTable->RemoveColumn(this->CurrentFieldIndex);
          this->OutputTable->InsertColumn(newArray, this->CurrentFieldIndex);
        }
      }
    }
  }

  vtkIdType MaxRecords;
  vtkIdType MaxRecordIndex;
  std::set<vtkTypeUInt32> RecordDelimiters;
  std::set<vtkTypeUInt32> FieldDelimiters;
  std::set<vtkTypeUInt32> StringDelimiters;
  std::set<vtkTypeUInt32> Whitespace;
  std::set<vtkTypeUInt32> EscapeDelimiter;
  bool HaveHeaders;
  bool WhiteSpaceOnlyString;
  vtkTable* OutputTable;
  vtkIdType CurrentRecordIndex;
  vtkIdType CurrentFieldIndex;
  std::string CurrentField;
  bool RecordAdjacent;
  bool MergeConsDelims;
  bool ProcessEscapeSequence;
  bool UseStringDelimiter;
  bool DetectNumericColumns;
  bool ForceDouble;
  int DefaultIntegerValue;
  double DefaultDoubleValue;
  vtkTypeUInt32 WithinString;
};

} // End anonymous namespace

/////////////////////////////////////////////////////////////////////////////////////////
// vtkDelimitedTextReader

vtkStandardNewMacro(vtkDelimitedTextReader);

//------------------------------------------------------------------------------
vtkDelimitedTextReader::vtkDelimitedTextReader()
  : FileName(nullptr)
  , UnicodeCharacterSet(nullptr)
  , MaxRecords(0)
  , UnicodeRecordDelimiters("\r\n")
  , UnicodeFieldDelimiters(",")
  , UnicodeStringDelimiters("\"")
  , UnicodeWhitespace(" \t\r\n\v\f")
  , UnicodeEscapeCharacter("\\")
  , HaveHeaders(false)
  , ReplacementCharacter('x')
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->ReadFromInputString = 0;
  this->InputString = nullptr;
  this->InputStringLength = 0;
  this->MergeConsecutiveDelimiters = false;
  this->PedigreeIdArrayName = nullptr;
  this->SetPedigreeIdArrayName("id");
  this->GeneratePedigreeIds = true;
  this->OutputPedigreeIds = false;
  this->AddTabFieldDelimiter = false;
  this->FieldDelimiterCharacters = nullptr;
  this->SetFieldDelimiterCharacters(",");
  this->StringDelimiter = '"';
  this->UseStringDelimiter = true;
  this->DetectNumericColumns = false;
  this->ForceDouble = false;
  this->DefaultIntegerValue = 0;
  this->DefaultDoubleValue = 0.0;
  this->TrimWhitespacePriorToNumericConversion = false;
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
  os << indent << "MaxRecords: " << this->MaxRecords << endl;
  os << indent << "UnicodeRecordDelimiters: '" << this->UnicodeRecordDelimiters << "'" << endl;
  os << indent << "UnicodeFieldDelimiters: '" << this->UnicodeFieldDelimiters << "'" << endl;
  os << indent << "UnicodeStringDelimiters: '" << this->UnicodeStringDelimiters << "'" << endl;
  os << indent << "StringDelimiter: " << this->StringDelimiter << endl;
  os << indent << "ReplacementCharacter: " << this->ReplacementCharacter << endl;
  os << indent << "FieldDelimiterCharacters: "
     << (this->FieldDelimiterCharacters ? this->FieldDelimiterCharacters : "(none)") << endl;
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
    DelimitedTextIterator iterator(this->MaxRecords, this->UnicodeRecordDelimiters,
      this->UnicodeFieldDelimiters, this->UnicodeStringDelimiters, this->UnicodeWhitespace,
      this->UnicodeEscapeCharacter, this->HaveHeaders, this->MergeConsecutiveDelimiters,
      this->UseStringDelimiter, this->DetectNumericColumns, this->ForceDouble,
      this->DefaultIntegerValue, this->DefaultDoubleValue, output_table);

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
