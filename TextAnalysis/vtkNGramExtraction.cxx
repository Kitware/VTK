/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNGramExtraction.cxx

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
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkUnicodeStringArray.h>
#include <vtkTable.h>
#include <vtkNGramExtraction.h>

#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

vtkStandardNewMacro(vtkNGramExtraction);

vtkNGramExtraction::vtkNGramExtraction() :
  N(1)
{
  this->SetInputArrayToProcess(0, 0, 0, 6, "document");
  this->SetInputArrayToProcess(1, 0, 0, 6, "begin");
  this->SetInputArrayToProcess(2, 0, 0, 6, "end");
  this->SetInputArrayToProcess(3, 0, 0, 6, "text");
  this->SetNumberOfInputPorts(1);
}

vtkNGramExtraction::~vtkNGramExtraction()
{
}

void vtkNGramExtraction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "N: " << this->N << "\n";
}

int vtkNGramExtraction::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  try
    {
    if(this->N < 1)
      throw vtkstd::runtime_error("N must be >= 1");

    // Special-case ... if N == 1 we can simply pass-through everything except the type
    if(this->N == 1)
      {
      vtkTable* const input_table = vtkTable::GetData(inputVector[0]);
      vtkTable* const output_table = vtkTable::GetData(outputVector);
      output_table->ShallowCopy(input_table);

      vtkStringArray* const type_array = vtkStringArray::New();
      type_array->SetName("type");

      for(vtkIdType i = 0; i != input_table->GetNumberOfRows(); ++i)
        type_array->InsertNextValue("1-gram");
      
      output_table->AddColumn(type_array);
      type_array->Delete();
      
      return 1;
      }

    vtkIdTypeArray* const input_document_array = vtkIdTypeArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(0, 0, inputVector));
    if(!input_document_array)
      throw vtkstd::runtime_error("missing input document array");

    vtkIdTypeArray* const input_begin_array = vtkIdTypeArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(1, 0, inputVector));
    if(!input_begin_array)
      throw vtkstd::runtime_error("missing input begin array");

    vtkIdTypeArray* const input_end_array = vtkIdTypeArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(2, 0, inputVector));
    if(!input_end_array)
      throw vtkstd::runtime_error("missing input end array");

    vtkUnicodeStringArray* const input_text_array = vtkUnicodeStringArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(3, 0, inputVector));
    if(!input_text_array)
      throw vtkstd::runtime_error("missing input text array");

    vtkIdTypeArray* const document_array = vtkIdTypeArray::New();
    document_array->SetName("document");

    vtkIdTypeArray* const begin_array = vtkIdTypeArray::New();
    begin_array->SetName("begin");

    vtkIdTypeArray* const end_array = vtkIdTypeArray::New();
    end_array->SetName("end");

    vtkStringArray* const type_array = vtkStringArray::New();
    type_array->SetName("type");

    vtkUnicodeStringArray* const text_array = vtkUnicodeStringArray::New();
    text_array->SetName("text");

    vtkstd::ostringstream buffer;
    buffer << this->N << "-gram";
    const vtkStdString type = buffer.str();

    const vtkIdType count = input_document_array->GetNumberOfTuples();
    for(vtkIdType i = 0; i + this->N <= count; ++i)
      {
      const vtkIdType document = input_document_array->GetValue(i);
      const vtkIdType begin = input_begin_array->GetValue(i);
      vtkIdType end = input_end_array->GetValue(i);
      vtkUnicodeString text = input_text_array->GetValue(i);

      bool document_boundary = false;
      for(vtkIdType n = 1; n < this->N; ++n)
        {
        if(input_document_array->GetValue(i + n) != document)
          {
          document_boundary = true;
          break;
          }
        end = input_end_array->GetValue(i + n);
        text.append(vtkUnicodeString::from_utf8(" "));
        text.append(input_text_array->GetValue(i + n));
        }

      if(!document_boundary)
        {
        document_array->InsertNextValue(document);
        begin_array->InsertNextValue(begin);
        end_array->InsertNextValue(end);
        type_array->InsertNextValue(type);
        text_array->InsertNextValue(text);
        }
      
      if( i % 100 == 0 )
        {
        //emit progress...
        double progress = static_cast<double>(i) / static_cast<double>(count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }

    vtkTable* const output_table = vtkTable::GetData(outputVector);
    output_table->AddColumn(document_array);
    output_table->AddColumn(begin_array);
    output_table->AddColumn(end_array);
    output_table->AddColumn(type_array);
    output_table->AddColumn(text_array);
    document_array->Delete();
    begin_array->Delete();
    end_array->Delete();
    type_array->Delete();
    text_array->Delete();
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

