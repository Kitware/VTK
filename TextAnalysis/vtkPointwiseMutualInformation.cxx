/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointwiseMutualInformation.cxx
  
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

#include "vtkArrayCoordinates.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointwiseMutualInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTypedArray.h"

#include <cmath>
#include <algorithm>
#include <vtkstd/stdexcept>

///////////////////////////////////////////////////////////////////////////////
// vtkPointwiseMutualInformation

vtkStandardNewMacro(vtkPointwiseMutualInformation);

vtkPointwiseMutualInformation::vtkPointwiseMutualInformation()
{
}

vtkPointwiseMutualInformation::~vtkPointwiseMutualInformation()
{
}

void vtkPointwiseMutualInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkPointwiseMutualInformation::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    // Enforce our input preconditions ...
    vtkArrayData* const input_data = vtkArrayData::GetData(inputVector[0]);
    if(!input_data)
      throw vtkstd::runtime_error("Missing vtkArrayData on input port 0.");
    if(input_data->GetNumberOfArrays() != 1)
      throw vtkstd::runtime_error("vtkArrayData on input port 0 must contain exactly one vtkArray.");
    vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(input_data->GetArray(0));
    if(!input_array)
      throw vtkstd::runtime_error("Unsupported input array type.");
    
    // Create an output array ...
    vtkTypedArray<double>* const output_array = vtkTypedArray<double>::SafeDownCast(input_array->DeepCopy());
    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    const vtkIdType dimension_count = input_array->GetDimensions();
    const vtkIdType value_count = input_array->GetNonNullSize();

    if(value_count == 0)
      {
      // Allow for an empty input
      return 1;
      }

    // This is a portable way to compute log base-2 ...
    static const double ln2 = log(2.0);

    // Compute array value sums along each dimension ...
    double array_sum = 0.0;
    vtkstd::vector<vtkstd::vector<double> > dimension_sums(dimension_count);
    for(vtkIdType i = 0; i != dimension_count; ++i)
      {
      dimension_sums[i].resize(input_array->GetExtent(i).GetSize(), 0.0);
      }

    vtkArrayCoordinates coordinates;
    for(vtkIdType n = 0; n != value_count; ++n)
      {
      const double value = input_array->GetValueN(n);
      input_array->GetCoordinatesN(n, coordinates);

      array_sum += value;
      for(vtkIdType i = 0; i != dimension_count; ++i)
        {
        dimension_sums[i][coordinates[i]] += value;
        }
      }

    if(!array_sum)
      throw vtkstd::runtime_error("Cannot compute PMI with zero array probability.");
    for(vtkIdType i = 0; i != dimension_count; ++i)
      {
      if(vtkstd::count(dimension_sums[i].begin(), dimension_sums[i].end(), 0))
        throw vtkstd::runtime_error("Cannot compute PMI with zero dimension sums.");
      }

    // Compute the PMI for each array value ...
    for(vtkIdType n = 0; n != value_count; ++n)
      {
      const double value = input_array->GetValueN(n);
      if(!value)
        {
        output_array->SetValueN(n, 0);
        continue;
        }

      input_array->GetCoordinatesN(n, coordinates);

      double result = value / array_sum;
      for(vtkIdType i = 0; i != dimension_count; ++i)
        {
        result /= (value / dimension_sums[i][coordinates[i]]);
        }

      output_array->SetValueN(n, vtkstd::log(result) / ln2);
      }
    }
  catch(vtkstd::exception& e)
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

