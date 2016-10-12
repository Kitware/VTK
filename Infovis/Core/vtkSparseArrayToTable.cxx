/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSparseArrayToTable.cxx

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

#include "vtkArrayData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"
#include "vtkSparseArrayToTable.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <sstream>
#include <stdexcept>

template<typename ValueT, typename ValueColumnT>
static bool Convert(vtkArray* Array, const char* ValueColumn, vtkTable* Table)
{
  vtkSparseArray<ValueT>* const array = vtkSparseArray<ValueT>::SafeDownCast(Array);
  if(!array)
    return false;

  if(!ValueColumn)
    throw std::runtime_error("ValueColumn not specified.");

  const vtkIdType dimensions = array->GetDimensions();
  const vtkIdType value_count = array->GetNonNullSize();

  for(vtkIdType dimension = 0; dimension != dimensions; ++dimension)
  {
    vtkIdType* const array_coordinates = array->GetCoordinateStorage(dimension);

    vtkIdTypeArray* const table_coordinates = vtkIdTypeArray::New();
    table_coordinates->SetName(array->GetDimensionLabel(dimension));
    table_coordinates->SetNumberOfTuples(value_count);
    std::copy(array_coordinates, array_coordinates + value_count, table_coordinates->GetPointer(0));
    Table->AddColumn(table_coordinates);
    table_coordinates->Delete();
  }

  ValueT* const array_values = array->GetValueStorage();

  ValueColumnT* const table_values = ValueColumnT::New();
  table_values->SetName(ValueColumn);
  table_values->SetNumberOfTuples(value_count);
  std::copy(array_values, array_values + value_count, table_values->GetPointer(0));
  Table->AddColumn(table_values);
  table_values->Delete();

  return true;
}

// ----------------------------------------------------------------------

vtkStandardNewMacro(vtkSparseArrayToTable);

// ----------------------------------------------------------------------

vtkSparseArrayToTable::vtkSparseArrayToTable() :
  ValueColumn(0)
{
  this->SetValueColumn("value");

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkSparseArrayToTable::~vtkSparseArrayToTable()
{
  this->SetValueColumn(0);
}

// ----------------------------------------------------------------------

void vtkSparseArrayToTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ValueColumn: " << (this->ValueColumn ? this->ValueColumn : "(none)") << endl;
}

int vtkSparseArrayToTable::FillInputPortInformation(int port, vtkInformation* info)
{
  switch(port)
  {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
  }

  return 0;
}

// ----------------------------------------------------------------------

int vtkSparseArrayToTable::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  try
  {
    vtkArrayData* const input_array_data = vtkArrayData::GetData(inputVector[0]);
    if(input_array_data->GetNumberOfArrays() != 1)
      throw std::runtime_error("vtkSparseArrayToTable requires a vtkArrayData containing exactly one array.");

    vtkArray* const input_array = input_array_data->GetArray(static_cast<vtkIdType>(0));

    vtkTable* const output_table = vtkTable::GetData(outputVector);

    if(Convert<double, vtkDoubleArray>(input_array, this->ValueColumn, output_table)) return 1;
    if(Convert<vtkStdString, vtkStringArray>(input_array, this->ValueColumn, output_table)) return 1;
  }
  catch(std::exception& e)
  {
    vtkErrorMacro(<< e.what());
  }

  return 0;
}

