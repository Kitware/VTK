/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReduceTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkReduceTable.h"

#include "vtkAbstractArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <algorithm>

vtkStandardNewMacro(vtkReduceTable);
//---------------------------------------------------------------------------
vtkReduceTable::vtkReduceTable()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->IndexColumn = -1;
  this->NumericalReductionMethod = vtkReduceTable::MEAN;
  this->NonNumericalReductionMethod = vtkReduceTable::MODE;
}

//---------------------------------------------------------------------------
vtkReduceTable::~vtkReduceTable()
{
}

//---------------------------------------------------------------------------
int vtkReduceTable::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (this->IndexColumn == -1)
    {
    vtkWarningMacro(<< "Index column not set");
    return 1;
    }

  // Get input table
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkTable* input = vtkTable::SafeDownCast(
    inputInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->IndexColumn < 0 ||
      this->IndexColumn > input->GetNumberOfColumns() - 1)
    {
    vtkWarningMacro(<< "Index column exceeds bounds of input table");
    return 1;
    }

  // Get output table
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->InitializeOutputTable(input, output);
  this->AccumulateIndexValues(input);

  // set the number of rows in the output table
  output->SetNumberOfRows(static_cast<vtkIdType>(this->IndexValues.size()));

  this->PopulateIndexColumn(output);

 // populate the data columns of the output table
 for (vtkIdType col = 0; col < output->GetNumberOfColumns(); ++col)
   {
   if (col == this->IndexColumn)
     {
     continue;
     }

   this->PopulateDataColumn(input, output, col);

   }

  // Clean up pipeline information
  int piece = -1;
  int npieces = -1;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), npieces);
  output->GetInformation()->Set(vtkDataObject::DATA_PIECE_NUMBER(), piece);

  return 1;
}

//---------------------------------------------------------------------------
void vtkReduceTable::InitializeOutputTable(vtkTable *input, vtkTable *output)
{
  output->DeepCopy(input);
  for (vtkIdType row = output->GetNumberOfRows() - 1; row > -1; --row)
    {
    output->RemoveRow(row);
    }
}

//---------------------------------------------------------------------------
void vtkReduceTable::AccumulateIndexValues(vtkTable *input)
{
  for (vtkIdType row = 0; row < input->GetNumberOfRows(); ++row)
    {
    vtkVariant value = input->GetValue(row, this->IndexColumn);
    this->IndexValues.insert(value);
    std::map<vtkVariant, std::vector<vtkIdType> >::iterator itr =
      this->NewRowToOldRowsMap.find(value);
    if (itr == this->NewRowToOldRowsMap.end())
      {
      std::vector<vtkIdType> v;
      v.push_back(row);
      this->NewRowToOldRowsMap[value] = v;
      }
    else
      {
      itr->second.push_back(row);
      }
    }
}

//---------------------------------------------------------------------------
void vtkReduceTable::PopulateIndexColumn(vtkTable *output)
{
  vtkIdType row = 0;
  for (std::set<vtkVariant>::iterator itr = this->IndexValues.begin();
       itr != this->IndexValues.end(); ++itr)
    {
    output->SetValue(row, this->IndexColumn, *itr);
    ++row;
    }
}

//---------------------------------------------------------------------------
void vtkReduceTable::PopulateDataColumn(vtkTable *input, vtkTable *output,
                                        vtkIdType col)
{
  int reductionMethod = 0;

  // check if this column has a reduction method
  int columnSpecificMethod = this->GetReductionMethodForColumn(col);
  if (columnSpecificMethod != -1)
    {
    reductionMethod = columnSpecificMethod;
    }
  else
    {
    // determine whether this column contains numerical or not.
    if (input->GetValue(0, col).IsNumeric())
      {
      reductionMethod = this->NumericalReductionMethod;
      }
    else
      {
      reductionMethod = this->NonNumericalReductionMethod;
      }
    }

  for (vtkIdType row = 0; row < output->GetNumberOfRows(); ++row)
    {
    // look up the cells in the input table that should be represented by
    // this cell in the output table
    vtkVariant indexValue = output->GetValue(row, this->IndexColumn);
    std::vector<vtkIdType> oldRows = this->NewRowToOldRowsMap[indexValue];

    // special case: one-to-one mapping between input table and output table
    // (no collapse necessary)
    if (oldRows.size() == 1)
      {
      output->SetValue(row, col,
        input->GetValue(this->NewRowToOldRowsMap[indexValue].at(0), col));
      continue;
      }

    // otherwise, combine them appropriately & store the value in the
    // output table
    switch (reductionMethod)
      {
      case vtkReduceTable::MODE:
        this->ReduceValuesToMode(input, output, row, col, oldRows);
        break;
      case vtkReduceTable::MEDIAN:
        this->ReduceValuesToMedian(input, output, row, col, oldRows);
        break;
      case vtkReduceTable::MEAN:
      default:
        this->ReduceValuesToMean(input, output, row, col, oldRows);
        break;
      }
    }
}

