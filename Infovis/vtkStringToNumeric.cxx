/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringToNumeric.cxx

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

#include "vtkStringToNumeric.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkUnicodeStringArray.h"
#include "vtkVariant.h"

vtkStandardNewMacro(vtkStringToNumeric);

vtkStringToNumeric::vtkStringToNumeric()
{
  this->ConvertFieldData = true;
  this->ConvertPointData = true;
  this->ConvertCellData = true;
}

vtkStringToNumeric::~vtkStringToNumeric()
{
}

int vtkStringToNumeric::CountItemsToConvert(vtkFieldData *fieldData)
{
  int count = 0;
  for (int arr = 0; arr < fieldData->GetNumberOfArrays(); arr++)
    {
    vtkAbstractArray *array = fieldData->GetAbstractArray(arr);
    vtkStringArray* stringArray = vtkStringArray::SafeDownCast(array);
    vtkUnicodeStringArray* unicodeArray = 
      vtkUnicodeStringArray::SafeDownCast(array);
    if (!stringArray && !unicodeArray)
      {
      continue;
      }
    else
      {
      count += array->GetNumberOfTuples() * array->GetNumberOfComponents();
      }
    }

  return count;
}

int vtkStringToNumeric::RequestData(
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

  vtkDataSet* outputDataSet = vtkDataSet::SafeDownCast(output);
  vtkGraph*   outputGraph = vtkGraph::SafeDownCast(output);
  vtkTable*   outputTable = vtkTable::SafeDownCast(output);

  // Figure out how many items we have to process
  int itemCount = 0;
  if (this->ConvertFieldData)
    {
    itemCount += this->CountItemsToConvert(output->GetFieldData());
    }
  if (outputDataSet && this->ConvertPointData)
    {
    itemCount += this->CountItemsToConvert(outputDataSet->GetPointData());
    }
  if (outputDataSet && this->ConvertCellData)
    {
    itemCount += this->CountItemsToConvert(outputDataSet->GetCellData());
    }
  if (outputGraph && this->ConvertPointData)
    {
    itemCount += this->CountItemsToConvert(outputGraph->GetVertexData());
    }
  if (outputGraph && this->ConvertCellData)
    {
    itemCount += this->CountItemsToConvert(outputGraph->GetEdgeData());
    }
  if (outputTable && this->ConvertPointData)
    {
    itemCount += this->CountItemsToConvert(outputTable->GetRowData());
    }

  this->ItemsToConvert = itemCount;
  this->ItemsConverted = 0;

  if (this->ConvertFieldData)
    {
    this->ConvertArrays(output->GetFieldData());
    }
  if (outputDataSet && this->ConvertPointData)
    {
    this->ConvertArrays(outputDataSet->GetPointData());
    }
  if (outputDataSet && this->ConvertCellData)
    {
    this->ConvertArrays(outputDataSet->GetCellData());
    }
  if (outputGraph && this->ConvertPointData)
    {
    this->ConvertArrays(outputGraph->GetVertexData());
    }
  if (outputGraph && this->ConvertCellData)
    {
    this->ConvertArrays(outputGraph->GetEdgeData());
    }
  if (outputTable && this->ConvertPointData)
    {
    this->ConvertArrays(outputTable->GetRowData());
    }
  
  return 1;
}

void vtkStringToNumeric::ConvertArrays(vtkFieldData* fieldData)
{
  for (int arr = 0; arr < fieldData->GetNumberOfArrays(); arr++)
    {
    vtkStringArray* stringArray = vtkStringArray::SafeDownCast(
      fieldData->GetAbstractArray(arr));
    vtkUnicodeStringArray* unicodeArray = vtkUnicodeStringArray::SafeDownCast(
      fieldData->GetAbstractArray(arr));
    if (!stringArray && !unicodeArray)
      {
      continue;
      }
    
    vtkIdType numTuples,numComps;
    vtkStdString arrayName;
    if (stringArray)
      {
      numTuples = stringArray->GetNumberOfTuples();
      numComps = stringArray->GetNumberOfComponents();
      arrayName = stringArray->GetName();
      }
    else
      {
      numTuples = unicodeArray->GetNumberOfTuples();
      numComps = unicodeArray->GetNumberOfComponents();
      arrayName = unicodeArray->GetName();
      }

    // Set up the output array
    vtkDoubleArray* doubleArray = vtkDoubleArray::New();
    doubleArray->SetNumberOfComponents(numComps);
    doubleArray->SetNumberOfTuples(numTuples);
    doubleArray->SetName(arrayName);
  
    // Set up the output array
    vtkIntArray* intArray = vtkIntArray::New();
    intArray->SetNumberOfComponents(numComps);
    intArray->SetNumberOfTuples(numTuples);
    intArray->SetName(arrayName);
  
    // Convert the strings to time point values
    bool allInteger = true;
    bool allNumeric = true;
    for (vtkIdType i = 0; i < numTuples*numComps; i++)
      {
      ++ this->ItemsConverted;
      if (this->ItemsConverted % 100 == 0)
        {
        this->UpdateProgress(static_cast<double>(this->ItemsConverted) /
                             static_cast<double>(this->ItemsToConvert));
        }

      vtkStdString str;
      if (stringArray)
        {
        str = stringArray->GetValue(i);
        }
      else
        {
        str = unicodeArray->GetValue(i).utf8_str(); 
        }
      bool ok;
      if (allInteger)
        {
        if (str.length() == 0)
          {
          intArray->SetValue(i, 0);
          doubleArray->SetValue(i, 0.0);
          continue;
          }
        int intValue = vtkVariant(str).ToInt(&ok);
        if (ok)
          {
          double doubleValue = intValue;
          intArray->SetValue(i, intValue);
          doubleArray->SetValue(i, doubleValue);
          }
        else
          {
          allInteger = false;
          }
        }
      if (!allInteger)
        {
        if (str.length() == 0)
          {
          doubleArray->SetValue(i, 0.0);
          continue;
          }
        double doubleValue = vtkVariant(str).ToDouble(&ok);
        if (!ok)
          {
          allNumeric = false;
          break;
          }
        else
          {
          doubleArray->SetValue(i, doubleValue);
          }
        }
      }
    if (allNumeric)
      {
      // Calling AddArray will replace the old array since the names match.
      if (allInteger && (numTuples*numComps)) // Are they all ints, and did I test anything?
        {
        fieldData->AddArray(intArray);
        }
      else
        {
        fieldData->AddArray(doubleArray);
        }
      }
    intArray->Delete();
    doubleArray->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkStringToNumeric::ProcessRequest(
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
int vtkStringToNumeric::RequestDataObject(
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

void vtkStringToNumeric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConvertFieldData: " 
    << (this->ConvertFieldData ? "on" : "off") << endl;
  os << indent << "ConvertPointData: " 
    << (this->ConvertPointData ? "on" : "off") << endl;
  os << indent << "ConvertCellData: " 
    << (this->ConvertCellData ? "on" : "off") << endl;
}
