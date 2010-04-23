/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnityMatrixWeighting.cxx
  
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

#include "vtkCommand.h"
#include "vtkDenseArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUnityMatrixWeighting.h"

#include <vtkstd/stdexcept>

///////////////////////////////////////////////////////////////////////////////
// vtkUnityMatrixWeighting

vtkStandardNewMacro(vtkUnityMatrixWeighting);

vtkUnityMatrixWeighting::vtkUnityMatrixWeighting() :
  FeatureDimension(0)
{
}

vtkUnityMatrixWeighting::~vtkUnityMatrixWeighting()
{
}

void vtkUnityMatrixWeighting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FeatureDimension: " << this->FeatureDimension << endl;
}

int vtkUnityMatrixWeighting::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    // Test our preconditions ...
    vtkArrayData* const input_data = vtkArrayData::GetData(inputVector[0]);
    if(!input_data)
      throw vtkstd::runtime_error("Missing input vtkArrayData on port 0.");
    if(input_data->GetNumberOfArrays() != 1)
      throw vtkstd::runtime_error("Input vtkArrayData must contain exactly one array.");
    vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(input_data->GetArray(0));
    if(!input_array)
      throw vtkstd::runtime_error("Input array must be a vtkTypedArray<double>.");
    if(input_array->GetDimensions() != 2)
      throw vtkstd::runtime_error("Input array must be a matrix.");

    vtkIdType feature_dimension;
    switch(this->FeatureDimension)
      {
      case 0:
        feature_dimension = 0;
        break;
      case 1:
        feature_dimension = 1;
        break;
      default:
        throw vtkstd::runtime_error("FeatureDimension out-of-bounds.");
      }

    const vtkArrayRange features = input_array->GetExtent(feature_dimension);

    // Setup our output ...
    vtkDenseArray<double>* const output_array = vtkDenseArray<double>::New();
    output_array->Resize(features);
    output_array->Fill(1.0);
    output_array->SetName("trivial_term_weight");

    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();
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

