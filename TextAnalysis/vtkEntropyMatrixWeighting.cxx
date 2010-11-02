/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEntropyMatrixWeighting.cxx
  
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
#include "vtkEntropyMatrixWeighting.h"

#include <vtkstd/stdexcept>

#if defined(WIN32) && !defined(__MINGW32__)

static inline double log2(double n)
{
  return log(n) / log(2.);
}

#endif // WIN32

///////////////////////////////////////////////////////////////////////////////
// vtkEntropyMatrixWeighting

vtkStandardNewMacro(vtkEntropyMatrixWeighting);

vtkEntropyMatrixWeighting::vtkEntropyMatrixWeighting() :
  FeatureDimension(0)
{
}

vtkEntropyMatrixWeighting::~vtkEntropyMatrixWeighting()
{
}

void vtkEntropyMatrixWeighting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FeatureDimension: " << this->FeatureDimension << endl;
}

int vtkEntropyMatrixWeighting::RequestData(
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
    vtkIdType object_dimension;
    switch(this->FeatureDimension)
      {
      case 0:
        feature_dimension = 0;
        object_dimension = 1;
        break;
      case 1:
        feature_dimension = 1;
        object_dimension = 0;
        break;
      default:
        throw vtkstd::runtime_error("FeatureDimension out-of-bounds.");
      }

    const vtkArrayRange features = input_array->GetExtent(feature_dimension);
    const vtkArrayRange objects = input_array->GetExtent(object_dimension);

    // Setup our output ...
    vtkDenseArray<double>* const output_array = vtkDenseArray<double>::New();
    output_array->Resize(features);
    output_array->Fill(0.0);

    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    // Make it happen ...
    output_array->SetName("entropy_weight");

    // Cache log2( number of documents ) ...
    const double logN = log2(static_cast<double>(objects.GetSize()));

    // Cache the frequency of each feature across the entire corpus ...
    vtkstd::vector<double> Fi(features.GetSize(), 0);
    vtkArrayCoordinates coordinates;
    const vtkIdType non_null_count = input_array->GetNonNullSize();
    for(vtkIdType n = 0; n != non_null_count; ++n)
      {
      input_array->GetCoordinatesN(n, coordinates);
      const vtkIdType i = coordinates[feature_dimension];
      const double fij = input_array->GetValueN(n);
      Fi[i - features.GetBegin()] += fij;
      }

    // Compute weights ...
    for(vtkIdType n = 0; n != non_null_count; ++n)
      {
      input_array->GetCoordinatesN(n, coordinates);
      const vtkIdType i = coordinates[feature_dimension];
      const double fij = input_array->GetValueN(n);
      const double pij = fij / Fi[i - features.GetBegin()];
      output_array->SetValue(i, output_array->GetValue(i) + (pij * log2(pij) / logN));
      }

    // Add 1 to each weight ...
    for(vtkIdType i = features.GetBegin(); i != features.GetEnd(); ++i)
      {
      output_array->SetValue(i, output_array->GetValue(i) + 1);
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

