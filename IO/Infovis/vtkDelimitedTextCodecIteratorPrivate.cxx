// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDelimitedTextCodecIteratorPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkValueFromString.h"

#include <vtk_utf8.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
bool vtkDelimitedTextCodecIteratorPrivate::RecordsCounter::MaxReached()
{
  return this->HasMax && this->Current == this->Max;
}

//------------------------------------------------------------------------------
bool vtkDelimitedTextCodecIteratorPrivate::RecordsCounter::AcceptingField()
{
  return (!this->HasMax || this->Current < this->Max) && this->Current >= this->Start;
}

//------------------------------------------------------------------------------
void vtkDelimitedTextCodecIteratorPrivate::RecordsCounter::Next()
{
  this->Current++;
}

//------------------------------------------------------------------------------
void vtkDelimitedTextCodecIteratorPrivate::RecordsCounter::Skip()
{
  this->Skipped++;
}

//------------------------------------------------------------------------------
bool vtkDelimitedTextCodecIteratorPrivate::RecordsCounter::FirstAccepted()
{
  return this->Current == this->Start + this->Skipped;
}

//------------------------------------------------------------------------------
vtkIdType vtkDelimitedTextCodecIteratorPrivate::RecordsCounter::GetNumberOfAcceptedRecords()
{
  return this->Current - this->Skipped - this->Start;
}

//------------------------------------------------------------------------------
vtkDelimitedTextCodecIteratorPrivate::vtkDelimitedTextCodecIteratorPrivate(
  const vtkIdType startRecords, const vtkIdType maxRecords, const std::string& recordDelimiters,
  const std::string& fieldDelimiters, const std::string& stringDelimiters,
  const std::string& whitespace, const std::string& comments, const std::string& escape,
  bool haveHeaders, bool mergConsDelimiters, bool useStringDelimiter, bool detectNumericColumns,
  bool forceDouble, int defaultInt, double defaultDouble, vtkTable* const outputTable)
  : RecordsCount(maxRecords > 0, haveHeaders ? maxRecords + 1 : maxRecords, startRecords)
  , RecordDelimiters(recordDelimiters.begin(), recordDelimiters.end())
  , FieldDelimiters(fieldDelimiters.begin(), fieldDelimiters.end())
  , StringDelimiters(stringDelimiters.begin(), stringDelimiters.end())
  , Whitespace(whitespace.begin(), whitespace.end())
  , CommentChar(comments.begin(), comments.end())
  , EscapeDelimiter(escape.begin(), escape.end())
  , HaveHeaders(haveHeaders)
  , OutputTable(outputTable)
  , MergeConsDelims(mergConsDelimiters)
  , UseStringDelimiter(useStringDelimiter)
  , DetectNumericColumns(detectNumericColumns)
  , ForceDouble(forceDouble)
  , DefaultIntegerValue(defaultInt)
  , DefaultDoubleValue(defaultDouble)
{
}

//------------------------------------------------------------------------------
vtkDelimitedTextCodecIteratorPrivate::~vtkDelimitedTextCodecIteratorPrivate()
{
  // Ensure that all table columns have the same length ...
  for (vtkIdType i = 0; i != this->OutputTable->GetNumberOfColumns(); ++i)
  {
    if (this->OutputTable->GetColumn(i)->GetNumberOfTuples() !=
      this->OutputTable->GetColumn(0)->GetNumberOfTuples())
    {
      this->OutputTable->GetColumn(i)->Resize(this->OutputTable->GetColumn(0)->GetNumberOfTuples());
    }
  }
}

//------------------------------------------------------------------------------
void vtkDelimitedTextCodecIteratorPrivate::ReachedEndOfInput()
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

//------------------------------------------------------------------------------
vtkDelimitedTextCodecIteratorPrivate& vtkDelimitedTextCodecIteratorPrivate::operator=(
  const vtkTypeUInt32& value)
{
  // If we've already read our maximum number of records, we're done ...
  if (this->RecordsCount.MaxReached())
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
    // optionaly store current field
    if (this->RecordsCount.AcceptingField())
    {
      this->InsertField();
    }

    // reset internal state for new Record.
    this->RecordsCount.Next();
    this->CurrentFieldIndex = 0;
    this->CurrentField.clear();
    this->RecordAdjacent = true;
    this->WithinString = 0;
    this->WhiteSpaceOnlyString = true;
    this->WithinComment = false;
    return *this;
  }

  if (!this->RecordsCount.AcceptingField())
  {
    return *this;
  }

  if (this->CommentChar.count(value))
  {
    // ignore comment char inside comments or inside string
    if (!this->WithinComment && !this->WithinString)
    {
      if (this->CurrentField.empty() && this->CurrentFieldIndex == 0)
      {
        this->RecordsCount.Skip();
      }
      this->WithinComment = true;
    }
  }

  if (this->WithinComment)
  {
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

//------------------------------------------------------------------------------
template <typename T>
vtkSmartPointer<vtkStringArray> vtkDelimitedTextCodecIteratorPrivate::ToStringArray(T* array)
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

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDoubleArray> vtkDelimitedTextCodecIteratorPrivate::ToDoubleArray(
  vtkIntArray* array)
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

//------------------------------------------------------------------------------
vtkSmartPointer<vtkAbstractArray> vtkDelimitedTextCodecIteratorPrivate::Append(
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

//------------------------------------------------------------------------------
vtkSmartPointer<vtkAbstractArray> vtkDelimitedTextCodecIteratorPrivate::Append(
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

//------------------------------------------------------------------------------
vtkSmartPointer<vtkAbstractArray> vtkDelimitedTextCodecIteratorPrivate::Append(
  vtkStringArray* array, vtkIdType index, const std::string& str)
{
  array->InsertValue(index, str.c_str());
  return nullptr;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkAbstractArray> vtkDelimitedTextCodecIteratorPrivate::Append(
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

//------------------------------------------------------------------------------
void vtkDelimitedTextCodecIteratorPrivate::CreateColumn()
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

//------------------------------------------------------------------------------
void vtkDelimitedTextCodecIteratorPrivate::InsertField()
{
  // Add column only when parsing first line
  if (this->CurrentFieldIndex >= this->OutputTable->GetNumberOfColumns() &&
    this->RecordsCount.FirstAccepted())
  {
    this->CreateColumn();
  }

  if (this->CurrentFieldIndex < this->OutputTable->GetNumberOfColumns())
  {
    vtkIdType rec_index = this->RecordsCount.GetNumberOfAcceptedRecords();
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

VTK_ABI_NAMESPACE_END
