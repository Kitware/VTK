/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTokenizer.cxx

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
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkUnicodeStringArray.h>
#include <vtkTable.h>
#include <vtkTokenizer.h>

#include <stdexcept>

class vtkTokenizer::Internals
{
public:
  // Returns true iff a Unicode code point is a delimiter that should be dropped
  // (not included as a token in the output).
  bool is_dropped(vtkUnicodeString::value_type code_point)
  {
    for(DelimiterRanges::const_iterator range = this->DroppedDelimiters.begin(); range != this->DroppedDelimiters.end(); ++range)
      {
      if(range->first <= code_point && code_point < range->second)
        return true;
      }
      
    return false;
  }

  // Returns true iff a Unicode code point is a delimiter that should be kept
  // (included as a token in the output).
  bool is_kept(vtkUnicodeString::value_type code_point)
  {
    for(DelimiterRanges::const_iterator range = this->KeptDelimiters.begin(); range != this->KeptDelimiters.end(); ++range)
      {
      if(range->first <= code_point && code_point < range->second)
        return true;
      }
      
    return false;
  }

  DelimiterRanges DroppedDelimiters;
  DelimiterRanges KeptDelimiters;
};

vtkStandardNewMacro(vtkTokenizer);

vtkTokenizer::vtkTokenizer() :
  Implementation(new Internals())
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);

  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "document");
  this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "text");
  this->SetInputArrayToProcess(2, 1, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "document");
  this->SetInputArrayToProcess(3, 1, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "begin");
  this->SetInputArrayToProcess(4, 1, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "end");
}

vtkTokenizer::~vtkTokenizer()
{
  delete this->Implementation;
}

void vtkTokenizer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << hex << setfill('0');
  for(size_t i = 0; i != this->Implementation->DroppedDelimiters.size(); ++i)
    {
    os << indent << "DroppedDelimiters: [" << "0x" << setw(4) << this->Implementation->DroppedDelimiters[i].first;
    os << ", " << "0x" << setw(4) << this->Implementation->DroppedDelimiters[i].second << ")\n";
    }
  for(size_t i = 0; i != this->Implementation->KeptDelimiters.size(); ++i)
    {
    os << indent << "KeptDelimiters: [" << "0x" << setw(4) << this->Implementation->KeptDelimiters[i].first;
    os << ", " << "0x" << setw(4) << this->Implementation->KeptDelimiters[i].second << ")\n";
    }
  os << dec;
}

const vtkTokenizer::DelimiterRanges vtkTokenizer::Punctuation()
{
  // Unicode punctuation based on the charts available at http://www.unicode.org/charts/symbols.html

  vtkTokenizer::DelimiterRanges result;

  result.push_back(std::make_pair(0x0021, 0x0030)); // ASCII Punctuation and Symbols
  result.push_back(std::make_pair(0x003a, 0x0041)); // ASCII Punctuation and Symbols
  result.push_back(std::make_pair(0x005b, 0x0061)); // ASCII Punctuation and Symbols
  result.push_back(std::make_pair(0x007b, 0x007f)); // ASCII Punctuation and Symbols
  result.push_back(std::make_pair(0x00a1, 0x00c0)); // Latin Punctuation and Symbols
  result.push_back(std::make_pair(0x200c, 0x206f)); // General Punctuation
  result.push_back(std::make_pair(0x2100, 0x214F)); // Letter-like Symbols
  result.push_back(std::make_pair(0x3000, 0x3040)); // CJK Symbols and Punctuation
  result.push_back(std::make_pair(0xfeff, 0xff00)); // Zero-width no-break space, which has become a de-facto byte-order mark
  result.push_back(std::make_pair(0xff01, 0xff10)); // Full-width punctuation
  result.push_back(std::make_pair(0xff1a, 0xff21)); // Full-width punctuation
  result.push_back(std::make_pair(0xff3b, 0xff41)); // Full-width punctuation
  result.push_back(std::make_pair(0xff5b, 0xff65)); // Full-width and half-width punctuation
  result.push_back(std::make_pair(0xffe0, 0xffef)); // Full-width and half-width symbols

  return result;
}

