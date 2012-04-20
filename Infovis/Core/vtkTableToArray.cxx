/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToArray.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractArray.h"
#include "vtkDenseArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTableToArray.h"
#include "vtkVariant.h"

#include <vtksys/stl/algorithm>

class vtkTableToArray::implementation
{
public:
  /// Store the list of columns as an ordered set of variants.  The type
  /// of each variant determines which columns will be inserted into the
  /// output matrix:
  ///
  /// vtkStdString - stores the name of a column to be inserted.
  /// int - stores the index of a column to be inserted.
  /// char 'A' - indicates that every table column should be inserted.
  std::vector<vtkVariant> Columns;
};

// ----------------------------------------------------------------------

vtkStandardNewMacro(vtkTableToArray);

// ----------------------------------------------------------------------

vtkTableToArray::vtkTableToArray() :
  Implementation(new implementation())
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkTableToArray::~vtkTableToArray()
{
  delete this->Implementation;
}

// ----------------------------------------------------------------------

void vtkTableToArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for(size_t i = 0; i != this->Implementation->Columns.size(); ++i)
    os << indent << "Column: " << this->Implementation->Columns[i] << endl;
}

void vtkTableToArray::ClearColumns()
{
  this->Implementation->Columns.clear();
  this->Modified();
}

void vtkTableToArray::AddColumn(const char* name)
{
  if(!name)
    {
    vtkErrorMacro(<< "cannot add column with NULL name");
    return;
    }

  this->Implementation->Columns.push_back(vtkVariant(vtkStdString(name)));
  this->Modified();
}

void vtkTableToArray::AddColumn(vtkIdType index)
{
  this->Implementation->Columns.push_back(vtkVariant(static_cast<int>(index)));
  this->Modified();
}

void vtkTableToArray::AddAllColumns()
{
  this->Implementation->Columns.push_back(vtkVariant(static_cast<char>('A')));
  this->Modified();
}

int vtkTableToArray::FillInputPortInformation(int port, vtkInformation* info)
{
  switch(port)
    {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
      return 1;
    }

  return 0;
}

// ----------------------------------------------------------------------

int vtkTableToArray::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkTable* const table = vtkTable::GetData(inputVector[0]);

  std::vector<vtkAbstractArray*> columns;

  for(size_t i = 0; i != this->Implementation->Columns.size(); ++i)
    {
    if(this->Implementation->Columns[i].IsString())
      {
      columns.push_back(table->GetColumnByName(this->Implementation->Columns[i].ToString().c_str()));
      if(!columns.back())
        {
        vtkErrorMacro(<< "Missing table column: " << this->Implementation->Columns[i].ToString().c_str());
        return 0;
        }
      }
    else if(this->Implementation->Columns[i].IsInt())
      {
      columns.push_back(table->GetColumn(this->Implementation->Columns[i].ToInt()));
      if(!columns.back())
        {
        vtkErrorMacro(<< "Missing table column: " << this->Implementation->Columns[i].ToInt());
        return 0;
        }
      }
    else if(this->Implementation->Columns[i].IsChar() && this->Implementation->Columns[i].ToChar() == 'A')
      {
      for(vtkIdType j = 0; j != table->GetNumberOfColumns(); ++j)
        {
        columns.push_back(table->GetColumn(j));
        }
      }
    }

  vtkDenseArray<double>* const array = vtkDenseArray<double>::New();
  array->Resize(table->GetNumberOfRows(), columns.size());
  array->SetDimensionLabel(0, "row");
  array->SetDimensionLabel(1, "column");

  for(vtkIdType i = 0; i != table->GetNumberOfRows(); ++i)
    {
    for(size_t j = 0; j != columns.size(); ++j)
      {
      array->SetValue(i, j, columns[j]->GetVariantValue(i).ToDouble());
      }
    }

  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(array);
  array->Delete();

  return 1;
}

