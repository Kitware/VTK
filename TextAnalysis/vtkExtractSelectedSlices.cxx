/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedSlices.cxx
  
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

#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkExtractSelectedSlices.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"

#include <vtkstd/stdexcept>
#include <vtkstd/vector>

#include <iterator>

///////////////////////////////////////////////////////////////////////////////
// vtkExtractSelectedSlices

vtkStandardNewMacro(vtkExtractSelectedSlices);

vtkExtractSelectedSlices::vtkExtractSelectedSlices() :
  SliceDimension(0)
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

vtkExtractSelectedSlices::~vtkExtractSelectedSlices()
{
}

void vtkExtractSelectedSlices::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SliceDimension: " << this->SliceDimension << endl;
}

int vtkExtractSelectedSlices::FillInputPortInformation(int port, vtkInformation* information)
{
  switch(port)
    {
    case 0:
      information->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;

    case 1:
      information->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
      return 1;
    }

    return 0;
}

int vtkExtractSelectedSlices::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    // Enforce our preconditions ...
    vtkArrayData* const input_array_data = vtkArrayData::GetData(inputVector[0]);
    if(!input_array_data)
      throw vtkstd::runtime_error("Missing vtkArrayData on input port 0.");
    if(input_array_data->GetNumberOfArrays() != 1)
      throw vtkstd::runtime_error("vtkArrayData on input port 0 must contain exactly one vtkArray.");
    vtkSparseArray<double>* const input_array = vtkSparseArray<double>::SafeDownCast(
      input_array_data->GetArray(0));
    if(!input_array)
      throw vtkstd::runtime_error("vtkArray on input port 0 must be a vtkSparseArray<double>.");

    const vtkIdType non_null_count = input_array->GetNonNullSize();
    const vtkIdType slice_dimension = this->SliceDimension;

    if(slice_dimension < 0 || slice_dimension >= input_array->GetDimensions())
      throw vtkstd::runtime_error("SliceDimension out-of-range.");

    const vtkArrayRange slices = input_array->GetExtent(slice_dimension);

    vtkSelection* const input_selection = vtkSelection::GetData(inputVector[1]);
    if(!input_selection)
      throw vtkstd::runtime_error("Missing vtkSelection on input port 1.");

    if(input_selection->GetNumberOfNodes() != 1)
      throw vtkstd::runtime_error("vtkSelection on input port 1 must contain exactly one vtkSelectionNode.");
    
    vtkSelectionNode* const input_selection_node = input_selection->GetNode(0);
    if(!input_selection_node)
      throw vtkstd::runtime_error("Missing vtkSelectionNode on input port 1.");
    if(input_selection_node->GetContentType() != vtkSelectionNode::INDICES)
      throw vtkstd::runtime_error("vtkSelectionNode on input port 1 must be an INDICES selection.");

    vtkIdTypeArray* const input_selection_list = vtkIdTypeArray::SafeDownCast(input_selection_node->GetSelectionList());
    if(!input_selection_list)
      throw vtkstd::runtime_error("Missing vtkIdTypeArray selection indices on input port 1.");

    // Convert selection indices into a bit-map for constant-time lookups ...
    vtkstd::vector<bool> selected_slice(slices.GetSize(), false);
    for(vtkIdType i = 0; i != input_selection_list->GetNumberOfTuples(); ++i)
      {
      const vtkIdType slice = input_selection_list->GetValue(i);
      if(!slices.Contains(slice))
        throw vtkstd::runtime_error("Selected slice out-of-bounds.");

      selected_slice[slice - slices.GetBegin()] = true;
      }

    // Create a map from old coordinates to new coordinates for constant-time lookups ...
    vtkstd::vector<vtkIdType> coordinate_map(slices.GetSize(), 0);
    for(vtkIdType i = 0, new_coordinate = 0; i != slices.GetSize(); ++i)
      {
      coordinate_map[i] = new_coordinate;
      if(selected_slice[i])
        ++new_coordinate;
      }

    // Setup our output ...
    vtkSparseArray<double>* const output_array = vtkSparseArray<double>::New();
    output_array->Resize(input_array->GetExtents());
    output_array->SetName(input_array->GetName());
    for(vtkIdType i = 0; i != input_array->GetDimensions(); ++i)
      output_array->SetDimensionLabel(i, input_array->GetDimensionLabel(i));

    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    // Map old coordinates to new coordinates ...
    vtkArrayCoordinates coordinates;
    for(vtkIdType n = 0; n != non_null_count; ++n)
      {
      input_array->GetCoordinatesN(n, coordinates);
      if(!selected_slice[coordinates[slice_dimension] - slices.GetBegin()])
        continue;

      coordinates[slice_dimension] = coordinate_map[coordinates[slice_dimension] - slices.GetBegin()];
      output_array->AddValue(coordinates, input_array->GetValueN(n));
      }

    // Reset the array extents ...
    output_array->SetExtentsFromContents();

    return 1;
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

