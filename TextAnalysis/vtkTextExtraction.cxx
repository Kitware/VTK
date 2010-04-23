/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextExtraction.cxx

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
#include <vtkPlainTextExtractionStrategy.h>
#include <vtkTextExtraction.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkUnicodeStringArray.h>

#include <vtkstd/algorithm>
#include <vtkstd/stdexcept>

////////////////////////////////////////////////////////////////
// vtkTextExtraction::Implementation

class vtkTextExtraction::Implementation
{
public:
  vtkstd::vector<vtkTextExtractionStrategy*> Strategies;
};

////////////////////////////////////////////////////////////////
// vtkTextExtraction

vtkStandardNewMacro(vtkTextExtraction);

vtkTextExtraction::vtkTextExtraction() :
  OutputArray(0),
  Internal(new Implementation())
{
  this->Internal->Strategies.push_back(vtkPlainTextExtractionStrategy::New());
  
  this->SetOutputArray("text");
  
  this->SetInputArrayToProcess(0, 0, 0, 6, "document");
  this->SetInputArrayToProcess(1, 0, 0, 6, "uri");
  this->SetInputArrayToProcess(2, 0, 0, 6, "mime_type");
  this->SetInputArrayToProcess(3, 0, 0, 6, "content");
  
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(2);
}

vtkTextExtraction::~vtkTextExtraction()
{
  this->ClearStrategies();
  this->SetOutputArray(0);
  delete this->Internal;
}

void vtkTextExtraction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputArray: " << (this->OutputArray ? this->OutputArray : "(none)") << endl;

  for(unsigned int i = 0; i != this->Internal->Strategies.size(); ++i)
    {
    os << indent << "Strategy: " << this->Internal->Strategies[i]->GetClassName() << endl;
    this->Internal->Strategies[i]->PrintSelf(os, indent.GetNextIndent());
    }
}

void vtkTextExtraction::ClearStrategies()
{
  for(unsigned int i = 0; i != this->Internal->Strategies.size(); ++i)
    this->Internal->Strategies[i]->UnRegister(NULL);
  this->Modified();
}

void vtkTextExtraction::PrependStrategy(vtkTextExtractionStrategy* strategy)
{
  if(!strategy)
    {
    vtkErrorMacro(<< "Cannot prepend NULL strategy.");
    return;
    }

  if(vtkstd::count(this->Internal->Strategies.begin(), this->Internal->Strategies.end(), strategy))
    {
    vtkErrorMacro(<< "Cannot prepend the same strategy twice.");
    return;
    }

  strategy->Register(NULL);
  this->Internal->Strategies.insert(this->Internal->Strategies.begin(), strategy);
  this->Modified();
}

void vtkTextExtraction::AppendStrategy(vtkTextExtractionStrategy* strategy)
{
  if(!strategy)
    {
    vtkErrorMacro(<< "Cannot append NULL strategy.");
    return;
    }

  if(vtkstd::count(this->Internal->Strategies.begin(), this->Internal->Strategies.end(), strategy))
    {
    vtkErrorMacro(<< "Cannot append the same strategy twice.");
    return;
    }

  strategy->Register(NULL);
  this->Internal->Strategies.insert(this->Internal->Strategies.end(), strategy);
  this->Modified();
}

int vtkTextExtraction::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  try
    {
    vtkTable* const input_table = vtkTable::GetData(inputVector[0]);
    if(!input_table)
      throw vtkstd::runtime_error("missing input table");

    vtkIdTypeArray* const document_id_array = vtkIdTypeArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(0, 0, inputVector));
    if(!document_id_array)
      throw vtkstd::runtime_error("Missing document id array.");
       
    vtkAbstractArray* const uri_array = this->GetInputAbstractArrayToProcess(1, 0, inputVector);
    if(!uri_array)
      throw vtkstd::runtime_error("Missing uri array.");

    vtkStringArray* const mime_type_array = vtkStringArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(2, 0, inputVector));
    if(!mime_type_array)
      throw vtkstd::runtime_error("Missing mime_type array.");

    vtkAbstractArray* const content_array = this->GetInputAbstractArrayToProcess(3, 0, inputVector);
    if(!content_array)
      throw vtkstd::runtime_error("Missing content array.");

    vtkUnicodeStringArray* const text_array = vtkUnicodeStringArray::New();
    text_array->SetName("text");

    vtkIdTypeArray* const tag_document_array = vtkIdTypeArray::New();
    tag_document_array->SetName("document");

    vtkIdTypeArray* const tag_begin_array = vtkIdTypeArray::New();
    tag_begin_array->SetName("begin");

    vtkIdTypeArray* const tag_end_array = vtkIdTypeArray::New();
    tag_end_array->SetName("end");

    vtkStringArray* const tag_type_array = vtkStringArray::New();
    tag_type_array->SetName("type");

    int count = document_id_array->GetNumberOfTuples();
    for(vtkIdType i = 0; i != count; ++i)
      {
      const vtkIdType document = document_id_array->GetValue(i);
      const vtkStdString& uri = uri_array->GetVariantValue(i).ToString();
      const vtkStdString& mime_type = mime_type_array->GetValue(i);
      const vtkStdString content = content_array->GetVariantValue(i).ToString();

      vtkUnicodeString text;
      for(size_t j = 0; j != this->Internal->Strategies.size(); ++j)
        {
        if(this->Internal->Strategies[j]->Extract(
          document,
          uri,
          mime_type,
          reinterpret_cast<const vtkTypeUInt8*>(content.data()),
          reinterpret_cast<const vtkTypeUInt8*>(content.data() + content.size()),
          text,
          tag_document_array,
          tag_begin_array,
          tag_end_array,
          tag_type_array))
          {
          break;
          }
        }
      text_array->InsertNextValue(text);
      
      if(i % 100 == 0)
        {
        double progress = static_cast<double>(i) / static_cast<double>(count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }        

    vtkTable* const output_table = vtkTable::GetData(outputVector, 0);
    output_table->ShallowCopy(input_table);
    output_table->AddColumn(text_array);
    text_array->Delete();

    vtkTable* const output_tags = vtkTable::GetData(outputVector, 1);
    output_tags->AddColumn(tag_document_array);
    output_tags->AddColumn(tag_begin_array);
    output_tags->AddColumn(tag_end_array);
    output_tags->AddColumn(tag_type_array);

    tag_document_array->Delete();
    tag_begin_array->Delete();
    tag_end_array->Delete();
    tag_type_array->Delete();
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

