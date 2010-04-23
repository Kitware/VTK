/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringToCategory.cxx

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

#include "vtkStringToCategory.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <vtksys/stl/set>

vtkStandardNewMacro(vtkStringToCategory);

vtkStringToCategory::vtkStringToCategory()
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               "label");
  this->CategoryArrayName = 0;
  this->SetCategoryArrayName("category");
  this->SetNumberOfOutputPorts(2);
}

vtkStringToCategory::~vtkStringToCategory()
{
  this->SetCategoryArrayName(0);
}

int vtkStringToCategory::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *outKeyInfo = outputVector->GetInformationObject(1);

  // Get the input and output objects
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);

  // This second output stores a list of the unique strings, in the same order
  // as used in the first output.
  vtkTable* stringTable =
      vtkTable::SafeDownCast(outKeyInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkStringArray* strings =
      vtkStringArray::SafeDownCast(stringTable->GetColumnByName("Strings"));
  if (strings)
    {
    strings->SetNumberOfTuples(0);
    }
  else
    {
    strings = vtkStringArray::New();
    strings->SetName("Strings");
    stringTable->AddColumn(strings);
    strings->Delete();
    }

  vtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, 0, inputVector);
  vtkStringArray* stringArr = vtkStringArray::SafeDownCast(arr);
  if (!stringArr)
    {
    vtkErrorMacro("String array input could not be found");
    return 0;
    }

  vtkInformation* arrayInfo = this->GetInputArrayInformation(0);
  // Find where the input array came from
  vtkFieldData* fd = output->GetAttributesAsFieldData(
      arrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION()));
  if (!fd)
    {
    vtkErrorMacro("Could not find where the input array came from");
    return 0;
    }

  // Perform the conversion
  vtkIdType numTuples = stringArr->GetNumberOfTuples();
  int numComp = stringArr->GetNumberOfComponents();
  vtkIntArray* catArr = vtkIntArray::New();
  if (this->CategoryArrayName)
    {
    catArr->SetName(this->CategoryArrayName);
    }
  else
    {
    catArr->SetName("category");
    }
  catArr->SetNumberOfComponents(numComp);
  catArr->SetNumberOfTuples(numTuples);
  fd->AddArray(catArr);
  catArr->Delete();
  vtkIdList* list = vtkIdList::New();
  vtksys_stl::set<vtkStdString> s;
  int category = 0;
  for (vtkIdType i = 0; i < numTuples*numComp; i++)
    {
    if (s.find(stringArr->GetValue(i)) == s.end())
      {
      s.insert(stringArr->GetValue(i));
      strings->InsertNextValue(stringArr->GetValue(i));
      stringArr->LookupValue(stringArr->GetValue(i), list);
      for (vtkIdType j = 0; j < list->GetNumberOfIds(); j++)
        {
        catArr->SetValue(list->GetId(j), category);
        }
      ++category;
      }
    }
  list->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkStringToCategory::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkStringToCategory::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(info);
        newOutput->Delete();
        }
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkStringToCategory::FillOutputPortInformation(int port,
                                                   vtkInformation* info)
{
  // now add our info
  if (port == 0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
    }
  else
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    }
  return 1;
}

void vtkStringToCategory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CategoryArrayName: " << (this->CategoryArrayName ? this->CategoryArrayName : "(null)") << endl;
}
