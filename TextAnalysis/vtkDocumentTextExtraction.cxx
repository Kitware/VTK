/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDocumentTextExtraction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include <vtkCommand.h>
#include <vtkDocumentTextExtraction.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkUnicodeStringArray.h>

#include <stdexcept>

vtkCxxRevisionMacro(vtkDocumentTextExtraction, "1.1");
vtkStandardNewMacro(vtkDocumentTextExtraction);

vtkDocumentTextExtraction::vtkDocumentTextExtraction()
{
  this->SetInputArrayToProcess(0, 0, 0, 6, "mime_type");
  this->SetInputArrayToProcess(1, 0, 0, 6, "content");
  this->SetNumberOfInputPorts(1);
}

vtkDocumentTextExtraction::~vtkDocumentTextExtraction()
{
}

void vtkDocumentTextExtraction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkDocumentTextExtraction::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  try
    {
    vtkTable* const input_table = vtkTable::GetData(inputVector[0]);
    if(!input_table)
      throw vtkstd::runtime_error("missing input table");
      
    vtkStringArray* const mime_type_array = vtkStringArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(0, 0, inputVector));
    if(!mime_type_array)
      throw vtkstd::runtime_error("missing mime_type array");

    vtkStringArray* const content_array = vtkStringArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess(1, 0, inputVector));
    if(!content_array)
      throw vtkstd::runtime_error("missing content array");

    vtkUnicodeStringArray* const text_array = vtkUnicodeStringArray::New();
    text_array->SetName("text");

    int count = mime_type_array->GetNumberOfTuples();
    for(vtkIdType i = 0; i != mime_type_array->GetNumberOfTuples(); ++i)
      {
      const vtkStdString& mime_type = mime_type_array->GetValue(i);
      const vtkStdString& content = content_array->GetValue(i);

      // If it's a text document, just copy the data ...
      if(0 == mime_type.find("text/"))
        {
        text_array->InsertNextUTF8Value(content.c_str());
        }
      // Can't identify the file type, so assume there's no text in it ...
      else
        {
        text_array->InsertNextValue(vtkUnicodeString());
        }

      if( i % 100 == 0 )
        {
        //emit progress...
        double progress = static_cast<double>(i) / static_cast<double>(count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }        

    vtkTable* const output_table = vtkTable::GetData(outputVector);
    output_table->ShallowCopy(input_table);
    output_table->AddColumn(text_array);
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

