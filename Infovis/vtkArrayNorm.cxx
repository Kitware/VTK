/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayNorm.cxx
  
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
#include "vtkArrayNorm.h"

#include <vtksys/ios/sstream>
#include <vtkstd/limits>
#include <vtkstd/stdexcept>

///////////////////////////////////////////////////////////////////////////////
// vtkArrayNorm

vtkStandardNewMacro(vtkArrayNorm);

vtkArrayNorm::vtkArrayNorm() :
  Dimension(0),
  L(2),
  Invert(false),
  Window(0, vtkstd::numeric_limits<vtkIdType>::max())
{
}

vtkArrayNorm::~vtkArrayNorm()
{
}

void vtkArrayNorm::SetWindow(const vtkArrayRange& window)
{
  if(window == this->Window)
    return;

  this->Window = window;
  this->Modified();
}

vtkArrayRange vtkArrayNorm::GetWindow()
{
  return this->Window;
}

void vtkArrayNorm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "L: " << this->L << endl;
  os << indent << "Invert: " << this->Invert << endl;
  os << indent << "Window: " << this->Window << endl;
}

void vtkArrayNorm::SetL(int value)
{
  if(value < 1)
    {
    vtkErrorMacro(<< "Cannot compute array norm for L < 1");
    return;
    }
  
  if(this->L == value)
    return;

  this->L = value;
  this->Modified();
}

int vtkArrayNorm::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    // Test our preconditions ...
    vtkArrayData* const input_data = vtkArrayData::GetData(inputVector[0]);
    if(!input_data)
      throw vtkstd::runtime_error("Missing vtkArrayData on input port 0.");
    if(input_data->GetNumberOfArrays() != 1)
      throw vtkstd::runtime_error("vtkArrayData on input port 0 must contain exactly one vtkArray.");
    vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(
      input_data->GetArray(static_cast<vtkIdType>(0)));
    if(!input_array)
      throw vtkstd::runtime_error("vtkArray on input port 0 must be a vtkTypedArray<double>.");
    if(input_array->GetDimensions() != 2)
      throw vtkstd::runtime_error("vtkArray on input port 0 must be a matrix.");

    const vtkIdType vector_dimension = this->Dimension;
    if(vector_dimension < 0 || vector_dimension > 1)
      throw vtkstd::runtime_error("Dimension must be zero or one.");
    const vtkIdType element_dimension = 1 - vector_dimension;

    // Setup our output ...
    vtkstd::ostringstream array_name;
    array_name << "L" << this->L << "_norm";
    
    vtkDenseArray<double>* const output_array = vtkDenseArray<double>::New();
    output_array->SetName(array_name.str());
    output_array->Resize(input_array->GetExtent(vector_dimension));
    output_array->Fill(0.0);

    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    // Make it happen ...
    vtkArrayCoordinates coordinates;
    const vtkIdType non_null_count = input_array->GetNonNullSize();
    for(vtkIdType n = 0; n != non_null_count; ++n)
      {
      input_array->GetCoordinatesN(n, coordinates);
      if(!this->Window.Contains(coordinates[element_dimension]))
        continue;
      output_array->SetValue(coordinates[vector_dimension], output_array->GetValue(coordinates[vector_dimension]) + pow(input_array->GetValueN(n), this->L));
      }

    for(vtkArray::SizeT n = 0; n != output_array->GetNonNullSize(); ++n)
      {
      output_array->SetValueN(n, pow(output_array->GetValueN(n), 1.0 / this->L));
      }

    // Optionally invert the output vector
    if(this->Invert)
      {
      for(vtkArray::SizeT n = 0; n != output_array->GetNonNullSize(); ++n)
        {
        if(output_array->GetValueN(n))
          output_array->SetValueN(n, 1.0 / output_array->GetValueN(n));
        }
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

