/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPowerWeighting.cxx
  
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
#include "vtkPowerWeighting.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTypedArray.h"

#include <math.h>
#include <vtkstd/stdexcept>

///////////////////////////////////////////////////////////////////////////////
// vtkPowerWeighting

vtkStandardNewMacro(vtkPowerWeighting);

vtkPowerWeighting::vtkPowerWeighting() :
  Power(2.)
{
}

vtkPowerWeighting::~vtkPowerWeighting()
{
}

void vtkPowerWeighting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Power: " << this->Power << endl;
}

int vtkPowerWeighting::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    vtkArrayData* const input_data = vtkArrayData::GetData(inputVector[0]);
    if(!input_data)
      throw vtkstd::runtime_error("Missing input vtkArrayData on port 0.");
    if(input_data->GetNumberOfArrays() != 1)
      throw vtkstd::runtime_error("Input vtkArrayData must contain exactly one array.");
    vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(input_data->GetArray(0));
    if(!input_array)
      throw vtkstd::runtime_error("Unsupported input array type.");

    vtkTypedArray<double>* const output_array = vtkTypedArray<double>::SafeDownCast(input_array->DeepCopy());
    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    const vtkIdType value_count = input_array->GetNonNullSize();

    for(vtkIdType i = 0; i != value_count; ++i)
      {
      output_array->SetValueN(i, pow(output_array->GetValueN(i), this->Power));
      
      if( i % 100 == 0 )
        {
        //emit progress...
        double progress = static_cast<double>(i) / static_cast<double>(value_count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
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

