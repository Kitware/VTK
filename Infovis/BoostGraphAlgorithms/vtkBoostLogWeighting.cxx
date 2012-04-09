/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostLogWeighting.cxx
  
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
#include "vtkBoostLogWeighting.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTypedArray.h"

#include <boost/math/special_functions/log1p.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION < 103400
  #error "vtkBoostLogWeighting requires Boost 1.34.0 or later"
#endif

#include <math.h>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////
// vtkBoostLogWeighting

vtkStandardNewMacro(vtkBoostLogWeighting);

vtkBoostLogWeighting::vtkBoostLogWeighting() :
  Base(BASE_E),
  EmitProgress(true)
{
}

vtkBoostLogWeighting::~vtkBoostLogWeighting()
{
}

void vtkBoostLogWeighting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Base: " << this->Base << endl;
  os << indent << "EmitProgress: " 
     << (this->EmitProgress ? "on" : "off") << endl;
}

int vtkBoostLogWeighting::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    vtkArrayData* const input_data = vtkArrayData::GetData(inputVector[0]);
    if(!input_data)
      throw std::runtime_error("Missing input vtkArrayData on port 0.");
    if(input_data->GetNumberOfArrays() != 1)
      throw std::runtime_error("Input vtkArrayData must contain exactly one array.");
    vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(input_data->GetArray(0));
    if(!input_array)
      throw std::runtime_error("Unsupported input array type.");

    vtkTypedArray<double>* const output_array = vtkTypedArray<double>::SafeDownCast(input_array->DeepCopy());
    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    const vtkIdType value_count = input_array->GetNonNullSize();
    switch(this->Base)
      {
      case BASE_E:
        {
        if(this->EmitProgress)
          {
          for(vtkIdType i = 0; i != value_count; ++i)
            {
            output_array->SetValueN(i, boost::math::log1p(output_array->GetValueN(i)));

            double progress = static_cast<double>(i) / static_cast<double>(value_count);
            this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
            }
          }
        else
          {
          for(vtkIdType i = 0; i != value_count; ++i)
            {
            output_array->SetValueN(i, boost::math::log1p(output_array->GetValueN(i)));
            }
          }
        break;
        }
      case BASE_2:
        {
        const double ln2 = log(2.0);
        if(this->EmitProgress)
          {
          for(vtkIdType i = 0; i != value_count; ++i)
            {
            output_array->SetValueN(i, 1.0 + log(output_array->GetValueN(i)) / ln2);

            double progress = static_cast<double>(i) / static_cast<double>(value_count);
            this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
            }
          }
        else
          {
          for(vtkIdType i = 0; i != value_count; ++i)
            {
            output_array->SetValueN(i, 1.0 + log(output_array->GetValueN(i)) / ln2);
            }
          }
        break;
        }
      default:
        throw std::runtime_error("Unknown Base type.");
      }
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

  return 1;
}

