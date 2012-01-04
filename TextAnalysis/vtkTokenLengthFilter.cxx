/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTokenLengthFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include <vtkCommand.h>
#include <vtkDataSetAttributes.h>
#include <vtkTokenLengthFilter.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkUnicodeStringArray.h>

#include <stdexcept>

vtkStandardNewMacro(vtkTokenLengthFilter);

vtkTokenLengthFilter::vtkTokenLengthFilter() :
  Begin(0),
  End(0)
{
  this->SetInputArrayToProcess(0, 0, 0, 6, "text");
  this->SetNumberOfInputPorts(1);
}

vtkTokenLengthFilter::~vtkTokenLengthFilter()
{
}

void vtkTokenLengthFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Begin: " << this->Begin << "\n";
  os << indent << "End: " << this->End << "\n";
}

int vtkTokenLengthFilter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  try
    {
    vtkTable* const input_table = vtkTable::GetData(inputVector[0]);
    if(!input_table)
      throw std::runtime_error("missing input table");

    vtkUnicodeStringArray* const input_array = vtkUnicodeStringArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(0, 0, inputVector));
    if(!input_array)
      throw std::runtime_error("missing input array");

    vtkDataSetAttributes* const input_attributes = input_table->GetRowData();

    vtkTable* const output_table = vtkTable::GetData(outputVector);
    vtkDataSetAttributes* const output_attributes = output_table->GetRowData();
    output_attributes->CopyAllocate(input_attributes);
    
    int count = input_array->GetNumberOfTuples();
    for(vtkIdType i = 0; i != input_array->GetNumberOfTuples(); ++i)
      {
      const vtkIdType token_length = input_array->GetValue(i).character_count();
      if(this->Begin <= token_length && token_length < this->End)
        continue;

      output_attributes->CopyData(input_attributes, i, output_table->GetNumberOfRows());
      
      if( i % 100 == 0 )
        {
        //emit progress...
        double progress = static_cast<double>(i) / static_cast<double>(count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
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

