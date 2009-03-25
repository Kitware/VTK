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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

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

  const vtkArrayExtents extents = array->GetExtents();

  ColumnT* const column = ColumnT::New();
  column->SetNumberOfTuples(extents[0]);
  column->SetName("0");
  for(vtkIdType i = 0; i != extents[0]; ++i)
    {
    column->SetValue(i, array->GetValue(i));
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

  const vtkArrayExtents extents = array->GetExtents();

  for(vtkIdType j = 0; j != extents[1]; ++j)
    {
    vtkstd::ostringstream column_name;
    column_name << j;
      
    ColumnT* const column = ColumnT::New();
    column->SetNumberOfTuples(extents[0]);
    column->SetName(column_name.str().c_str());

    for(vtkIdType i = 0; i != extents[0]; ++i)
      {
      column->SetValue(i, array->GetValue(i, j));
      }

    Output->AddColumn(column);
    column->Delete();
    }

  return true;
}

// ----------------------------------------------------------------------

vtkCxxRevisionMacro(vtkArrayToTable, "1.3");
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
    if(input_array_data->GetNumberOfArrays() != 1)
      throw vtkstd::runtime_error("vtkArrayToTable requires a vtkArrayData containing exactly one array.");
    
    vtkArray* const input_array = input_array_data->GetArray(0);
    if(input_array->GetDimensions() > 2)
      throw vtkstd::runtime_error("vtkArrayToTable input array must have 1 or 2 dimensions.");
    
    vtkTable* const output_table = vtkTable::GetData(outputVector);

    if(ConvertVector<double, vtkDoubleArray>(input_array, output_table)) return 1;
    if(ConvertVector<vtkStdString, vtkStringArray>(input_array, output_table)) return 1;
    
    if(ConvertMatrix<double, vtkDoubleArray>(input_array, output_table)) return 1;
    if(ConvertMatrix<vtkStdString, vtkStringArray>(input_array, output_table)) return 1;
    }
  catch(vtkstd::exception& e)
    {
    vtkErrorMacro(<< e.what());
    }

  return 0;
}