//---------------------------------------------------------------------------
void vtkReduceTable::ReduceValuesToMean(vtkTable *input, vtkTable *output,
                                        vtkIdType row, vtkIdType col,
                                        std::vector<vtkIdType> oldRows)
{
  if (!input->GetValue(0, col).IsNumeric())
    {
    vtkErrorMacro(<< "Mean is unsupported for non-numerical data");
    return;
    }

  double mean = 0.0;
  for (std::vector<vtkIdType>::iterator itr = oldRows.begin();
       itr != oldRows.end(); ++itr)
    {
    mean += input->GetValue(*itr, col).ToDouble();
    }
  mean /= oldRows.size();
  output->SetValue(row, col, vtkVariant(mean));
}

//---------------------------------------------------------------------------
void vtkReduceTable::ReduceValuesToMedian(vtkTable *input, vtkTable *output,
                                          vtkIdType row, vtkIdType col,
                                          std::vector<vtkIdType> oldRows)
{
  if (!input->GetValue(0, col).IsNumeric())
    {
    vtkErrorMacro(<< "Median is unsupported for non-numerical data");
    return;
    }

  // generate a vector of values
  std::vector<double> values;
  for (std::vector<vtkIdType>::iterator itr = oldRows.begin();
       itr != oldRows.end(); ++itr)
    {
    values.push_back(input->GetValue(*itr, col).ToDouble());
    }

  // sort it
  std::sort(values.begin(), values.end());

  // get the median and store it in the output table
  if (values.size() % 2 == 1)
    {
    output->SetValue(row, col,
      vtkVariant( values.at( (values.size() - 1) / 2 ) )
    );
    }
  else
    {
    double d1 = values.at( (values.size() - 1) / 2 );
    double d2 = values.at( values.size() / 2 );
    output->SetValue(row, col, vtkVariant( (d1 + d2) / 2.0 ));
    }
}

//---------------------------------------------------------------------------
void vtkReduceTable::ReduceValuesToMode(vtkTable *input, vtkTable *output,
                                        vtkIdType row, vtkIdType col,
                                        std::vector<vtkIdType> oldRows)
{
  // setup a map to determine how frequently each value appears
  std::map<vtkVariant, int> modeMap;
  std::map<vtkVariant, int>::iterator mapItr;
  for (std::vector<vtkIdType>::iterator vectorItr = oldRows.begin();
       vectorItr != oldRows.end(); ++vectorItr)
    {
    vtkVariant v = input->GetValue(*vectorItr, col);
    mapItr = modeMap.find(v);
    if (mapItr == modeMap.end())
      {
      modeMap[v] = 1;
      }
    else
      {
      mapItr->second += 1;
      }
    }

  // use our map to find the mode & store it in the output table
  int maxCount = -1;
  vtkVariant mode;
  for (mapItr = modeMap.begin(); mapItr != modeMap.end(); ++mapItr)
    {
    if (mapItr->second > maxCount)
      {
      mode = mapItr->first;
      maxCount = mapItr->second;
      }
    }
  output->SetValue(row, col, mode);
}

//---------------------------------------------------------------------------
int vtkReduceTable::GetReductionMethodForColumn(vtkIdType col)
{
  std::map<vtkIdType, int>::iterator itr =
    this->ColumnReductionMethods.find(col);
  if (itr != this->ColumnReductionMethods.end())
    {
    return itr->second;
    }
  return -1;
}

//---------------------------------------------------------------------------
void vtkReduceTable::SetReductionMethodForColumn(vtkIdType col, int method)
{
  this->ColumnReductionMethods[col] = method;
}

//---------------------------------------------------------------------------
void vtkReduceTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IndexColumn: " << this->IndexColumn << endl;
  os << indent << "NumericalReductionMethod: "
     << this->NumericalReductionMethod << endl;
  os << indent << "NonNumericalReductionMethod: "
     << this->NonNumericalReductionMethod << endl;
}
