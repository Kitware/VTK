/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTokenValueFilter.cxx

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
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTextAnalysisUtility.h>
#include <vtkTokenValueFilter.h>
#include <vtkUnicodeStringArray.h>

#include <set>
#include <sstream>
#include <stdexcept>

vtkStandardNewMacro(vtkTokenValueFilter);

class vtkTokenValueFilter::Internals
{
public:
  Internals()
  {
  }

  typedef std::set<vtkUnicodeString> ValuesT;
  ValuesT Values;
};

vtkTokenValueFilter::vtkTokenValueFilter() :
  Implementation(new Internals())
{
  this->SetInputArrayToProcess(0, 0, 0, 6, "text");
  this->SetNumberOfInputPorts(1);
}

vtkTokenValueFilter::~vtkTokenValueFilter()
{
  delete this->Implementation;
}

void vtkTokenValueFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Value Count: " << this->Implementation->Values.size() << endl;
}

void vtkTokenValueFilter::AddStopWordValues()
{
  std::string value;
  std::istringstream buffer(vtkTextAnalysisUtility::DefaultStopWords());
  for(std::getline(buffer, value); buffer; std::getline(buffer, value))
    {
    this->Implementation->Values.insert(vtkUnicodeString::from_utf8(value));
    }

  this->Modified();
}

void vtkTokenValueFilter::AddValue(const vtkUnicodeString& value)
{
  this->Implementation->Values.insert(value);
  this->Modified();
}

void vtkTokenValueFilter::ClearValues()
{
  this->Implementation->Values.clear();
  this->Modified();
}

int vtkTokenValueFilter::RequestData(
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
      if(this->Implementation->Values.count(input_array->GetValue(i)))
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

