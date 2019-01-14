/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostSplitTableField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBoostSplitTableField.h"

#include "vtkAbstractArray.h"
#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>

vtkStandardNewMacro(vtkBoostSplitTableField);

/// Ecapsulates private implementation details of vtkBoostSplitTableField
class vtkBoostSplitTableField::implementation
{
public:
  typedef boost::char_separator<char> delimiter_t;
  typedef boost::tokenizer<delimiter_t> tokenizer_t;
  typedef std::vector<tokenizer_t*> tokenizers_t;

  static void GenerateRows(const tokenizers_t& tokenizers, const unsigned int column_index, vtkVariantArray* input_row, vtkVariantArray* output_row, vtkTable* output_table)
  {
    if(column_index == tokenizers.size())
    {
      output_table->InsertNextRow(output_row);
      return;
    }

    tokenizer_t* const tokenizer = tokenizers[column_index];
    vtkVariant input_value = input_row->GetValue(column_index);

    if(tokenizer && input_value.IsString())
    {
      const std::string value = input_value.ToString();
      tokenizer->assign(value);
      for(tokenizer_t::iterator token = tokenizer->begin(); token != tokenizer->end(); ++token)
      {
        output_row->SetValue(column_index, boost::trim_copy(*token).c_str());
        GenerateRows(tokenizers, column_index + 1, input_row, output_row, output_table);
      }
    }
    else
    {
      output_row->SetValue(column_index, input_value);
      GenerateRows(tokenizers, column_index + 1, input_row, output_row, output_table);
    }
  }
};

vtkBoostSplitTableField::vtkBoostSplitTableField() :
  Fields(vtkStringArray::New()),
  Delimiters(vtkStringArray::New())
{
}

vtkBoostSplitTableField::~vtkBoostSplitTableField()
{
  this->Delimiters->Delete();
  this->Fields->Delete();
}

void vtkBoostSplitTableField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkBoostSplitTableField::ClearFields()
{
  this->Fields->Initialize();
  this->Delimiters->Initialize();
  this->Modified();
}

void vtkBoostSplitTableField::AddField(const char* field, const char* delimiters)
{
  assert(field);
  assert(delimiters);

  this->Fields->InsertNextValue(field);
  this->Delimiters->InsertNextValue(delimiters);

  this->Modified();
}

int vtkBoostSplitTableField::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkTable* const input = vtkTable::GetData(inputVector[0]);
  vtkTable* const output = vtkTable::GetData(outputVector);

  // If the number of fields and delimiters don't match, we're done ...
  if(this->Fields->GetNumberOfValues() != this->Delimiters->GetNumberOfValues())
  {
    vtkErrorMacro("The number of fields and the number of delimiters must match");
    return 0;
  }

  // If no fields were specified, we don't do any splitting - just shallow copy ...
  if(0 == this->Fields->GetNumberOfValues())
  {
    output->ShallowCopy(input);
    return 1;
  }

  // Setup the columns for our output table ...
  for(vtkIdType i = 0; i < input->GetNumberOfColumns(); ++i)
  {
    vtkAbstractArray* const column = input->GetColumn(i);
    vtkAbstractArray* const new_column = vtkAbstractArray::CreateArray(column->GetDataType());
    new_column->SetName(column->GetName());
    new_column->SetNumberOfComponents(column->GetNumberOfComponents());
    output->AddColumn(new_column);
    if(input->GetRowData()->GetPedigreeIds() == column)
    {
      output->GetRowData()->SetPedigreeIds(new_column);
    }
    new_column->Delete();
  }

  // Setup a tokenizer for each column that will be split ...
  implementation::tokenizers_t tokenizers;
  for(vtkIdType column = 0; column < input->GetNumberOfColumns(); ++column)
  {
    tokenizers.push_back(static_cast<implementation::tokenizer_t*>(0));

    for(vtkIdType field = 0; field < this->Fields->GetNumberOfValues(); ++field)
    {
      if(this->Fields->GetValue(field) == input->GetColumn(column)->GetName())
      {
        tokenizers[column] = new implementation::tokenizer_t(std::string(), implementation::delimiter_t(this->Delimiters->GetValue(field)));
        break;
      }
    }
  }

  // Iterate over each row in the input table, generating one-to-many rows in the output table ...
  vtkVariantArray* const output_row = vtkVariantArray::New();
  output_row->SetNumberOfValues(input->GetNumberOfColumns());

  for(vtkIdType i = 0; i < input->GetNumberOfRows(); ++i)
  {
    vtkVariantArray* const input_row = input->GetRow(i);
    implementation::GenerateRows(tokenizers, 0, input_row, output_row, output);

    double progress = static_cast<double>(i) / static_cast<double>(input->GetNumberOfRows());
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
  }

  output_row->Delete();

  // Cleanup tokenizers ...
  for(implementation::tokenizers_t::iterator tokenizer = tokenizers.begin(); tokenizer != tokenizers.end(); ++tokenizer)
    delete *tokenizer;

  return 1;
}
