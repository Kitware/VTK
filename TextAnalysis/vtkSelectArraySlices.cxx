/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectArraySlices.cxx
  
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
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelectionNode.h"
#include "vtkSelectArraySlices.h"
#include "vtkSmartPointer.h"
#include "vtkTypedArray.h"

#include <stdexcept>
#include <vector>

#include <iterator>
#include <limits>

///////////////////////////////////////////////////////////////////////////////
// vtkSelectArraySlices

vtkStandardNewMacro(vtkSelectArraySlices);

vtkSelectArraySlices::vtkSelectArraySlices() :
  SliceDimension(0),
  MinimumCount(1),
  MaximumCount(std::numeric_limits<vtkIdType>::max()),
  MinimumPercent(0.0),
  MaximumPercent(1.0)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkSelectArraySlices::~vtkSelectArraySlices()
{
}

void vtkSelectArraySlices::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SliceDimension: " << this->SliceDimension << endl;
  os << indent << "MinimumCount: " << this->MinimumCount << endl;
  os << indent << "MaximumCount: " << this->MaximumCount << endl;
  os << indent << "MinimumPercent: " << this->MinimumPercent << endl;
  os << indent << "MaximumPercent: " << this->MaximumPercent << endl;
}

int vtkSelectArraySlices::FillInputPortInformation(int port, vtkInformation* information)
{
  switch(port)
    {
    case 0:
      information->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
    }

    return 0;
}

int vtkSelectArraySlices::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    // Setup our output selection ...
    vtkIdTypeArray* const output_selection_list = vtkIdTypeArray::New();
    
    vtkSelectionNode* const output_selection_node = vtkSelectionNode::New();
    output_selection_node->SetContentType(vtkSelectionNode::INDICES);
    output_selection_node->SetFieldType(vtkSelectionNode::ROW);
    output_selection_node->SetSelectionList(output_selection_list);
    output_selection_list->Delete();

    vtkSelection* const output_selection = vtkSelection::GetData(outputVector);
    output_selection->AddNode(output_selection_node);
    output_selection_node->Delete();

    // Enforce our preconditions ...
    vtkArrayData* const input_array_data = vtkArrayData::GetData(inputVector[0]);
    if(!input_array_data)
      throw std::runtime_error("Missing vtkArrayData on input port 0.");
    if(input_array_data->GetNumberOfArrays() != 1)
      throw std::runtime_error("vtkArrayData on input port 0 must contain exactly one vtkArray.");
    vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(
      input_array_data->GetArray(0));
    if(!input_array)
      throw std::runtime_error("vtkArray on input port 0 must be a vtkTypedArray<double>.");

    const vtkIdType dimension = this->SliceDimension;

    if(dimension < 0 || dimension >= input_array->GetDimensions())
      throw std::runtime_error("SliceDimension out-of-range.");

    const vtkArrayRange dimension_extents = input_array->GetExtent(dimension);

    // Special-case: if the dimension extents are empty, there's nothing to select and we're done.
    if(0 == dimension_extents.GetSize())
      return 1;
    
    const vtkIdType slice_extents = input_array->GetExtents().GetSize() / dimension_extents.GetSize();
    const vtkIdType non_null_count = input_array->GetNonNullSize();

    // Compute the number of non-zero values in each slice along the target dimension ...
    vtkArrayCoordinates coordinates;
    std::vector<vtkIdType> slice_counts(dimension_extents.GetSize(), 0);
    for(vtkIdType n = 0; n != non_null_count; ++n)
      {
      input_array->GetCoordinatesN(n, coordinates);
      slice_counts[coordinates[dimension] - dimension_extents.GetBegin()] += (input_array->GetValueN(n) ? 1 : 0);
      }

    // Select / deselect each slice based on whether its count meets our criteria ...
    const vtkIdType minimum_count = this->MinimumCount;
    const vtkIdType maximum_count = this->MaximumCount;
    const vtkIdType minimum_percent_count = static_cast<vtkIdType>(this->MinimumPercent * slice_extents);
    const vtkIdType maximum_percent_count = static_cast<vtkIdType>(this->MaximumPercent * slice_extents);

    for(vtkIdType i = 0; i != dimension_extents.GetSize(); ++i)
      {
      const vtkIdType count = slice_counts[i];
      if(count >= minimum_count && count >= minimum_percent_count && count <= maximum_count && count <= maximum_percent_count)
        {
        output_selection_list->InsertNextValue(i);
        }
      }

    return 1;
    } 
  catch(std::exception& e)
    {
    vtkErrorMacro(<< "caught exception: " << e.what() << endl);
    }
  catch(...)
    {
    vtkErrorMacro(<< "caught unknown exception." << endl);
    }

  return 0;
}

