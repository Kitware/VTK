/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaleDimension.cxx
  
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
#include "vtkScaleDimension.h"

#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
// vtkScaleDimension

vtkStandardNewMacro(vtkScaleDimension);

vtkScaleDimension::vtkScaleDimension() :
  Dimension(0),
  Invert(false)
{
  this->SetNumberOfInputPorts(2);
}

vtkScaleDimension::~vtkScaleDimension()
{
}

void vtkScaleDimension::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "Invert: " << this->Invert << endl;
}

int vtkScaleDimension::FillInputPortInformation(int port, vtkInformation* information)
{
  switch(port)
    {
    case 0:
      information->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
    case 1:
      information->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
    }

    return 0;
}

int vtkScaleDimension::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    // Enforce our preconditions ...
    vtkArrayData* const input_array_data = vtkArrayData::GetData(inputVector[0]);
    if(!input_array_data)
      throw std::runtime_error("Missing array input.");
    if(input_array_data->GetNumberOfArrays() != 1)
      throw std::runtime_error("Array input must contain exactly one vtkArray.");
    vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(input_array_data->GetArray(0));
    if(!input_array)
      throw std::runtime_error("Array input must be a vtkTypedArray<double>.");

    if(this->Dimension < 0 || this->Dimension >= input_array->GetDimensions())
      throw std::runtime_error("Scale dimension out-of-range.");

    vtkArrayData* const scale_vector_data = vtkArrayData::GetData(inputVector[1]);
    if(!scale_vector_data)
      throw std::runtime_error("Missing vector input.");
    if(scale_vector_data->GetNumberOfArrays() != 1)
      throw std::runtime_error("Vector input must contain exactly one vtkArray.");
    vtkDenseArray<double>* const scale_vector = vtkDenseArray<double>::SafeDownCast(scale_vector_data->GetArray(0));
    if(!scale_vector)
      throw std::runtime_error("Vector input must be a vtkDenseArray<double>.");
    if(scale_vector->GetDimensions() != 1)
      throw std::runtime_error("Vector input must have exactly one dimension.");

    if(scale_vector->GetExtent(0).GetSize() != input_array->GetExtent(this->Dimension).GetSize())
      throw std::runtime_error("Vector extents must match Array extents along the scale dimension.");

    // Optionally invert the input vector
    std::vector<double> scale(scale_vector->GetStorage(), scale_vector->GetStorage() + scale_vector->GetExtent(0).GetSize());
    if(this->Invert)
      {
      for(unsigned int i = 0; i != scale.size(); ++i)
        {
        if(scale[i])
          scale[i] = 1.0 / scale[i];
        }
      }

    // Setup our output ...
    vtkTypedArray<double>* const output_array = vtkTypedArray<double>::SafeDownCast(input_array->DeepCopy());
    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    // Multiply each element of our output array by the corresponding element in the scale vector.
    vtkArrayCoordinates coordinates;
    const vtkIdType offset = output_array->GetExtent(this->Dimension).GetBegin();
    const vtkIdType element_count = output_array->GetNonNullSize();
    for(vtkIdType n = 0; n != element_count; ++n)
      {
      output_array->GetCoordinatesN(n, coordinates);
      output_array->SetValueN(n, output_array->GetValueN(n) * scale[coordinates[this->Dimension] - offset]);

      if( n % 100 == 0 )
        {
        //emit progress...
        double progress = static_cast<double>(n) / static_cast<double>(element_count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }

    return 1;
    }
  catch(std::exception& e)
    {
    vtkErrorMacro(<< "unhandled exception: " << e.what());
    return 0;
    }
  catch(...)
    {
    vtkErrorMacro(<< "unknown exception");
    return 0;
    }
}

