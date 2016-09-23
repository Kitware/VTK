/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdjacencyMatrixToEdgeTable.cxx

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
#include "vtkAdjacencyMatrixToEdgeTable.h"
#include "vtkCommand.h"
#include "vtkDenseArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <algorithm>
#include <map>
#include <functional>

// ----------------------------------------------------------------------

vtkStandardNewMacro(vtkAdjacencyMatrixToEdgeTable);

// ----------------------------------------------------------------------

vtkAdjacencyMatrixToEdgeTable::vtkAdjacencyMatrixToEdgeTable() :
  SourceDimension(0),
  ValueArrayName(0),
  MinimumCount(0),
  MinimumThreshold(0.5)
{
  this->SetValueArrayName("value");

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkAdjacencyMatrixToEdgeTable::~vtkAdjacencyMatrixToEdgeTable()
{
  this->SetValueArrayName(0);
}

// ----------------------------------------------------------------------

void vtkAdjacencyMatrixToEdgeTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SourceDimension: " << this->SourceDimension << endl;
  os << indent << "ValueArrayName: " << (this->ValueArrayName ? this->ValueArrayName : "") << endl;
  os << indent << "MinimumCount: " << this->MinimumCount << endl;
  os << indent << "MinimumThreshold: " << this->MinimumThreshold << endl;
}

int vtkAdjacencyMatrixToEdgeTable::FillInputPortInformation(int port, vtkInformation* info)
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

int vtkAdjacencyMatrixToEdgeTable::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  if(input->GetNumberOfArrays() != 1)
  {
    vtkErrorMacro(<< this->GetClassName() << " requires an input vtkArrayData containing one array.");
    return 0;
  }

  vtkDenseArray<double>* const input_array = vtkDenseArray<double>::SafeDownCast(
    input->GetArray(static_cast<vtkIdType>(0)));
  if(!input_array)
  {
    vtkErrorMacro(<< this->GetClassName() << " requires an input vtkDenseArray<double>.");
    return 0;
  }
  if(input_array->GetDimensions() != 2)
  {
    vtkErrorMacro(<< this->GetClassName() << " requires an input matrix.");
    return 0;
  }

  const vtkArrayExtents input_extents = input_array->GetExtents();

  const vtkIdType source_dimension = std::max(static_cast<vtkIdType>(0), std::min(static_cast<vtkIdType>(1), this->SourceDimension));
  const vtkIdType target_dimension = 1 - source_dimension;

  vtkTable* const output_table = vtkTable::GetData(outputVector);

  vtkIdTypeArray* const source_array = vtkIdTypeArray::New();
  source_array->SetName(input_array->GetDimensionLabel(source_dimension));

  vtkIdTypeArray* const target_array = vtkIdTypeArray::New();
  target_array->SetName(input_array->GetDimensionLabel(target_dimension));

  vtkDoubleArray* const value_array = vtkDoubleArray::New();
  value_array->SetName(this->ValueArrayName);

  // For each source in the matrix ...
  vtkArrayCoordinates coordinates(0, 0);
  for(vtkIdType i = input_extents[source_dimension].GetBegin(); i != input_extents[source_dimension].GetEnd(); ++i)
  {
    coordinates[source_dimension] = i;

    // Create a sorted list of source values ...
    typedef std::multimap<double, vtkIdType, std::greater<double> > sorted_values_t;
    sorted_values_t sorted_values;
    for(vtkIdType j = input_extents[target_dimension].GetBegin(); j != input_extents[target_dimension].GetEnd(); ++j)
    {
      coordinates[target_dimension] = j;

#ifdef _RWSTD_NO_MEMBER_TEMPLATES
      // Deal with Sun Studio old libCstd.
      // http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
      sorted_values.insert(std::pair<const double,vtkIdType>(input_array->GetValue(coordinates),j));
#else
      sorted_values.insert(std::make_pair(input_array->GetValue(coordinates), j));
#endif
    }

    // Create edges for each value that meets our count / threshold criteria ...
    vtkIdType count = 0;
    for(sorted_values_t::const_iterator value = sorted_values.begin(); value != sorted_values.end(); ++value, ++count)
    {
      if(count < this->MinimumCount || value->first >= this->MinimumThreshold)
      {
        source_array->InsertNextValue(i);
        target_array->InsertNextValue(value->second);
        value_array->InsertNextValue(value->first);
      }
    }

    double progress = static_cast<double>(i - input_extents[source_dimension].GetBegin()) / static_cast<double>(input_extents[source_dimension].GetSize());
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
  }


  output_table->AddColumn(source_array);
  output_table->AddColumn(target_array);
  output_table->AddColumn(value_array);

  source_array->Delete();
  target_array->Delete();
  value_array->Delete();

  return 1;
}

