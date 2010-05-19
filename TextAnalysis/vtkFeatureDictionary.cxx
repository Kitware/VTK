/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFeatureDictionary.cxx

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

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkFeatureDictionary.h"
#include "vtkUnicodeStringArray.h"
#include "vtkIdTypeArray.h"

#include <vtkstd/set>
#include <vtkstd/stdexcept>

///////////////////////////////////////////////////////////////////////////////
// vtkFeatureDictionary

vtkStandardNewMacro(vtkFeatureDictionary);

vtkFeatureDictionary::vtkFeatureDictionary()
{
  this->SetInputArrayToProcess(0, 0, 0, 6, "type");
  this->SetInputArrayToProcess(1, 0, 0, 6, "text");

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkFeatureDictionary::~vtkFeatureDictionary()
{
}

void vtkFeatureDictionary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkFeatureDictionary::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    // Enforce our input preconditions ...
    vtkStringArray* const input_type_array = vtkStringArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(0, 0, inputVector));
    if(!input_type_array)
      throw vtkstd::runtime_error("missing input type array");

    vtkUnicodeStringArray* const input_term_array = vtkUnicodeStringArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(1, 0, inputVector));
    if(!input_term_array)
      throw vtkstd::runtime_error("missing input term array");

    // Prepare our outputs ...
    vtkStringArray* const type_array = vtkStringArray::New();
    type_array->SetName("type");

    vtkUnicodeStringArray* const text_array = vtkUnicodeStringArray::New();
    text_array->SetName("text");

    vtkIdTypeArray* const freq_array = vtkIdTypeArray::New();
    freq_array->SetName("frequency");

    vtkTable* const output_table = vtkTable::GetData(outputVector);
    output_table->AddColumn(type_array);
    output_table->AddColumn(text_array);
    output_table->AddColumn(freq_array);

    type_array->Delete();
    text_array->Delete();
    freq_array->Delete();
    // Filter-out duplicate terms ...
    vtkstd::set<vtkUnicodeString> terms;

    const vtkIdType term_count = input_term_array->GetNumberOfTuples();
    for(vtkIdType i = 0; i != term_count; ++i)
      {
      const vtkUnicodeString& term = input_term_array->GetValue(i);
      if(terms.insert(term).second)
        {
        type_array->InsertNextValue(input_type_array->GetValue(i));
        text_array->InsertNextValue(term);
        freq_array->InsertNextValue(1);
        }
      else
        {
        vtkIdType loc = text_array->LookupValue(term);
        vtkIdType count = freq_array->GetValue(loc);
        freq_array->SetValue(loc, ++count);
        }

      if(i % 100 == 0)
        {
        //emit progress...
        double progress = static_cast<double>(i) / static_cast<double>(term_count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }

    return 1;
    } 
  catch(vtkstd::exception& e)
    {
    vtkErrorMacro(<< "caught exception: " << e.what() << endl);
    }
  catch(...)
    {
    vtkErrorMacro(<< "caught unknown exception." << endl);
    }

  return 0;
}

