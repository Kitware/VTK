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

///////////////////////////////////////////////////////////////////////////////
// vtkBoostLogWeighting

vtkCxxRevisionMacro(vtkBoostLogWeighting, "1.2");
vtkStandardNewMacro(vtkBoostLogWeighting);

vtkBoostLogWeighting::vtkBoostLogWeighting()
{
}

vtkBoostLogWeighting::~vtkBoostLogWeighting()
{
}

void vtkBoostLogWeighting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkBoostLogWeighting::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  if(input->GetNumberOfArrays() != 1)
    {
    vtkErrorMacro(<< "vtkBoostLogWeighting requires vtkArrayData containing exactly one array as input.");
    return 0;
    }

  if(vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(input->GetArray(0)))
    {
    vtkTypedArray<double>* const output_array = vtkTypedArray<double>::SafeDownCast(input_array->DeepCopy());

    const vtkIdType value_count = input_array->GetNonNullSize();
    
    for(vtkIdType i = 0; i != value_count; ++i)
      {
      output_array->SetValueN(i, boost::math::log1p(output_array->GetValueN(i)));

      double progress = static_cast<double>(i) / static_cast<double>(value_count);
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
      }

    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();
    }
  else
    {
    vtkErrorMacro(<< "Unsupported input array type");
    return 0;
    }
  
  return 1;
}

