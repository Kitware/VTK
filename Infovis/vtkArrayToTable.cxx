/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayToTable.cxx
  
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
#include "vtkArrayToTable.h"
#include "vtkDenseArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkUnicodeStringArray.h"

#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

/// Convert a 1D array to a table with one column ...
template<typename ValueT, typename ColumnT>
static bool ConvertVector(vtkArray* Array, vtkTable* Output)
{
  if(Array->GetDimensions() != 1)
    return false;

  vtkTypedArray<ValueT>* const array = vtkTypedArray<ValueT>::SafeDownCast(Array);
  if(!array)
    return false;

  const vtkArrayRange extents = array->GetExtent(0);

  ColumnT* const column = ColumnT::New();
  column->SetNumberOfTuples(extents.GetSize());
  column->SetName(array->GetName());
  for(vtkIdType i = extents.GetBegin(); i != extents.GetEnd(); ++i)
    {
    column->SetValue(i - extents.GetBegin(), array->GetValue(i));
    }

  Output->AddColumn(column);
  column->Delete();

  return true;
}

/// Convert a 2D array to a table with 1-or-more columns ...
template<typename ValueT, typename ColumnT>
static bool ConvertMatrix(vtkArray* Array, vtkTable* Output)
{
  if(Array->GetDimensions() != 2)
    return false;

  vtkTypedArray<ValueT>* const array = vtkTypedArray<ValueT>::SafeDownCast(Array);
  if(!array)
    return false;

  vtkSparseArray<ValueT>* const sparse_array = vtkSparseArray<ValueT>::SafeDownCast(array);

  const vtkIdType non_null_count = array->GetNonNullSize();
  const vtkArrayRange columns = array->GetExtent(1);
  const vtkArrayRange rows = array->GetExtent(0);

  vtkstd::vector<ColumnT*> new_columns;
  for(vtkIdType j = columns.GetBegin(); j != columns.GetEnd(); ++j)
    {
    vtkstd::ostringstream column_name;
    column_name << j;
      
    ColumnT* const column = ColumnT::New();
    column->SetNumberOfTuples(rows.GetSize());
    column->SetName(column_name.str().c_str());

    if(sparse_array)
      {
      for(vtkIdType i = 0; i != rows.GetSize(); ++i)
        column->SetValue(i, sparse_array->GetNullValue());
      }

    Output->AddColumn(column);
    column->Delete();
    new_columns.push_back(column);
    }

  for(vtkIdType n = 0; n != non_null_count; ++n)
    {
    vtkArrayCoordinates coordinates;
    array->GetCoordinatesN(n, coordinates);

    new_columns[coordinates[1] - columns.GetBegin()]->SetValue(coordinates[0] - rows.GetBegin(), array->GetValueN(n));
    }

  return true;
}

// ----------------------------------------------------------------------

vtkStandardNewMacro(vtkArrayToTable);

// ----------------------------------------------------------------------

vtkArrayToTable::vtkArrayToTable()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkArrayToTable::~vtkArrayToTable()
{
}

// ----------------------------------------------------------------------

void vtkArrayToTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkArrayToTable::FillInputPortInformation(int port, vtkInformation* info)
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

int vtkArrayToTable::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    vtkArrayData* const input_array_data = vtkArrayData::GetData(inputVector[0]);
    if(!input_array_data)
      throw vtkstd::runtime_error("Missing vtkArrayData on input port 0.");
    if(input_array_data->GetNumberOfArrays() != 1)
      throw vtkstd::runtime_error("vtkArrayToTable requires a vtkArrayData containing exactly one array.");
    
    vtkArray* const input_array = input_array_data->GetArray(static_cast<vtkIdType>(0));
    if(input_array->GetDimensions() > 2)
      throw vtkstd::runtime_error("vtkArrayToTable input array must have 1 or 2 dimensions.");
    
    vtkTable* const output_table = vtkTable::GetData(outputVector);

    if(ConvertVector<double, vtkDoubleArray>(input_array, output_table)) return 1;
    if(ConvertVector<vtkIdType, vtkIdTypeArray>(input_array, output_table)) return 1;
    if(ConvertVector<vtkStdString, vtkStringArray>(input_array, output_table)) return 1;
    if(ConvertVector<vtkUnicodeString, vtkUnicodeStringArray>(input_array, output_table)) return 1;
    
    if(ConvertMatrix<double, vtkDoubleArray>(input_array, output_table)) return 1;
    if(ConvertMatrix<vtkIdType, vtkIdTypeArray>(input_array, output_table)) return 1;
    if(ConvertMatrix<vtkStdString, vtkStringArray>(input_array, output_table)) return 1;
    if(ConvertMatrix<vtkUnicodeString, vtkUnicodeStringArray>(input_array, output_table)) return 1;

    throw vtkstd::runtime_error("Unhandled input array type.");
    }
  catch(vtkstd::exception& e)
    {
    vtkErrorMacro(<< "caught exception: " << e.what() << endl);
    }
  catch(...)
    {
    vtkErrorMacro(<< "caught unknown exception." << endl);
    }

  return 0;
}