const vtkTokenizer::DelimiterRanges vtkTokenizer::Whitespace()
{
  // Unicode whitespace based on the charts available at http://www.unicode.org/charts, including
  // http://unicode.org/charts/PDF/U0000.pdf

  vtkTokenizer::DelimiterRanges result;
 
  result.push_back(std::make_pair(0x0000, 0x0021)); // Includes, among other things: NUL, HT, LF, VT, FF, CR, ESC, Space
  result.push_back(std::make_pair(0x0080, 0x00a1)); // Latin control codes and no-break space.
  result.push_back(std::make_pair(0x2000, 0x200c)); // General Punctuation

  return result;
}

const vtkTokenizer::DelimiterRanges vtkTokenizer::Logosyllabic()
{
  // Unicode logosyllabic characters based on the charts available at http://www.unicode.org/charts

  vtkTokenizer::DelimiterRanges result;

  result.push_back(std::make_pair(0x4e00, 0x9fd0)); // CJK Unified Ideographs
  result.push_back(std::make_pair(0x3400, 0x4e00)); // CJK Unified Ideographs Extension A
  result.push_back(std::make_pair(0x20000, 0x2A6e0)); // CJK Unified Ideographs Extension B
  result.push_back(std::make_pair(0xf900, 0xfb00)); // CJK Compatibility Ideographs
  result.push_back(std::make_pair(0x2f800, 0x2fa20)); // CJK Compatibility Ideographs Supplement

  return result;
}

void vtkTokenizer::AddDroppedDelimiters(vtkUnicodeString::value_type begin, vtkUnicodeString::value_type end)
{
  this->Implementation->DroppedDelimiters.push_back(std::make_pair(begin, std::max(begin, end)));
  this->Modified();
}

void vtkTokenizer::AddDroppedDelimiters(const DelimiterRanges& ranges)
{
  this->Implementation->DroppedDelimiters.insert(this->Implementation->DroppedDelimiters.end(), ranges.begin(), ranges.end());
  this->Modified();
}

void vtkTokenizer::AddKeptDelimiters(vtkUnicodeString::value_type begin, vtkUnicodeString::value_type end)
{
  this->Implementation->KeptDelimiters.push_back(std::make_pair(begin, std::max(begin, end)));
  this->Modified();
}

void vtkTokenizer::AddKeptDelimiters(const DelimiterRanges& ranges)
{
  this->Implementation->KeptDelimiters.insert(this->Implementation->KeptDelimiters.end(), ranges.begin(), ranges.end());
  this->Modified();
}

void vtkTokenizer::DropPunctuation()
{
  this->AddDroppedDelimiters(this->Punctuation());
}

void vtkTokenizer::DropWhitespace()
{
  this->AddDroppedDelimiters(this->Whitespace());
}

void vtkTokenizer::KeepPunctuation()
{
  this->AddKeptDelimiters(this->Punctuation());
}

void vtkTokenizer::KeepWhitespace()
{
  this->AddKeptDelimiters(this->Whitespace());
}

void vtkTokenizer::KeepLogosyllabic()
{
  this->AddKeptDelimiters(this->Logosyllabic());
}

void vtkTokenizer::ClearDroppedDelimiters()
{
  this->Implementation->DroppedDelimiters.clear();
  this->Modified();
}

void vtkTokenizer::ClearKeptDelimiters()
{
  this->Implementation->KeptDelimiters.clear();
  this->Modified();
}

int vtkTokenizer::FillInputPortInformation(int port, vtkInformation* info)
{
  switch(port)
    {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
      return 1;
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
      info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
      return 1;
    }

  return 0;
}

