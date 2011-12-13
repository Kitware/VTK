/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFoldCase.cxx

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
#include <vtkFoldCase.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkUnicodeStringArray.h>

#include <stdexcept>

vtkStandardNewMacro(vtkFoldCase);

vtkFoldCase::vtkFoldCase() :
  ResultArray(0)
{
  this->SetResultArray("text");
  this->SetInputArrayToProcess(0, 0, 0, 6, "text");
  this->SetNumberOfInputPorts(1);
}

vtkFoldCase::~vtkFoldCase()
{
  this->SetResultArray(0);
}

void vtkFoldCase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ResultArray: " << (this->ResultArray ? this->ResultArray : "(none)") << "\n";
}

int vtkFoldCase::RequestData(
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

    const vtkIdType count = input_array->GetNumberOfTuples();

    vtkUnicodeStringArray* const output_array = vtkUnicodeStringArray::New();
    output_array->SetName(this->ResultArray);
    output_array->SetNumberOfTuples(count);
    for(vtkIdType i = 0; i < count; ++i)
      {
      output_array->SetValue(i, input_array->GetValue(i).fold_case());

      if( i % 100 == 0 )
        {
        //emit progress...
        double progress = static_cast<double>(i) / static_cast<double>(count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }

    vtkTable* const output_table = vtkTable::GetData(outputVector);
    output_table->ShallowCopy(input_table);
    output_table->GetRowData()->AddArray(output_array);
    output_array->Delete();
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

