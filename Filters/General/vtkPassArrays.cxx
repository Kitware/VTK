/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassArrays.cxx

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

#include "vtkPassArrays.h"

#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

vtkStandardNewMacro(vtkPassArrays);

namespace
{
  // returns true if modified
  typedef std::vector<std::pair<int, std::string> > ArraysType;
  bool ClearArraysOfType(int type, ArraysType& arrays)
  {
    bool retVal = false;
    ArraysType::iterator iter = arrays.begin();
    while (iter != arrays.end())
      {
      if (iter->first == type)
        {
        iter = arrays.erase(iter);
        retVal = true;
        }
      else
        {
        ++iter;
        }
      }
    return retVal;
  }
}

class vtkPassArrays::Internals
{
public:
  ArraysType Arrays;
  std::vector<int> FieldTypes;
};

vtkPassArrays::vtkPassArrays()
{
  this->Implementation = new Internals();
  this->RemoveArrays = false;
  this->UseFieldTypes = false;
}

vtkPassArrays::~vtkPassArrays()
{
  delete this->Implementation;
}

void vtkPassArrays::AddArray(int fieldType, const char* name)
{
  if(!name)
    {
    vtkErrorMacro("name cannont be null.");
    return;
    }
  std::string n=name;
  this->Implementation->Arrays.push_back(std::make_pair(fieldType,n));
  this->Modified();
}

void vtkPassArrays::AddPointDataArray(const char* name)
{
  this->AddArray(vtkDataObject::POINT, name);
}

void vtkPassArrays::AddCellDataArray(const char* name)
{
  this->AddArray(vtkDataObject::CELL, name);
}

void vtkPassArrays::AddFieldDataArray(const char* name)
{
  this->AddArray(vtkDataObject::FIELD, name);
}

void vtkPassArrays::RemoveArray(int fieldType, const char* name)
{
  if(!name)
    {
    vtkErrorMacro("name cannot be null.");
    return;
    }
  ArraysType::iterator iter = this->Implementation->Arrays.begin();
  while (iter != this->Implementation->Arrays.end())
    {
    if (iter->first == fieldType && iter->second == name)
      {
      iter = this->Implementation->Arrays.erase(iter);
      this->Modified();
      }
    else
      {
      ++iter;
      }
    }
}

void vtkPassArrays::RemovePointDataArray(const char* name)
{
  this->RemoveArray(vtkDataObject::POINT, name);
}

void vtkPassArrays::RemoveCellDataArray(const char* name)
{
  this->RemoveArray(vtkDataObject::CELL, name);
}

void vtkPassArrays::RemoveFieldDataArray(const char* name)
{
  this->RemoveArray(vtkDataObject::FIELD, name);
}

void vtkPassArrays::ClearArrays()
{
  if(this->Implementation->Arrays.empty() == false)
    {
    this->Modified();
    }
  this->Implementation->Arrays.clear();
}

void vtkPassArrays::ClearPointDataArrays()
{
  if(ClearArraysOfType(vtkDataObject::POINT, this->Implementation->Arrays) == true)
    {
    this->Modified();
    }
}

void vtkPassArrays::ClearCellDataArrays()
{
  if(ClearArraysOfType(vtkDataObject::CELL, this->Implementation->Arrays) == true)
    {
    this->Modified();
    }
}

void vtkPassArrays::ClearFieldDataArrays()
{
  if(ClearArraysOfType(vtkDataObject::FIELD, this->Implementation->Arrays) == true)
    {
    this->Modified();
    }
}

void vtkPassArrays::AddFieldType(int fieldType)
{
  this->Implementation->FieldTypes.push_back(fieldType);
  this->Modified();
}

void vtkPassArrays::ClearFieldTypes()
{
  this->Implementation->FieldTypes.clear();
  this->Modified();
}

int vtkPassArrays::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the input and output objects
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);

  // If we are specifying arrays to add, start with no arrays in output
  if (!this->RemoveArrays)
    {
    if (this->UseFieldTypes)
      {
      for (std::vector<int>::size_type i = 0; i < this->Implementation->FieldTypes.size(); ++i)
        {
        vtkFieldData* outData = output->GetAttributesAsFieldData(
          this->Implementation->FieldTypes[i]);
        if (outData)
          {
          outData->Initialize();
          }
        }
      }
    else
      {
      for (ArraysType::size_type i = 0; i < this->Implementation->Arrays.size(); ++i)
        {
        vtkFieldData* outData = output->GetAttributesAsFieldData(
          this->Implementation->Arrays[i].first);
        if (outData)
          {
          outData->Initialize();
          }
        }
      }
    }

  ArraysType::iterator it, itEnd;
  itEnd = this->Implementation->Arrays.end();
  for (it = this->Implementation->Arrays.begin(); it != itEnd; ++it)
    {
    if (this->UseFieldTypes)
      {
      // Make sure this is a field type we are interested in
      if (std::find(
          this->Implementation->FieldTypes.begin(),
          this->Implementation->FieldTypes.end(), it->first) ==
          this->Implementation->FieldTypes.end())
        {
        continue;
        }
      }

    vtkFieldData* data = input->GetAttributesAsFieldData(it->first);
    vtkFieldData* outData = output->GetAttributesAsFieldData(it->first);
    if (!data)
      {
      continue;
      }
    vtkAbstractArray* arr = data->GetAbstractArray(it->second.c_str());
    if (!arr)
      {
      continue;
      }
    if (this->RemoveArrays)
      {
      outData->RemoveArray(it->second.c_str());
      }
    else
      {
      outData->AddArray(arr);

      // Preserve attribute type if applicable
      vtkDataSetAttributes* attrib = vtkDataSetAttributes::SafeDownCast(data);
      vtkDataSetAttributes* outAttrib = vtkDataSetAttributes::SafeDownCast(outData);
      if (attrib)
        {
        for (int a = 0; a < vtkDataSetAttributes::NUM_ATTRIBUTES; ++a)
          {
          if (attrib->GetAbstractAttribute(a) == arr)
            {
            outAttrib->SetActiveAttribute(it->second.c_str(), a);
            }
          }
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPassArrays::ProcessRequest(
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
int vtkPassArrays::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    // Skip composite data sets so that executives will treat this as a simple filter
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPistonDataObject");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPassArrays::RequestDataObject(
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
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
        }
      }
    return 1;
    }
  return 0;
}

void vtkPassArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RemoveArrays: " << (this->RemoveArrays ? "on" : "off") << endl;
  os << indent << "UseFieldTypes: " << (this->UseFieldTypes ? "on" : "off") << endl;
}
