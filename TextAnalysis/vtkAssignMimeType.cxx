/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssignMimeType.cxx

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
#include <vtkAssignMimeType.h>
#include <vtkInformation.h>
#include <vtkMimeTypes.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include "vtkMimeTypes.h"

#include <stdexcept>

vtkStandardNewMacro(vtkAssignMimeType);
vtkCxxSetObjectMacro(vtkAssignMimeType,MimeTypes,vtkMimeTypes);

vtkAssignMimeType::vtkAssignMimeType() :
  OutputArray(0),
  DefaultMimeType(0),
  MimeTypes(vtkMimeTypes::New())
{
  this->SetOutputArray("mime_type");
  this->SetDefaultMimeType("");

  this->SetInputArrayToProcess(0, 0, 0, 6, "uri");
  this->SetInputArrayToProcess(1, 0, 0, 6, "content");
  
  this->SetNumberOfInputPorts(1);
}

vtkAssignMimeType::~vtkAssignMimeType()
{
  this->SetMimeTypes(0);
  this->SetDefaultMimeType(0);
  this->SetOutputArray(0);
}

void vtkAssignMimeType::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputArray: " << (this->OutputArray ? this->OutputArray : "(none)") << "\n";
  os << indent << "DefaultMimeType: " << (this->DefaultMimeType ? this->DefaultMimeType : "(none)") << "\n";
  os << indent << "MimeTypes: " << "\n";
  if(this->MimeTypes)
    {
    this->MimeTypes->PrintSelf(os, indent.GetNextIndent());
    }
}

int vtkAssignMimeType::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  try
    {
    if(!this->OutputArray)
      throw vtkstd::runtime_error("missing OutputArray");

    vtkTable* const input_table = vtkTable::GetData(inputVector[0]);
    if(!input_table)
      throw vtkstd::runtime_error("missing input table");
      
    vtkAbstractArray* const uri_array = this->GetInputAbstractArrayToProcess(0, 0, inputVector);
    if(!uri_array)
      throw vtkstd::runtime_error("missing uri array");

    vtkAbstractArray* const content_array = this->GetInputAbstractArrayToProcess(1, 0, inputVector);
    if(!content_array)
      throw vtkstd::runtime_error("missing content array");

    vtkStringArray* const mime_type_array = vtkStringArray::New();
    mime_type_array->SetName(this->OutputArray);

    const vtkIdType count = uri_array->GetNumberOfTuples();
    for(vtkIdType i = 0; i != count; ++i)
      {
      const vtkStdString uri = uri_array->GetVariantValue(i).ToString();
      const vtkStdString content = content_array->GetVariantValue(i).ToString();

      vtkStdString mime_type = this->MimeTypes->Lookup(uri, &content[0], &content[0] + content.size());
      if(mime_type.empty() && this->DefaultMimeType)
        mime_type = this->DefaultMimeType;

      mime_type_array->InsertNextValue(mime_type);

      if( i % 100 == 0 )
        {
        //emit progress...
        double progress = static_cast<double>(i) / static_cast<double>(count);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }        

    vtkTable* const output_table = vtkTable::GetData(outputVector);
    output_table->ShallowCopy(input_table);
    output_table->AddColumn(mime_type_array);
    mime_type_array->Delete();
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