int vtkTokenizer::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  try
    {
    // Enforce our input preconditions ...
    vtkIdTypeArray* const input_document_array = vtkIdTypeArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(0, 0, inputVector));
    if(!input_document_array)
      throw std::runtime_error("missing input document ID array");

    vtkUnicodeStringArray* const input_text_array = vtkUnicodeStringArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(1, 0, inputVector));
    if(!input_text_array)
      throw std::runtime_error("missing input text array");

    vtkIdTypeArray* range_documents_array = 0;
    vtkIdTypeArray* range_begin_array = 0;
    vtkIdTypeArray* range_end_array = 0;
    vtkTable* const ranges = vtkTable::GetData(inputVector[1]);
    if(ranges)
      {
      range_documents_array = vtkIdTypeArray::SafeDownCast(this->GetInputAbstractArrayToProcess(2, 0, inputVector));
      if(!range_documents_array)
        throw std::runtime_error("Missing range documents array.");
      range_begin_array = vtkIdTypeArray::SafeDownCast(this->GetInputAbstractArrayToProcess(3, 0, inputVector));
      if(!range_begin_array)
        throw std::runtime_error("Missing range begin array.");
      range_end_array = vtkIdTypeArray::SafeDownCast(this->GetInputAbstractArrayToProcess(4, 0, inputVector));
      if(!range_end_array)
        throw std::runtime_error("Missing range end array.");
      }

    // Setup our output ...
    vtkIdTypeArray* const document_array = vtkIdTypeArray::New();
    document_array->SetName("document");

    vtkIdTypeArray* const begin_array = vtkIdTypeArray::New();
    begin_array->SetName("begin");

    vtkIdTypeArray* const end_array = vtkIdTypeArray::New();
    end_array->SetName("end");

    vtkStringArray* const type_array = vtkStringArray::New();
    type_array->SetName("type");

    vtkUnicodeStringArray* const text_array = vtkUnicodeStringArray::New();
    text_array->SetName("text");

    // Do the work ...
    vtkIdType count = input_document_array->GetNumberOfTuples();
    for(vtkIdType i = 0; i != input_document_array->GetNumberOfTuples(); ++i)
      {
      const vtkIdType document_id = input_document_array->GetValue(i);
      const vtkUnicodeString& document_text = input_text_array->GetValue(i);
      const vtkIdType document_length = static_cast<vtkIdType>(
        document_text.character_count());

      std::vector<vtkIdType> range_begin;
      std::vector<vtkIdType> range_end;
      if(ranges)
        {
        for(vtkIdType range = 0; range != range_documents_array->GetNumberOfTuples(); ++range)
          {
          if(range_documents_array->GetValue(range) != document_id)
            continue;

          range_begin.push_back(std::min(document_length, range_begin_array->GetValue(range)));
          range_end.push_back(std::min(document_length, range_end_array->GetValue(range)));
          }
        }
      else
        {
        range_begin.push_back(0);
        range_end.push_back(document_length);
        }

      for(size_t range = 0; range != range_begin.size(); ++range)
        {
        vtkIdType current_offset = range_begin[range];

        vtkUnicodeString::const_iterator current = document_text.begin();
        std::advance(current, range_begin[range]);

        vtkUnicodeString::const_iterator end = current;
        std::advance(end, range_end[range] - range_begin[range]);
        
        for(; current != end; )
          {
          // Skip past any dropped delimiters ...
          while(current != end && this->Implementation->is_dropped(*current))
            {
            ++current;
            ++current_offset;
            }
          if(current == end)
            break;

          const vtkUnicodeString::const_iterator start(current);
          const vtkIdType start_offset(current_offset);

          // If we are on a kept delimiter, move past it and stop ...
          if(this->Implementation->is_kept(*current))
            {
            ++current;
            ++current_offset;
            }
          // Otherwise, append all non-delimiter characters ...
          else
            {
            for(; current != end && !this->Implementation->is_kept(*current) && !this->Implementation->is_dropped(*current); ++current, ++current_offset)
              {
              }
            }

          document_array->InsertNextValue(document_id);
          begin_array->InsertNextValue(start_offset);
          end_array->InsertNextValue(current_offset);
          type_array->InsertNextValue("token");
          text_array->InsertNextValue(vtkUnicodeString(start, current));
          }
        }

      if( i % 100 == 0 )
        {
        //emit progress...
        double progress = static_cast<double>(i) / static_cast<double>(count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }

    vtkTable* const output_table = vtkTable::GetData(outputVector);
    output_table->AddColumn(document_array);
    output_table->AddColumn(begin_array);
    output_table->AddColumn(end_array);
    output_table->AddColumn(type_array);
    output_table->AddColumn(text_array);
    document_array->Delete();
    begin_array->Delete();
    end_array->Delete();
    type_array->Delete();
    text_array->Delete();
    }
  catch(std::exception& e)
    {
    vtkErrorMacro(<< "unhandled exception: " << e.what());
    return 0;
    }
  catch(...)
    {
    vtkErrorMacro(<< "unknown exception");
    return 0;
    }

  return 1;
}

